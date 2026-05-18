from datetime import datetime, timezone
from typing import Any, Dict

from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse

app = FastAPI(title="ESP32 Mock Receiver", version="0.1.0")


def validate_payload(data: Dict[str, Any]) -> Dict[str, Any]:
    required = [
        "device_id",
        "fw_version",
        "seq",
        "temp_c",
        "rh_pct",
        "dew_point_c",
        "wifi_rssi",
        "sensor_ok",
        "camera_ok",
    ]

    missing = [k for k in required if k not in data]
    return {
        "ok": len(missing) == 0,
        "missing": missing,
    }


@app.get("/health")
async def health() -> Dict[str, str]:
    return {"status": "ok"}


@app.post("/upload")
async def upload(request: Request):
    try:
        data = await request.json()
    except Exception:
        return JSONResponse(status_code=400, content={"ok": False, "error": "invalid_json"})

    check = validate_payload(data)
    now = datetime.now(timezone.utc).isoformat()

    print("\n=== Incoming packet ===")
    print(f"time={now}")
    print(f"device={data.get('device_id')} seq={data.get('seq')} sensor_ok={data.get('sensor_ok')} camera_ok={data.get('camera_ok')}")
    print(f"temp={data.get('temp_c')} rh={data.get('rh_pct')} dew={data.get('dew_point_c')} rssi={data.get('wifi_rssi')}")

    if not check["ok"]:
        return JSONResponse(
            status_code=422,
            content={
                "ok": False,
                "error": "missing_fields",
                "missing": check["missing"],
                "server_time": now,
            },
        )

    return {
        "ok": True,
        "server_time": now,
        "ack_seq": data.get("seq"),
        "next_action": "continue_sampling",
    }
