# ESP32 固件（无树莓派阶段）

## 目标
- 定时唤醒读取 DHT11（GPIO2）
- 计算露点温度
- 以 JSON 通过 HTTP POST 上传到临时后端
- 进入深度睡眠降低功耗

## 依赖库（Arduino IDE）
- DHT sensor library by Adafruit
- Adafruit Unified Sensor

## 使用步骤
1. 将 secrets.example.h 复制为 secrets.h。
2. 填入 Wi-Fi 和上传 URL。
3. 如需修改采样周期，在 secrets.h 中设置 WAKE_INTERVAL_MIN（单位：分钟）。
4. 打开 main.ino，选择 ESP32-S3 对应板型。
5. 烧录并打开串口监视器（115200）。

## 串口关键日志
- DHT11 OK: 温湿度读取成功
- WiFi connected: 网络连通
- Upload success: 上传成功
- Entering deep sleep: 本轮结束

## 说明
- 默认 ENABLE_CAMERA=0，仅跑温湿度链路。
- 等你准备好摄像头链路后，把 ENABLE_CAMERA 改成 1，并安装相机相关依赖。

## Wi-Fi 连接失败排查
- 手机热点必须是 2.4GHz（ESP32-S3 不连 5GHz 热点）。
- 热点安全模式建议用 WPA2-Personal。
- 如果 SSID 含中文或特殊字符，先临时改成纯英文测试。
- 串口会打印扫描到的网络列表和状态码，可据此判断是找不到 SSID 还是鉴权失败。
