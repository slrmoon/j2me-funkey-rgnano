#!/usr/bin/env python3
"""Extract reproducible W200 audio firmware facts from MAIN and FS images."""

from __future__ import annotations

import argparse
import binascii
import hashlib
import json
import os
from pathlib import Path
import struct
import zlib


DSP_HEADER = bytes.fromhex("01 00 00 00 00 80 00 00 80 00 00 00 80 80 01 ca")
DSP_PAYLOAD_OFFSET = 0x110
DSP_PAYLOAD_VMA = 0x5E8000
DSP_NAMES = (
    "MIDI_PCM_ASR",
    "Phone_Audio",
    "Phone_Audio_TTY",
    "Phone_Audio_PTT",
    "AMR_ASR",
    "AAC",
    "MP3",
    "Video_H263",
)
SAMPLE_RATES = (8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000)
DSP_PROCESSES = (
    "Dspsys_Idle_Process",
    "Dspsys_Timer_Process",
    "DSPIF_INT_IntProcess",
    "pcmif_ul_proc",
    "pcmif_dl_proc",
    "Pcmif_InterruptFunc",
    "MMA_Decoder",
    "MIDI_Synthesizer",
    "voicecall_tx_proc",
    "voicecall_rx_proc",
    "DICT_Control",
    "Mixer_Process",
    "Tonegen_Process",
    "Dspsys_Dbgprt_Proc",
    "crypto_proc",
    "ASR",
    "VCRV_Process",
    "vcra_tx_proc",
)
ARM_DSP_DIAGNOSTICS = (
    "DSP_RM_LOADMODULE_PHONE_AUDIO",
    "DSP_RM_LOADMODULE_PHONE_AUDIO_TTY",
    "DSP_RM_LOADMODULE_PHONE_AUDIO_PTT",
    "DSP_RM_LOADMODULE_MIDI_PCM_ASR",
    "DSP_RM_LOADMODULE_MP3",
    "DSP_RM_LOADMODULE_AMR_ASR",
    "DSP_RM_LOADMODULE_AAC",
    "DSP_RM_LOADMODULE_WMA",
    "DSP_RM_LOADMODULE_VIDEO",
    "DSP_RM_LOADMODULE_MIDI_72",
    "AUC ERROR: Timed out while waiting on R_REQ_AC_DSP_POSTBOOT",
    "DSP LOADER: ERROR DSP Version responder TIMED OUT!",
    "DSP_Loader: First R_Req_AC_DSP_PreBoot didn't work!",
    "No DSP code found in Flash/failed to download code, releasing MegaStar",
    "DSP Loader: Interrupts disabled",
    "DSP_RM/TryReplace: Failed to replace module!",
    "DSP_RM: DSP_LOADER_ABORT_RESPONSE_TIMEOUT",
    "DSP_Loader: R_Req_AC_DSP_PostBoot didn't return OK!",
    "DSP_Loader: R_Req_AC_DSP_PreBoot didn't work!",
)
ARM_DSP_SOURCE_LABELS = (
    "r_dsp_rm_dsp_loader.c",
    "r_af_audio_codecs.c",
    "r_avcr_audio_codecs.c",
    "r_mma_audio_codecs.c",
    "r_sed_audio_codecs.c",
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def find_all(data: bytes, needle: bytes) -> list[int]:
    offsets = []
    pos = 0
    while True:
        pos = data.find(needle, pos)
        if pos < 0:
            return offsets
        offsets.append(pos)
        pos += 1


def parse_smf(data: bytes, offset: int) -> dict | None:
    if data[offset : offset + 4] != b"MThd" or offset + 14 > len(data):
        return None
    header_size, midi_format, track_count, division = struct.unpack_from(">IHHH", data, offset + 4)
    if header_size != 6 or midi_format > 2 or not track_count:
        return None
    pos = offset + 8 + header_size
    for _ in range(track_count):
        if data[pos : pos + 4] != b"MTrk" or pos + 8 > len(data):
            return None
        size = struct.unpack_from(">I", data, pos + 4)[0]
        pos += 8 + size
        if pos > len(data):
            return None
    return {
        "offset": offset,
        "format": midi_format,
        "tracks": track_count,
        "division": division,
        "size": pos - offset,
    }


def scan_smf(data: bytes) -> list[dict]:
    return [
        parsed
        for offset in find_all(data, b"MThd")
        if (parsed := parse_smf(data, offset)) is not None
    ]


def scan_pitch_table(module: bytes) -> dict | None:
    pattern = struct.pack("<6H", 22, 23, 24, 26, 27, 29)
    start = module.find(pattern)
    if start < 0:
        return None
    values = []
    offset = start
    while offset + 2 <= len(module):
        value = struct.unpack_from("<H", module, offset)[0]
        if values and value <= values[-1]:
            break
        values.append(value)
        offset += 2
    ratios = [values[index + 12] / values[index] for index in range(len(values) - 12)]
    return {
        "offset": start,
        "count": len(values),
        "first": values[:16],
        "last": values[-16:],
        "octave_ratio_min": min(ratios),
        "octave_ratio_max": max(ratios),
        "octave_ratio_mean": sum(ratios) / len(ratios),
    }


def scan_rate_table(module: bytes) -> dict | None:
    pattern = b"".join(struct.pack("<I", rate) for rate in SAMPLE_RATES)
    offset = module.find(pattern)
    if offset < 0:
        return None
    return {"offset": offset, "values": list(SAMPLE_RATES)}


def decode_c55x_vectors(module: bytes) -> dict:
    """Decode the 32 eight-byte C55x interrupt vectors after the module header."""
    vectors = []
    for index in range(32):
        offset = 0x10 + index * 8
        raw = module[offset : offset + 8]
        vectors.append(
            {
                "index": index,
                "offset": offset,
                "raw": raw.hex(),
                "isr_address": int.from_bytes(raw[1:4], "little"),
                "inferred_payload_relative_offset": int.from_bytes(raw[1:4], "little") - DSP_PAYLOAD_VMA,
                "inferred_module_offset": DSP_PAYLOAD_OFFSET + int.from_bytes(raw[1:4], "little") - DSP_PAYLOAD_VMA,
            }
        )
    return {
        "offset": 0x10,
        "count": len(vectors),
        "slot_size": 8,
        "payload_offset": DSP_PAYLOAD_OFFSET,
        "inferred_payload_vma": DSP_PAYLOAD_VMA,
        "vectors": vectors,
    }


def scan_dsp_processes(module: bytes) -> list[dict]:
    processes = []
    for name in DSP_PROCESSES:
        offset = module.find(name.encode("utf-16le"))
        if offset >= 0:
            processes.append({"name": name, "offset": offset})
    return sorted(processes, key=lambda item: item["offset"])


def scan_ascii_catalog(data: bytes, strings: tuple[str, ...]) -> list[dict]:
    entries = []
    for text in strings:
        needle = text.encode("ascii")
        for offset in find_all(data, needle):
            end = offset + len(needle)
            if end < len(data) and (data[end : end + 1].isalnum() or data[end] == ord("_")):
                continue
            entries.append({"text": text, "offset": offset})
    return sorted(entries, key=lambda item: item["offset"])


def local_zip_records(data: bytes) -> list[dict]:
    records = []
    for offset in find_all(data, b"PK\x03\x04"):
        if offset + 30 > len(data):
            continue
        _, _, flag, method, _, _, _, packed_size, size, name_len, extra_len = struct.unpack_from(
            "<IHHHHHIIIHH", data, offset
        )
        if name_len > 4096 or offset + 30 + name_len + extra_len > len(data):
            continue
        name = data[offset + 30 : offset + 30 + name_len].decode("utf-8", "replace")
        records.append(
            {
                "offset": offset,
                "name": name,
                "method": method,
                "flag": flag,
                "packed_size": packed_size,
                "size": size,
                "data_offset": offset + 30 + name_len + extra_len,
            }
        )
    return records


def central_zip_archives(data: bytes) -> list[dict]:
    archives = []
    for end_offset in find_all(data, b"PK\x05\x06"):
        if end_offset + 22 > len(data):
            continue
        _, _, _, _, count, directory_size, _, _ = struct.unpack_from("<IHHHHIIH", data, end_offset)
        directory_offset = end_offset - directory_size
        if directory_offset < 0 or data[directory_offset : directory_offset + 4] != b"PK\x01\x02":
            continue
        entries = []
        pos = directory_offset
        for _ in range(count):
            if data[pos : pos + 4] != b"PK\x01\x02":
                break
            fields = struct.unpack_from("<IHHHHHHIIIHHHHHII", data, pos)
            _, _, _, flag, method, _, _, crc, packed_size, size, name_len, extra_len, comment_len, _, _, _, relative = fields
            name = data[pos + 46 : pos + 46 + name_len].decode("utf-8", "replace")
            entries.append(
                {
                    "name": name,
                    "method": method,
                    "flag": flag,
                    "crc32": crc,
                    "packed_size": packed_size,
                    "size": size,
                    "relative_offset": relative,
                }
            )
            pos += 46 + name_len + extra_len + comment_len
        archives.append({"directory_offset": directory_offset, "end_offset": end_offset, "entries": entries})
    return archives


def extract_sound_bins(data: bytes, out_dir: Path) -> list[dict]:
    local_records = local_zip_records(data)
    archives = central_zip_archives(data)
    result = []
    out_dir.mkdir(parents=True, exist_ok=True)
    for archive_index, archive in enumerate(archives):
        for entry in archive["entries"]:
            if os.path.basename(entry["name"]).lower() != "sound.bin":
                continue
            candidates = [
                item
                for item in local_records
                if item["name"] == entry["name"] and item["offset"] < archive["directory_offset"]
            ]
            if not candidates:
                continue
            local = max(candidates, key=lambda item: item["offset"])
            descriptor = data.find(b"PK\x07\x08", local["data_offset"])
            if descriptor < 0:
                continue
            compressed = data[local["data_offset"] : descriptor]
            try:
                payload = compressed if entry["method"] == 0 else zlib.decompress(compressed, -15)
            except zlib.error:
                continue
            output = out_dir / f"archive-{archive_index:02d}-sound.bin"
            output.write_bytes(payload)
            result.append(
                {
                    "archive": archive_index,
                    "offset": local["offset"],
                    "output": str(output),
                    "size": len(payload),
                    "sha256": sha256(payload),
                    "riff_offsets": find_all(payload, b"RIFF"),
                    "descriptor_crc32": f"{entry['crc32']:08x}",
                    "actual_crc32": f"{binascii.crc32(payload) & 0xffffffff:08x}",
                }
            )
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("main", type=Path, help="W200 MAIN .mbn image")
    parser.add_argument("fs", type=Path, help="W200 FS .fbn image")
    parser.add_argument("--out", type=Path, required=True, help="output directory")
    args = parser.parse_args()

    main_data = args.main.read_bytes()
    fs_data = args.fs.read_bytes()
    args.out.mkdir(parents=True, exist_ok=True)

    dsp_offsets = find_all(main_data, DSP_HEADER)
    modules = []
    for index, (name, offset) in enumerate(zip(DSP_NAMES, dsp_offsets)):
        end = dsp_offsets[index + 1] if index + 1 < len(dsp_offsets) else None
        module = main_data[offset:end] if end is not None else main_data[offset : offset + 0x10000]
        modules.append(
            {
                "name": name,
                "offset": offset,
                "next_module_offset": end,
                "vectors": decode_c55x_vectors(module),
                "processes": scan_dsp_processes(module),
            }
        )
    midi_start = dsp_offsets[0]
    midi_end = main_data.find(b"No DSP code found in Flash", midi_start)
    midi_module = main_data[midi_start:midi_end]
    midi_path = args.out / "W200_R4JA011_DSP_MIDI_PCM_ASR.bin"
    midi_path.write_bytes(midi_module)

    report = {
        "main": {"path": str(args.main), "size": len(main_data), "sha256": sha256(main_data)},
        "fs": {"path": str(args.fs), "size": len(fs_data), "sha256": sha256(fs_data)},
        "standard_bank_signatures": {
            "main": {key: len(find_all(main_data, value)) for key, value in {"sfbk": b"sfbk", "DLS ": b"DLS ", "wvpl": b"wvpl", "ptbl": b"ptbl", "lins": b"lins", "lrgn": b"lrgn"}.items()},
            "fs": {key: len(find_all(fs_data, value)) for key, value in {"sfbk": b"sfbk", "DLS ": b"DLS ", "wvpl": b"wvpl", "ptbl": b"ptbl", "lins": b"lins", "lrgn": b"lrgn"}.items()},
        },
        "dsp_modules": modules,
        "midi_module": {
            "path": str(midi_path),
            "offset": midi_start,
            "size": len(midi_module),
            "sha256": sha256(midi_module),
            "pitch_table": scan_pitch_table(midi_module),
            "sample_rate_table": scan_rate_table(midi_module),
            "vectors": decode_c55x_vectors(midi_module),
            "processes": scan_dsp_processes(midi_module),
        },
        "shared_audio_tables": {
            "pitch_table_main_offsets": find_all(main_data, struct.pack("<6H", 22, 23, 24, 26, 27, 29)),
            "sample_rate_table_main_offsets": find_all(
                main_data, b"".join(struct.pack("<I", rate) for rate in SAMPLE_RATES)
            ),
        },
        "arm_dsp_loader": {
            "diagnostics": scan_ascii_catalog(main_data, ARM_DSP_DIAGNOSTICS),
            "source_labels": scan_ascii_catalog(main_data, ARM_DSP_SOURCE_LABELS),
        },
        "valid_smf": {"main": scan_smf(main_data), "fs": scan_smf(fs_data)},
        "embedded_zip_archive_count": len(central_zip_archives(fs_data)),
        "sound_bins": extract_sound_bins(fs_data, args.out / "sound-bins"),
    }
    report_path = args.out / "deep-scan.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(report_path)


if __name__ == "__main__":
    main()
