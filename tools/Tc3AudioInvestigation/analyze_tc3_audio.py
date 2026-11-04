"""Inspect a local TC3 MPS and compare its PCM stream with captured SPU DMA."""
import argparse
import json
import struct
from pathlib import Path


def inspect_mps(path):
    data = path.read_bytes()
    offset = 0x800
    chunks = []
    pictures = 0
    sequences = set()
    while offset + 64 <= len(data):
        fields = struct.unpack_from("<16I", data, offset)
        _, video_end, audio_offset, audio_size, packet_size = fields[:5]
        if not packet_size:
            break
        if not (64 <= video_end <= audio_offset and
                audio_offset + audio_size == packet_size and
                offset + packet_size <= len(data)):
            raise ValueError(f"Unexpected MPS packet layout at {offset:#x}")
        video = data[offset + 64:offset + video_end]
        pictures += video.count(b"\x00\x00\x01\x00")
        position = 0
        while True:
            position = video.find(b"\x00\x00\x01\xb3", position)
            if position < 0:
                break
            if position + 8 <= len(video):
                a, b, c, d = video[position + 4:position + 8]
                sequences.add(((a << 4) | (b >> 4), ((b & 15) << 8) | c, d & 15))
            position += 4
        chunks.append(data[offset + audio_offset:offset + audio_offset + audio_size])
        offset = (offset + packet_size + 0x7FF) & ~0x7FF
    return b"".join(chunks), pictures, sequences, len(chunks)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mps", type=Path)
    parser.add_argument("dma", type=Path)
    args = parser.parse_args()
    source, pictures, sequences, chunks = inspect_mps(args.mps)
    dma = args.dma.read_bytes()
    first = next((i for i, (a, b) in enumerate(zip(source, dma)) if a != b), None)
    mappings = []
    if first is not None:
        for offset in [first, first + 0x8000, first + 0x10000, len(dma) - 1024]:
            if offset + 128 > len(dma):
                continue
            match = source.find(dma[offset:offset + 128])
            mappings.append({
                "dma_byte": offset,
                "source_byte": match if match >= 0 else None,
                "source_minus_dma_seconds_at_48khz_stereo": (match - offset) / 192000 if match >= 0 else None,
            })
    print(json.dumps({
        "mps_chunks": chunks,
        "mpeg_sequence_width_height_frame_rate_code": sorted(sequences),
        "mpeg_picture_count": pictures,
        "video_duration_if_30fps": pictures / 30,
        "source_pcm_bytes": len(source),
        "source_audio_duration_if_48khz_stereo_s16": len(source) / 192000,
        "captured_dma_bytes": len(dma),
        "missing_source_bytes": max(0, len(source) - len(dma)),
        "trailing_capture_bytes": max(0, len(dma) - len(source)),
        "complete_source_prefix_matches": len(dma) >= len(source) and dma[:len(source)] == source,
        "first_mismatching_byte": first,
        "first_mismatch_source_seconds_if_48khz_stereo_s16": first / 192000 if first is not None else None,
        "sample_mappings": mappings,
    }, indent=2))


if __name__ == "__main__":
    main()
