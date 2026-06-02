#!/usr/bin/env python3
import math
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parent
RES = ROOT / "res"


def vlq(value):
    out = [value & 0x7f]
    value >>= 7
    while value:
        out.insert(0, 0x80 | (value & 0x7f))
        value >>= 7
    return bytes(out)


def write_midi(path):
    events = bytearray()
    events += vlq(0) + bytes([0xC0, 80])
    events += vlq(0) + bytes([0x90, 69, 110])
    events += vlq(480) + bytes([0x80, 69, 0])
    events += vlq(0) + bytes([0xFF, 0x2F, 0x00])
    track = bytes(events)
    data = b"MThd" + struct.pack(">IHHH", 6, 0, 1, 480)
    data += b"MTrk" + struct.pack(">I", len(track)) + track
    path.write_bytes(data)


def write_wav(path):
    rate = 22050
    duration = 0.45
    frames = int(rate * duration)
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(rate)
        for i in range(frames):
            env = 1.0 - (i / float(frames))
            sample = int(math.sin(2.0 * math.pi * 440.0 * i / rate) *
                         18000 * env)
            wav.writeframesraw(struct.pack("<h", sample))


def main():
    RES.mkdir(parents=True, exist_ok=True)
    write_midi(RES / "tone.mid")
    write_wav(RES / "tone.wav")
    (RES / "probe.mp3").write_bytes(b"ID3\x04\x00\x00\x00\x00\x00\x00")
    (RES / "probe.amr").write_bytes(b"#!AMR\n")


if __name__ == "__main__":
    main()
