#!/usr/bin/env python3
"""Build a conservative CFG sketch from a raw TMS320C55x objdump listing."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re


DSP_PAYLOAD_OFFSET = 0x110
DSP_PAYLOAD_VMA = 0x5E8000
INSTRUCTION_RE = re.compile(r"^\s*([0-9a-f]+):\s+([0-9a-f ]+)\s+\t(.+)$", re.IGNORECASE)
PROCESS_NAMES = (
    "Dspsys_Idle_Process",
    "Dspsys_Timer_Process",
    "DSPIF_INT_IntProcess",
    "pcmif_ul_proc",
    "pcmif_dl_proc",
    "Pcmif_InterruptFunc",
    "MMA_Decoder",
    "MIDI_Synthesizer",
    "Mixer_Process",
    "Tonegen_Process",
    "crypto_proc",
    "ASR",
)
CONTROL_MNEMONICS = {"B", "BCC", "CALL", "RET", "RETI", "RETCC"}


def parse_int(text: str) -> int | None:
    text = text.strip().rstrip(",")
    if not text or text.startswith("AC"):
        return None
    try:
        return int(text, 0)
    except ValueError:
        try:
            return int(text, 10)
        except ValueError:
            return None


def parse_listing(path: Path) -> dict[int, dict]:
    instructions = {}
    for line in path.read_text(errors="replace").splitlines():
        match = INSTRUCTION_RE.match(line)
        if not match:
            continue
        address = int(match.group(1), 16)
        raw = "".join(match.group(2).split())
        text = match.group(3).strip()
        normalized = text[3:].strip() if text.startswith("|| ") else text
        mnemonic = normalized.split(None, 1)[0] if normalized else ""
        instructions[address] = {
            "address": address,
            "raw": raw,
            "text": text,
            "mnemonic": mnemonic,
            "is_unknown": normalized.startswith(".byte"),
            "source": str(path),
        }

    addresses = sorted(instructions)
    for index, address in enumerate(addresses):
        if index + 1 < len(addresses):
            size = addresses[index + 1] - address
        else:
            size = max(1, len(instructions[address]["raw"]) // 2)
        instructions[address]["size"] = size
        instructions[address]["fallthrough"] = address + size
    return instructions


def local_targets(target: int | None, module_size: int) -> list[dict]:
    if target is None:
        return []
    targets = []
    if 0 <= target < module_size:
        targets.append({"kind": "module_offset", "address": target})
    module_offset = DSP_PAYLOAD_OFFSET + target - DSP_PAYLOAD_VMA
    if 0 <= module_offset < module_size:
        targets.append({"kind": "inferred_payload_vma", "address": module_offset})
    return targets


def instruction_edges(instruction: dict, module_size: int) -> tuple[list[dict], bool]:
    mnemonic = instruction["mnemonic"]
    text = instruction["text"][3:].strip() if instruction["text"].startswith("|| ") else instruction["text"]
    parts = text.split(None, 1)
    operand_text = parts[1] if len(parts) > 1 else ""
    first_operand = operand_text.split(",", 1)[0]
    target = parse_int(first_operand)
    direct_targets = local_targets(target, module_size)
    edges = []
    stops = False

    if mnemonic in {"RET", "RETI"}:
        stops = True
    elif mnemonic == "RETCC":
        edges.append({"type": "conditional_return"})
        edges.append({"type": "fallthrough", "kind": "module_offset", "address": instruction["fallthrough"]})
    elif mnemonic == "B":
        for item in direct_targets:
            edges.append({"type": "branch", **item})
        stops = bool(direct_targets)
    elif mnemonic == "BCC":
        for item in direct_targets:
            edges.append({"type": "conditional", **item})
        edges.append({"type": "fallthrough", "kind": "module_offset", "address": instruction["fallthrough"]})
    elif mnemonic == "CALL":
        for item in direct_targets:
            edges.append({"type": "call", **item})
        edges.append({"type": "fallthrough", "kind": "module_offset", "address": instruction["fallthrough"]})
    else:
        edges.append({"type": "fallthrough", "kind": "module_offset", "address": instruction["fallthrough"]})
    return edges, stops


def walk_entry(instructions: dict[int, dict], module_size: int, entry: int, max_instructions: int) -> dict:
    visited = set()
    queue = [entry]
    edges = []
    unknown_count = 0
    control_count = 0

    while queue and len(visited) < max_instructions:
        address = queue.pop(0)
        while address in instructions and address not in visited and len(visited) < max_instructions:
            instruction = instructions[address]
            visited.add(address)
            unknown_count += int(instruction["is_unknown"])
            control_count += int(instruction["mnemonic"] in CONTROL_MNEMONICS)
            next_edges, stops = instruction_edges(instruction, module_size)
            for edge in next_edges:
                edge = {"from": address, **edge}
                edges.append(edge)
                if "address" not in edge:
                    continue
                target = edge["address"]
                if edge["kind"] == "module_offset" and target in instructions and target not in visited:
                    queue.append(target)
            if stops:
                break
            address = instruction["fallthrough"]

    return {
        "entry": entry,
        "visited_instruction_count": len(visited),
        "unknown_instruction_count": unknown_count,
        "control_instruction_count": control_count,
        "unknown_fraction": unknown_count / len(visited) if visited else 0.0,
        "min_address": min(visited) if visited else None,
        "max_address": max(visited) if visited else None,
        "edges": edges[:1000],
        "edge_count": len(edges),
        "truncated": len(visited) >= max_instructions,
    }


def scan_utf16_strings(module: bytes) -> list[dict]:
    strings = []
    for name in PROCESS_NAMES:
        raw = name.encode("utf-16le")
        offset = module.find(raw)
        if offset >= 0:
            strings.append({"name": name, "offset": offset, "size": len(raw)})
    return sorted(strings, key=lambda item: item["offset"])


def chunk_map(instructions: dict[int, dict], module: bytes, chunk_size: int) -> list[dict]:
    strings = scan_utf16_strings(module)
    chunks = []
    for start in range(0, len(module), chunk_size):
        end = min(len(module), start + chunk_size)
        items = [item for address, item in instructions.items() if start <= address < end]
        unknown = sum(1 for item in items if item["is_unknown"])
        control = sum(1 for item in items if item["mnemonic"] in CONTROL_MNEMONICS)
        string_hits = [
            item["name"]
            for item in strings
            if start <= item["offset"] < end or start < item["offset"] + item["size"] <= end
        ]
        unknown_fraction = unknown / len(items) if items else 1.0
        if string_hits:
            classification = "data_utf16_labels"
        elif not items:
            classification = "undecoded"
        elif unknown_fraction >= 0.35:
            classification = "data_likely"
        elif control:
            classification = "code_likely"
        else:
            classification = "mixed_or_table"
        chunks.append(
            {
                "start": start,
                "end": end,
                "instruction_count": len(items),
                "unknown_fraction": unknown_fraction,
                "control_instruction_count": control,
                "utf16_labels": string_hits,
                "classification": classification,
            }
        )
    return chunks


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("module", type=Path)
    parser.add_argument("asm", type=Path, nargs="+")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--entry", type=lambda value: int(value, 0), action="append", default=[])
    parser.add_argument("--chunk-size", type=lambda value: int(value, 0), default=0x100)
    parser.add_argument("--max-instructions", type=int, default=4000)
    args = parser.parse_args()

    module = args.module.read_bytes()
    instructions = {}
    for asm_path in args.asm:
        instructions.update(parse_listing(asm_path))
    entries = args.entry or [0x112, 0x115]
    report = {
        "module": {"path": str(args.module), "size": len(module)},
        "asm": {"paths": [str(path) for path in args.asm], "instruction_count": len(instructions)},
        "mapping": {
            "payload_offset": DSP_PAYLOAD_OFFSET,
            "inferred_payload_vma": DSP_PAYLOAD_VMA,
            "confidence": "static-inference",
        },
        "entries": [walk_entry(instructions, len(module), entry, args.max_instructions) for entry in entries],
        "utf16_strings": scan_utf16_strings(module),
        "chunks": chunk_map(instructions, module, args.chunk_size),
        "notes": [
            "Raw objdump is linear and may decode embedded tables as instructions.",
            "Branch targets are kept only when they can be mapped to module offsets or inferred payload VMA.",
        ],
    }
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(args.out)


if __name__ == "__main__":
    main()
