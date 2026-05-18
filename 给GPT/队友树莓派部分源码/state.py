import json
import os
import time
from typing import Any

from config import COOLDOWN_SECONDS, SIGNIFICANT_CHANGE


STATE_FILE = "./runtime_state.json"


def _load_state() -> dict[str, Any]:
    if not os.path.exists(STATE_FILE):
        return {}

    try:
        with open(STATE_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}


def _save_state(state: dict[str, Any]) -> None:
    with open(STATE_FILE, "w", encoding="utf-8") as f:
        json.dump(state, f, ensure_ascii=False, indent=2)


def _to_float(value):
    try:
        if value is None:
            return None
        return float(value)
    except Exception:
        return None


def calc_delta(old_sensor: dict, new_sensor: dict) -> dict:
    """
    计算本次数据和上次决策时数据的变化量。
    """
    delta = {}

    for key in ["temperature", "humidity", "soil_moisture", "co2"]:
        old_v = _to_float(old_sensor.get(key))
        new_v = _to_float(new_sensor.get(key))

        if old_v is None or new_v is None:
            delta[key] = None
        else:
            delta[key] = round(new_v - old_v, 3)

    return delta


def is_significant_change(delta: dict) -> bool:
    """
    判断环境变化是否明显。
    """
    for key, threshold in SIGNIFICANT_CHANGE.items():
        value = delta.get(key)

        if value is None:
            continue

        if abs(value) >= threshold:
            return True

    return False


def should_decide(device_id: str, sensor: dict) -> tuple[bool, dict]:
    """
    返回：
    - 是否允许本轮调用 LLM 决策
    - 上次状态上下文
    """

    now = time.time()
    state = _load_state()
    info = state.get(device_id)

    if not info:
        return True, {
            "status": "first_decision",
            "message": "首次决策",
            "elapsed_seconds": None,
            "delta": {},
            "last_sensor": None,
            "last_result": None,
        }

    last_ts = info.get("last_decision_ts", 0)
    elapsed = now - last_ts

    last_sensor = info.get("last_sensor", {})
    last_result = info.get("last_result", {})
    delta = calc_delta(last_sensor, sensor)
    changed = is_significant_change(delta)

    context = {
        "status": "history_found",
        "elapsed_seconds": int(elapsed),
        "delta": delta,
        "significant_change": changed,
        "last_sensor": last_sensor,
        "last_result": last_result,
    }

    if elapsed < COOLDOWN_SECONDS and not changed:
        return False, context

    return True, context


def update_decision_state(device_id: str, sensor: dict, result: dict) -> None:
    """
    每次真正调用 LLM 后，保存当前状态。
    """
    state = _load_state()

    state[device_id] = {
        "last_decision_ts": time.time(),
        "last_sensor": sensor,
        "last_result": result,
    }

    _save_state(state)