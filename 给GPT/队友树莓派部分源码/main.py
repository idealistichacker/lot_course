import json
from datetime import datetime

import paho.mqtt.client as mqtt

from config import (
    MQTT_BROKER,
    MQTT_PORT,
    SUB_TOPIC,
    PUB_TOPIC,
    DEVICE_MAP,
    POT_PROFILES,
)
from parser import parse
from rag import query, add_knowledge, count_knowledge
from llm import llm_agent
from state import should_decide, update_decision_state


client = mqtt.Client()


def get_device_meta(device_id: str) -> dict:
    """
    获取设备绑定的作物、地区、类型。
    如果没有找到，就默认 greenhouse_1。
    """
    meta = DEVICE_MAP.get(device_id)

    if not meta:
        meta = DEVICE_MAP["greenhouse_1"].copy()
    else:
        meta = meta.copy()

    if meta.get("type") == "indoor_pot":
        pot_size = meta.get("pot_size", "small_pot")
        meta["pot_profile"] = POT_PROFILES.get(pot_size, POT_PROFILES["small_pot"])

    return meta


def on_connect(client, userdata, flags, rc):
    print("\n==============================")
    print("✅ MQTT连接成功")
    print("📡 订阅主题:", SUB_TOPIC)
    print("📤 发布主题:", PUB_TOPIC)
    print("==============================\n")

    client.subscribe(SUB_TOPIC)


def on_message(client, userdata, msg):
    raw = msg.payload.decode(errors="replace")

    print("\n==============================")
    print("📥 收到MQTT数据")
    print("⏰ 时间:", datetime.now())
    print("📦 原始数据:", raw)

    sensor = parse(raw)

    if not sensor:
        print("❌ 数据解析失败，跳过")
        return

    print("🧠 解析结果:")
    print(json.dumps(sensor, ensure_ascii=False, indent=2))

    device_id = sensor.get("device_id", "greenhouse_1")
    meta = get_device_meta(device_id)

    print("🌱 作物信息:")
    print(json.dumps(meta, ensure_ascii=False, indent=2))

    allow, previous_context = should_decide(device_id, sensor)

    if not allow:
        print("⏳ 5分钟内环境变化不明显，跳过本次LLM决策")
        print("📊 状态上下文:")
        print(json.dumps(previous_context, ensure_ascii=False, indent=2))
        return

    category = meta.get("type", "general")

    query_text = json.dumps(
        {
            "sensor": sensor,
            "meta": meta,
            "previous_context": previous_context,
        },
        ensure_ascii=False,
    )

    print("📚 开始查询本地知识库...")
    knowledge = query(query_text, category=category, top_k=5)

    print(f"📚 知识库分类: {category}")
    print(f"📚 当前分类知识数量: {count_knowledge(category)}")
    print(f"📚 命中数量: {len(knowledge)}")

    for i, item in enumerate(knowledge, start=1):
        print(f"  [{i}] {item[:120]}")

    print("🤖 开始调用LLM API...")
    result = llm_agent(sensor, meta, knowledge, previous_context)

    print("🤖 LLM输出结果:")
    print(json.dumps(result, ensure_ascii=False, indent=2))

    # 发布完整控制JSON，而不是只发一个动作
    control_payload = {
        "device_id": device_id,
        "time": str(datetime.now()),
        "result": result,
    }

    if result.get("action") != "无操作":
        client.publish(PUB_TOPIC, json.dumps(control_payload, ensure_ascii=False))
        print("📤 已发送控制指令:")
        print(json.dumps(control_payload, ensure_ascii=False, indent=2))
    else:
        print("ℹ️ 无操作，不发送控制指令")

    update_decision_state(device_id, sensor, result)

    # 写入本地分类知识库
    learn_record = {
        "source": "runtime_decision",
        "time": str(datetime.now()),
        "crop": meta.get("crop"),
        "region": meta.get("region"),
        "type": meta.get("type"),
        "sensor": sensor,
        "previous_context": previous_context,
        "decision": result,
        "text": (
            f"作物:{meta.get('crop')} 地区:{meta.get('region')} 类型:{meta.get('type')} "
            f"传感器:{sensor} 决策:{result}"
        ),
    }

    add_knowledge([learn_record], category=category)

    print("💾 已写入本地知识库")
    print("==============================\n")


client.on_connect = on_connect
client.on_message = on_message

print("🚀 启动树莓派农业AI控制系统...")
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_forever()