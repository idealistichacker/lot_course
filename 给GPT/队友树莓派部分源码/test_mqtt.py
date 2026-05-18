import json
import time

import paho.mqtt.client as mqtt

from config import MQTT_BROKER, MQTT_PORT, SUB_TOPIC


client = mqtt.Client()
client.connect(MQTT_BROKER, MQTT_PORT, 60)


test_data = {
    "device_id": "greenhouse_1",
    "temperature": 36,
    "humidity": 75,
    "soil_moisture": 22,
    "co2": 500,
}


while True:
    payload = json.dumps(test_data, ensure_ascii=False)

    client.publish(SUB_TOPIC, payload)

    print("📤 已发送测试数据:")
    print(payload)

    time.sleep(10)