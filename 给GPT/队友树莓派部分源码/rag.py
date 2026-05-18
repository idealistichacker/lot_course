import json
import os
import re
import time
from pathlib import Path
from typing import Any

from config import KB_DIR


def _kb_file(category: str) -> Path:
    path = Path(KB_DIR) / category
    path.mkdir(parents=True, exist_ok=True)
    return path / "knowledge.jsonl"


def _record_to_text(record: dict[str, Any]) -> str:
    if "text" in record:
        return str(record["text"])

    return json.dumps(record, ensure_ascii=False)


def _tokenize(text: str) -> set[str]:
    """
    简单中文/英文混合检索。
    不依赖任何外部模型，适合树莓派。
    """
    text = str(text).lower()

    parts = re.findall(r"[\u4e00-\u9fff]+|[a-zA-Z0-9_.]+", text)

    tokens = set()

    for part in parts:
        tokens.add(part)

        # 中文片段做2字组合，提高检索效果
        if re.fullmatch(r"[\u4e00-\u9fff]+", part):
            for i in range(len(part) - 1):
                tokens.add(part[i:i + 2])

    return tokens


def _score(query_text: str, doc_text: str) -> float:
    q = _tokenize(query_text)
    d = _tokenize(doc_text)

    if not q or not d:
        return 0.0

    overlap = q & d
    return len(overlap) / max(len(q), 1)


def add_knowledge(items, category: str = "general") -> None:
    """
    写入分类知识库。

    items 可以是：
    - list[str]
    - list[dict]
    """

    file_path = _kb_file(category)

    with open(file_path, "a", encoding="utf-8") as f:
        for item in items:
            if isinstance(item, dict):
                record = item
            else:
                record = {"text": str(item)}

            record.setdefault("created_at", time.strftime("%Y-%m-%d %H:%M:%S"))
            record.setdefault("category", category)

            f.write(json.dumps(record, ensure_ascii=False) + "\n")


def _load_records(category: str) -> list[dict[str, Any]]:
    file_path = _kb_file(category)

    if not file_path.exists():
        return []

    records = []

    with open(file_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()

            if not line:
                continue

            try:
                records.append(json.loads(line))
            except Exception:
                continue

    return records


def query(q: str, category: str = "general", top_k: int = 5) -> list[str]:
    """
    从当前分类 + general 分类里检索知识。
    """

    records = []

    records.extend(_load_records(category))

    if category != "general":
        records.extend(_load_records("general"))

    scored = []

    for record in records:
        text = _record_to_text(record)
        score = _score(q, text)

        if score > 0:
            scored.append((score, text))

    scored.sort(key=lambda x: x[0], reverse=True)

    return [text for _, text in scored[:top_k]]


def list_knowledge(category: str = "general", limit: int = 50) -> list[dict[str, Any]]:
    records = _load_records(category)
    return records[-limit:]


def count_knowledge(category: str = "general") -> int:
    return len(_load_records(category))