import json
import ast


def parse(raw: str) -> dict:
    """
    解析 MQTT 原始数据。

    支持：
    1. 标准 JSON 字符串
    2. Python dict 字符串
    3. 简写字段 temp/hum/ds

    返回统一格式：
    {
        "device_id": "greenhouse_1",
        "temperature": 36,
        "humidity": 75,
        "soil_moisture": 22,
        "co2": 500
    }
    """

    raw = (raw or "").strip()

    try:
        data = json.loads(raw)
    except Exception:
        try:
            data = ast.literal_eval(raw)
        except Exception:
            return {}

    if not isinstance(data, dict):
        return {}

    return {
        "device_id": data.get("device_id", "greenhouse_1"),
        "temperature": data.get("temperature", data.get("temp")),
        "humidity": data.get("humidity", data.get("hum")),
        "soil_moisture": data.get("soil_moisture", data.get("ds")),
        "co2": data.get("co2"),
    }