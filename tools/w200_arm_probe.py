#!/usr/bin/env python3
"""Probe ARM-side W200 firmware references without assuming a single VMA map."""

from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import struct


TARGET_STRINGS = (
    "CCustomMidiBank_imp.c",
    "CPlayerManager_imp.c",
    "CSynthesizer_imp.c",
    r".\cxc1329035_G200_PHONE_Melinda\IAR-ARM7\inc\midi.c",
    "MThd",
    "MTrk",
    "_capMIDI",
    "_capCMIDI",
    "_capCompoundSound",
    "_capStreamSound",
    "DSP_RM_LOADMODULE_MIDI_PCM_ASR",
    "DSP_RM_LOADMODULE_MIDI_72",
    "AudioControl has received primitive %d from 0x%x unexpectedly.",
)
VMA_BASES = (
    0x00000000,
    0x40000000,
    0x41000000,
    0x42000000,
    0x43000000,
    0x44000000,
    0x44100000,
    0x44200000,
    0x44300000,
    0x44400000,
    0x44500000,
    0x44600000,
)
AUDIO_NEEDLES = (
    "AudioControl",
    "AudioContext",
    "IAudioControl",
    "R_REQ_AC",
    "R_Req_AC",
    "AC_DSP",
    "DSP_RM_LOADMODULE_MIDI_PCM_ASR",
    "DSP_RM_LOADMODULE_MIDI_72",
)


def find_all(data: bytes, needle: bytes) -> list[int]:
    offsets = []
    pos = 0
    while True:
        pos = data.find(needle, pos)
        if pos < 0:
            return offsets
        offsets.append(pos)
        pos += 1


def printable_window(data: bytes, offset: int, radius: int = 0x80) -> list[str]:
    start = max(0, offset - radius)
    end = min(len(data), offset + radius)
    chunk = data[start:end]
    strings = []
    current = bytearray()
    current_start = start
    for index, byte in enumerate(chunk):
        if 32 <= byte < 127:
            if not current:
                current_start = start + index
            current.append(byte)
        else:
            if len(current) >= 4:
                strings.append(f"{current_start:08x}: {current.decode('ascii', 'replace')}")
            current.clear()
    if len(current) >= 4:
        strings.append(f"{current_start:08x}: {current.decode('ascii', 'replace')}")
    return strings


def printable_ratio(data: bytes, start: int, end: int) -> float:
    if end <= start:
        return 0.0
    printable = sum(1 for byte in data[start:end] if byte in (0, 9, 10, 13) or 32 <= byte < 127)
    return printable / (end - start)


def pointer_hits(data: bytes, target_offset: int) -> list[dict]:
    hits = []
    for base in VMA_BASES:
        value = base + target_offset
        needle = struct.pack("<I", value)
        aligned = [offset for offset in find_all(data, needle) if offset % 4 == 0]
        if aligned:
            hits.append({"base": base, "value": value, "offsets": aligned})
    return hits


def aligned_words(data: bytes, start: int, end: int) -> list[int]:
    return [
        struct.unpack_from("<I", data, offset)[0]
        for offset in range(start + ((4 - start) % 4), max(start, end - 3), 4)
    ]


def arm_expand_imm12(imm12: int) -> int:
    imm8 = imm12 & 0xFF
    rotate = ((imm12 >> 8) & 0xF) * 2
    if rotate == 0:
        return imm8
    return ((imm8 >> rotate) | (imm8 << (32 - rotate))) & 0xFFFFFFFF


def arm_immediate_operations(data: bytes, ranges: list[tuple[int, int]]) -> list[dict]:
    operations = []
    op_names = {
        0b0010: "sub",
        0b0011: "rsb",
        0b0100: "add",
        0b1010: "cmp",
        0b1101: "mov",
    }
    for start, end in ranges:
        for offset in range(start + ((4 - start) % 4), min(end, len(data) - 3), 4):
            word = struct.unpack_from("<I", data, offset)[0]
            if word & 0x0E000000 != 0x02000000:
                continue
            opcode = (word >> 21) & 0xF
            name = op_names.get(opcode)
            if not name:
                continue
            imm = arm_expand_imm12(word & 0xFFF)
            rn = (word >> 16) & 0xF
            rd = (word >> 12) & 0xF
            operations.append(
                {
                    "offset": offset,
                    "word": word,
                    "op": name,
                    "rn": rn,
                    "rd": rd,
                    "imm": imm,
                }
            )
    return operations


def arm_code_score(data: bytes, start: int, end: int) -> dict:
    words = aligned_words(data, start, end)
    if not words:
        return {"score": 0.0, "words": 0}
    push = sum(1 for word in words if word & 0xFFFF0000 == 0xE92D0000)
    pop = sum(1 for word in words if word & 0xFFFF0000 == 0xE8BD0000)
    bx_lr = words.count(0xE12FFF1E)
    bl = sum(1 for word in words if word & 0x0F000000 == 0x0B000000)
    ldr_pc = sum(1 for word in words if word & 0x0E5F0000 == 0x041F0000)
    branch = sum(1 for word in words if word & 0x0F000000 == 0x0A000000)
    structural = push + pop + bx_lr + bl + ldr_pc
    return {
        "score": structural / len(words),
        "words": len(words),
        "push": push,
        "pop": pop,
        "bx_lr": bx_lr,
        "bl": bl,
        "ldr_pc": ldr_pc,
        "branch": branch,
    }


