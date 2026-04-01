# 本地假后端（替代树莓派）

## 作用
- 接收 ESP32 的 HTTP POST 数据
- 校验字段完整性
- 返回标准 ACK，便于你验证重试与成功逻辑

## 启动
1. 在该目录创建并激活 Python 虚拟环境（可选）。
2. 安装依赖：
   - pip install -r requirements.txt
3. 启动服务：
   - uvicorn server:app --host 0.0.0.0 --port 8000

## 测试
- 健康检查：GET /health
- 上传接口：POST /upload

## 常见问题
- 若 ESP32 无法访问电脑服务：
  - 确保电脑与 ESP32 在同一热点网段
  - 放行 8000 端口防火墙
  - 使用电脑在热点内网的 IPv4 地址填入 UPLOAD_URL
