#!/usr/bin/env python3

import argparse
import hashlib
import struct
import sys
from pathlib import Path

MAGIC = 0x004D4D4D
VERSION = 1
SIGNATURE_SIZE = 20

HEADER_SIZE = 136
HEADER_SIGNATURE_SIZE = 20
DATA_START = HEADER_SIZE + HEADER_SIGNATURE_SIZE

HEADER_FORMAT = "<II"
BIN_FORMAT = "<IIIIIII4s"

def sha1(data):
    return hashlib.sha1(data).digest()

def generate_ota(input_file, output_file, start_addr, partition_size, compressed=False):

    firmware = Path(input_file).read_bytes()

    if not firmware:
        raise ValueError("CM4 firmware is empty")

    if not compressed and len(firmware) > partition_size:
        raise ValueError("CM4 firmware exceeds partition size")

    magic_ver = (VERSION << 24) | MAGIC

    bin_offset = DATA_START
    bin_length = len(firmware)

    sig_offset = bin_offset + bin_length
    sig_length = SIGNATURE_SIZE

    descriptor = struct.pack(
        BIN_FORMAT,
        bin_offset,
        start_addr,
        bin_length,
        partition_size,
        sig_offset,
        sig_length,
        1 if compressed else 0,
        b"\x00" * 4
    )

    header = struct.pack(HEADER_FORMAT, magic_ver, 1)

    header += descriptor

    header += b"\x00" * (3 * 32)

    assert len(header) == HEADER_SIZE

    header_checksum = sha1(header)

    firmware_checksum = sha1(firmware)

    ota_image = (header + header_checksum + firmware + firmware_checksum)

    Path(output_file).parent.mkdir(parents=True, exist_ok=True)

    Path(output_file).write_bytes(ota_image)

    print("\n========== MT7687 OTA ==============")
    print(f"Input firmware : {input_file}")
    print(f"Output OTA     : {output_file}")
    print(f"Magic/version  : 0x{magic_ver:08X}")
    print(f"Address        : 0x{start_addr:X}")
    print(f"Partition size : 0x{partition_size:X} ({partition_size})")
    print(f"Firmware size  : {bin_length}")
    print(f"Compressed     : {compressed}")
    print(f"Header SHA-1   : {header_checksum.hex()}")
    print(f"Firmware SHA-1 : {firmware_checksum.hex()}")
    print(f"OTA image size : {len(ota_image)}")
    print("====================================")

    verify_ota(ota_image)

    return ota_image

def verify_ota(image):

    if len(image) < DATA_START + SIGNATURE_SIZE:
        raise ValueError("OTA image too small")

    magic_ver, bin_count = struct.unpack_from(HEADER_FORMAT, image, 0)

    if (magic_ver & 0x00FFFFFF) != MAGIC:
        raise ValueError("Invalid FOTA magic")

    header = image[:HEADER_SIZE]
    header_sha = image[HEADER_SIZE:DATA_START]

    if sha1(header) != header_sha:
        raise ValueError("Header checksum failed")

    descriptor = struct.unpack_from(BIN_FORMAT, image, 8)

    (
        bin_offset,
        start_addr,
        bin_length,
        partition_length,
        sig_offset,
        sig_length,
        compressed,
        reserved
    ) = descriptor

    if sig_length != SIGNATURE_SIZE:
        raise ValueError("Invalid firmware signature length")

    if bin_offset < DATA_START:
        raise ValueError("Invalid firmware offset")

    if bin_offset + bin_length > len(image):
        raise ValueError("Firmware payload out of bounds")

    if sig_offset != bin_offset + bin_length:
        raise ValueError("Invalid firmware checksum offset")

    if sig_offset + sig_length != len(image):
        raise ValueError("Unexpected data after firmware checksum")

    if not compressed and bin_length > partition_length:
        raise ValueError("Firmware exceeds destination partition")

    firmware = image[bin_offset:bin_offset + bin_length]
    firmware_sha = image[sig_offset:sig_offset + sig_length]

    if sha1(firmware) != firmware_sha:
        raise ValueError("Firmware checksum failed")

def main():

    parser = argparse.ArgumentParser(description="MT7687 OTA image generator")

    parser.add_argument("-i", "--input", required=True, help="CM4 firmware binary")

    parser.add_argument("-o", "--output", default="mt7687_cm4_ota.bin")

    parser.add_argument("--address", default="0x79000", help="CM4 flash destination address")

    parser.add_argument("--partition-size", default="0xBF000", help="CM4 partition size")

    parser.add_argument("--compressed", action="store_true", help="Input is already compressed in the expected LZMA format")

    args = parser.parse_args()

    try:
        generate_ota(args.input, args.output, int(args.address, 0), int(args.partition_size, 0), args.compressed)

    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