def thumb_code_score(data: bytes, start: int, end: int) -> dict:
    if end <= start:
        return {"score": 0.0, "halfwords": 0}
    halfwords = [
        struct.unpack_from("<H", data, offset)[0]
        for offset in range(start + (start % 2), max(start, end - 1), 2)
    ]
    if not halfwords:
        return {"score": 0.0, "halfwords": 0}
    push = sum(1 for hw in halfwords if hw & 0xFF00 == 0xB500)
    pop_pc = sum(1 for hw in halfwords if hw & 0xFF00 == 0xBD00)
    bx = sum(1 for hw in halfwords if hw & 0xFF87 == 0x4700)
    bl_prefix = sum(1 for hw in halfwords if hw & 0xF800 == 0xF000)
    branch = sum(1 for hw in halfwords if hw & 0xF000 == 0xD000 or hw & 0xF800 == 0xE000)
    ldr_literal = sum(1 for hw in halfwords if hw & 0xF800 == 0x4800)
    structural = push + pop_pc + bx + bl_prefix + ldr_literal
    return {
        "score": structural / len(halfwords),
        "halfwords": len(halfwords),
        "push": push,
        "pop_pc": pop_pc,
        "bx": bx,
        "bl_prefix": bl_prefix,
        "ldr_literal": ldr_literal,
        "branch": branch,
    }


def code_windows(data: bytes, window: int = 0x400, step: int = 0x200) -> list[dict]:
    items = []
    for start in range(0, len(data) - window + 1, step):
        end = start + window
        printable = printable_ratio(data, start, end)
        if printable > 0.65:
            continue
        arm = arm_code_score(data, start, end)
        thumb = thumb_code_score(data, start, end)
        best_mode = "arm" if arm["score"] >= thumb["score"] else "thumb"
        best_score = max(arm["score"], thumb["score"])
        if best_score >= 0.035:
            items.append(
                {
                    "start": start,
                    "end": end,
                    "best_mode": best_mode,
                    "best_score": best_score,
                    "printable_ratio": printable,
                    "arm": arm,
                    "thumb": thumb,
                }
            )
    return sorted(items, key=lambda item: item["best_score"], reverse=True)


def merge_windows(items: list[dict]) -> list[dict]:
    ordered = sorted(items, key=lambda item: (item["best_mode"], item["start"]))
    merged = []
    for item in ordered:
        if not merged or merged[-1]["best_mode"] != item["best_mode"] or item["start"] > merged[-1]["end"]:
            merged.append(
                {
                    "start": item["start"],
                    "end": item["end"],
                    "best_mode": item["best_mode"],
                    "window_count": 1,
                    "max_score": item["best_score"],
                    "avg_score": item["best_score"],
                }
            )
            continue
        current = merged[-1]
        current["end"] = max(current["end"], item["end"])
        current["window_count"] += 1
        current["max_score"] = max(current["max_score"], item["best_score"])
        current["avg_score"] += item["best_score"]
    for item in merged:
        item["avg_score"] /= item["window_count"]
    return sorted(merged, key=lambda item: (item["start"], item["best_mode"]))


def audio_string_hits(data: bytes) -> list[dict]:
    hits = []
    for text in AUDIO_NEEDLES:
        for offset in find_all(data, text.encode("ascii")):
            hits.append(
                {
                    "text": text,
                    "offset": offset,
                    "nearby_strings": printable_window(data, offset, radius=0x120),
                }
            )
    return sorted(hits, key=lambda item: item["offset"])


def audio_adjacent_code_ranges(data: bytes) -> list[dict]:
    candidates = []
    for start in (0x1EF000, 0x1F0000):
        end = min(len(data), start + 0x6000)
        if start < len(data):
            candidates.append(
                {
                    "start": start,
                    "end": end,
                    "arm": arm_code_score(data, start, end),
                    "thumb": thumb_code_score(data, start, end),
                    "printable_ratio": printable_ratio(data, start, end),
                    "nearby_audio_strings": [
                        hit
                        for hit in audio_string_hits(data)
                        if start <= hit["offset"] < end
                    ],
                }
            )
    return candidates


def primitive_diagnostics(data: bytes) -> list[dict]:
    diagnostics = []
    for offset in find_all(data, b"primitive"):
        start = offset
        while start > 0 and data[start - 1] not in (0, 10, 13):
            start -= 1
        end = offset
        while end < len(data) and data[end] not in (0, 10, 13):
            end += 1
        text = data[start:end].decode("ascii", "replace")
        diagnostics.append(
            {
                "offset": offset,
                "line_start": start,
                "text": text,
                "aligned_pointer_hits": pointer_hits(data, start),
            }
        )
    return diagnostics


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("main", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    data = args.main.read_bytes()
    windows = code_windows(data)
    audio_ranges = audio_adjacent_code_ranges(data)
    targets = []
    for text in TARGET_STRINGS:
        offsets = find_all(data, text.encode("ascii"))
        targets.append(
            {
                "text": text,
                "offsets": offsets,
                "aligned_pointer_hits": {
                    f"0x{offset:08x}": pointer_hits(data, offset) for offset in offsets[:8]
                },
                "nearby_strings": printable_window(data, offsets[0]) if offsets else [],
            }
        )

    report = {
        "main": {"path": str(args.main), "size": len(data)},
        "vma_bases_tested": list(VMA_BASES),
        "targets": targets,
        "audio_string_hits": audio_string_hits(data),
        "primitive_diagnostics": primitive_diagnostics(data),
        "code_windows": windows[:200],
        "code_islands": merge_windows(windows)[:200],
        "code_window_mode_counts": Counter(item["best_mode"] for item in windows),
        "audio_adjacent_code_ranges": audio_ranges,
        "audio_adjacent_arm_immediates": arm_immediate_operations(
            data, [(item["start"], item["end"]) for item in audio_ranges]
        ),
        "notes": [
            "No single VMA map is assumed.",
            "Pointer hits are word-aligned exact little-endian values only.",
            "Unaligned matches are deliberately ignored because MAIN has many accidental byte matches.",
        ],
    }
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(args.out)


if __name__ == "__main__":
    main()
