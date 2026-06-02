#!/usr/bin/env python3
"""Probe W200 sysparam/process symbol registry references."""

from __future__ import annotations

import argparse
from collections import defaultdict
import json
from pathlib import Path
import struct


PROCESS_BLOCK_START = 0x6E3700
PROCESS_BLOCK_END = 0x6E3C90
PROCESS_LIST_START = 0x6E371C
TARGET_NAMES = {
    "AVCR_Process",
    "AF_Process",
    "AudioControl_Process",
    "VIE_EncoderProcess",
    "CameraServer_Process",
    "DSPIF_INT_IntProcess",
    "LoadModuleStarterProcess",
}


def build_word_index(data: bytes) -> dict[int, list[int]]:
    index: dict[int, list[int]] = defaultdict(list)
    for offset in range(0, len(data) - 3, 4):
        value = struct.unpack_from("<I", data, offset)[0]
        if len(index[value]) < 256:
            index[value].append(offset)
    return index


def extract_names(data: bytes) -> list[dict]:
    names = []
    pos = PROCESS_LIST_START
    while pos < PROCESS_BLOCK_END:
        end = data.find(b"\0", pos, PROCESS_BLOCK_END)
        if end < 0:
            break
        if end > pos:
            text = data[pos:end].decode("ascii", "replace")
            if len(text) >= 2:
                names.append(
                    {
                        "index": len(names),
                        "offset": pos,
                        "relative_to_block": pos - PROCESS_LIST_START,
                        "text": text,
                    }
                )
        pos = end + 1
    return names


def thumb_prologue_near(data: bytes, offset: int, radius: int = 0x20) -> list[int]:
    hits = []
    start = max(0, offset - radius)
    end = min(len(data) - 1, offset + radius)
    for pos in range(start + (start % 2), end, 2):
        hw = struct.unpack_from("<H", data, pos)[0]
        if hw & 0xFF00 in (0xB500, 0xB400):
            hits.append(pos)
    return hits


def local_words(data: bytes, offset: int, radius: int = 0x20) -> list[dict]:
    start = max(0, offset - radius)
    start += (4 - start) % 4
    end = min(len(data) - 3, offset + radius)
    words = []
    for pos in range(start, end, 4):
        value = struct.unpack_from("<I", data, pos)[0]
        words.append({"offset": pos, "value": value})
    return words


def target_hits(data: bytes, word_index: dict[int, list[int]], names: list[dict]) -> list[dict]:
    by_name = []
    for item in names:
        if item["text"] not in TARGET_NAMES:
            continue
        values = [
            {"kind": "file_offset", "value": item["offset"]},
            {"kind": "relative_to_process_list", "value": item["relative_to_block"]},
            {"kind": "process_list_index", "value": item["index"]},
        ]
        hits = []
        for value_item in values:
            for offset in word_index.get(value_item["value"], []):
                hits.append(
                    {
                        "kind": value_item["kind"],
                        "value": value_item["value"],
                        "offset": offset,
                        "thumb_prologue_near": thumb_prologue_near(data, offset),
                        "nearby_words": local_words(data, offset),
                    }
                )
        by_name.append({**item, "hits": hits})
    return by_name


def symbol_dictionaries(data: bytes, names: list[dict]) -> list[dict]:
    # Dictionaries that contain several adjacent process-name relative offsets.
    rels = {item["relative_to_block"]: item["text"] for item in names}
    rel_values = set(rels)
    hits = []
    for offset in range(0, len(data) - 3, 4):
        value = struct.unpack_from("<I", data, offset)[0]
        if value not in rel_values:
            continue
        window = []
        for pos in range(max(0, offset - 0x40), min(len(data) - 3, offset + 0x80), 4):
            v = struct.unpack_from("<I", data, pos)[0]
            if v in rel_values:
                window.append({"offset": pos, "relative": v, "name": rels[v]})
        if len({entry["name"] for entry in window}) >= 3:
            hits.append({"offset": offset, "window": window})
    return hits[:200]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("main", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    data = args.main.read_bytes()
    names = extract_names(data)
    index = build_word_index(data)
    report = {
        "main": {"path": str(args.main), "size": len(data)},
        "process_block": {
            "start": PROCESS_BLOCK_START,
            "process_list_start": PROCESS_LIST_START,
            "end": PROCESS_BLOCK_END,
        },
        "process_names": names,
        "target_hits": target_hits(data, index, names),
        "symbol_dictionary_windows": symbol_dictionaries(data, names),
        "notes": [
            "The process list is part of the sysparam_translate_symbol string block.",
            "Hits are 32-bit word matches for relative offsets and list indices, not proof of function ownership.",
            "thumb_prologue_near marks candidate code/literal-pool adjacency for manual disassembly.",
        ],
    }
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(args.out)


if __name__ == "__main__":
    main()
