#!/usr/bin/env python3
"""Native TXW81x application and compressed-OTA image packer.

Portions of the CRC and HGIC/Taixin SPI-header implementation are adapted from
I-AM-ENGINEER/RNode_Halow_Firmware, pack/prepare_firmware.py (MIT License,
Copyright (c) 2026 I-AM-ENGINEER). See ATTRIBUTION.md.

This tool does not contain or redistribute Taixin executables, SDK sources, or
loader binaries. The OTA command consumes loader_compress.bin from an existing
SDK/OpenTXW81X checkout.
"""
from __future__ import annotations

import argparse
import configparser
import dataclasses
import datetime as dt
import hashlib
import json
import lzma
import os
import re
import struct
import sys
from pathlib import Path
from typing import Iterable, Mapping, Sequence

BOOT_STRUCT = struct.Struct("<HBBIIIIHHHHHH")
FW_STRUCT = struct.Struct("<BBIIIHBIHH")
COMPRESS_DESCRIPTOR_MAGIC = 0xA1B2C3D4
DEFAULT_PARAM_IMAGE_OFFSET = 0x200
DEFAULT_PARAM_SOURCE_OFFSET = 0x210
DEFAULT_COMPRESS_DICT_SIZE = 32 * 1024
CODE_ALIGNMENT = 0x200
CODE_POST_ALIGNMENT_BYTES = 16  # twelve zeroes and the 4-byte CRC32 complement


class FormatError(ValueError):
    """Raised when an input image or configuration is structurally invalid."""


def parse_int(value: str | int) -> int:
    """Parse Taixin INI integers, which are hexadecimal even without ``0x``."""
    if isinstance(value, int):
        return value
    text = value.strip()
    if not text:
        return 0
    return int(text, 16)


def crc16_modbus(data: bytes | bytearray | memoryview) -> int:
    """CRC-16/MODBUS used by the Taixin/HGIC image structures."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc >> 1) ^ 0xA001) & 0xFFFF if crc & 1 else crc >> 1
    return crc


def crc32_hgic(data: bytes | bytearray | memoryview) -> int:
    """Reflected CRC-32 used by HGIC firmware headers."""
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc >> 1) ^ 0xEDB88320) & 0xFFFFFFFF if crc & 1 else crc >> 1
    return (~crc) & 0xFFFFFFFF


def with_crc16(data: bytes) -> bytes:
    return data + struct.pack("<H", crc16_modbus(data))


def parse_build_date(value: str | None) -> int:
    """Return Taixin date encoding: ``year << 16 | month << 8 | day``."""
    if value:
        date = dt.date.fromisoformat(value)
    elif os.environ.get("SOURCE_DATE_EPOCH"):
        date = dt.datetime.fromtimestamp(int(os.environ["SOURCE_DATE_EPOCH"]), tz=dt.timezone.utc).date()
    else:
        date = dt.datetime.now(dt.timezone.utc).date()
    return (date.year << 16) | (date.month << 8) | date.day


def load_makecode_ini(path: Path) -> tuple[dict[str, str], dict[str, str]]:
    parser = configparser.ConfigParser(
        interpolation=None,
        delimiters=("=",),
        comment_prefixes=("#", ";"),
        inline_comment_prefixes=("#", ";"),
        strict=False,
    )
    parser.optionxform = str
    last_error: UnicodeDecodeError | None = None
    for encoding in ("utf-8-sig", "cp1251", "latin-1"):
        try:
            parser.read(path, encoding=encoding)
            last_error = None
            break
        except UnicodeDecodeError as exc:
            last_error = exc
    if last_error:
        raise FormatError(f"cannot decode INI file: {path}") from last_error
    if "COMMON" not in parser or "SPI" not in parser:
        raise FormatError("makecode.ini must contain [COMMON] and [SPI]")
    return dict(parser["COMMON"]), dict(parser["SPI"])


def _ihex_record(line: str, line_number: int) -> tuple[int, int, bytes]:
    text = line.strip()
    if not text:
        raise FormatError(f"empty Intel HEX record at line {line_number}")
    if not text.startswith(":"):
        raise FormatError(f"Intel HEX line {line_number} does not start with ':'")
    try:
        raw = bytes.fromhex(text[1:])
    except ValueError as exc:
        raise FormatError(f"invalid Intel HEX digits at line {line_number}") from exc
    if len(raw) < 5 or len(raw) != raw[0] + 5:
        raise FormatError(f"invalid Intel HEX length at line {line_number}")
    if sum(raw) & 0xFF:
        raise FormatError(f"Intel HEX checksum failure at line {line_number}")
    address = (raw[1] << 8) | raw[2]
    return address, raw[3], raw[4:-1]


def ihex_to_binary(text: str) -> tuple[int, bytes]:
    """Implement the BinScript HEX-to-binary remap with 0xFF gap filling."""
    upper = 0
    data_records: list[tuple[int, bytes]] = []
    eof_seen = False
    for line_number, line in enumerate(text.splitlines(), 1):
        if not line.strip():
            continue
        address, record_type, payload = _ihex_record(line, line_number)
        if record_type == 0x00:
            absolute = upper + address
            data_records.append((absolute, payload))
        elif record_type == 0x01:
            eof_seen = True
            break
        elif record_type == 0x02:
            if len(payload) != 2:
                raise FormatError(f"bad extended-segment record at line {line_number}")
            upper = int.from_bytes(payload, "big") << 4
        elif record_type == 0x04:
            if len(payload) != 2:
                raise FormatError(f"bad extended-linear record at line {line_number}")
            upper = int.from_bytes(payload, "big") << 16
        elif record_type in (0x03, 0x05):
            continue
        else:
            raise FormatError(f"unsupported Intel HEX record type 0x{record_type:02X} at line {line_number}")
    if not eof_seen:
        raise FormatError("Intel HEX EOF record is missing")
    if not data_records:
        raise FormatError("Intel HEX contains no data records")
    start = min(address for address, _ in data_records)
    end = max(address + len(payload) for address, payload in data_records)
    output = bytearray(b"\xFF" * (end - start))
    occupied: dict[int, int] = {}
    for address, payload in data_records:
        offset = address - start
        for index, byte in enumerate(payload):
            absolute = address + index
            previous = occupied.get(absolute)
            if previous is not None and previous != byte:
                raise FormatError(f"conflicting Intel HEX data at 0x{absolute:08X}")
            occupied[absolute] = byte
            output[offset + index] = byte
    return start, bytes(output)


def load_code(path: Path) -> tuple[int | None, bytes]:
    if path.suffix.lower() in {".hex", ".ihex"}:
        return ihex_to_binary(path.read_text(encoding="ascii"))
    return None, path.read_bytes()


def extract_parameter(
    code: bytes,
    parameter_cfg: bytes,
    *,
    length_offset: int,
    source_offset: int = DEFAULT_PARAM_SOURCE_OFFSET,
) -> bytes:
    if length_offset < 0 or length_offset + 2 > len(code):
        raise FormatError(f"parameter length offset 0x{length_offset:X} is outside code")
    length = struct.unpack_from("<H", code, length_offset)[0]
    if length < 2:
        raise FormatError(f"invalid parameter length: 0x{length:X}")
    if source_offset + length > len(parameter_cfg):
        raise FormatError(
            f"parameter source needs 0x{length:X} bytes at 0x{source_offset:X}, "
            f"but file is only 0x{len(parameter_cfg):X} bytes"
        )
    parameter = bytearray(parameter_cfg[source_offset : source_offset + length])
    struct.pack_into("<H", parameter, 0, length)
    return bytes(parameter)


def finish_compiler_code(raw_code: bytes, parameter: bytes, *, parameter_offset: int) -> bytes:
    """Apply the Taixin SPI packager's missing post-link transformations.

    BinScript extracts a parameter block from parameter.bincfg. Taixin's
    makecode.exe also replaces the parameter reservation *inside the code*
    before emitting the SPI image, then pads the code to a 512-byte boundary
    plus 16 bytes, with a complemented CRC32 in the final four bytes.

    This was reconstructed from July and October 2026 WSL outputs, including
    their embedded parameters, padded lengths and trailing CRC32 values. The
    function accepts raw compiler output, NOT code extracted from an already
    packaged APP.bin (which has been transformed).
    """
    if parameter_offset < 0 or parameter_offset + len(parameter) > len(raw_code):
        raise FormatError("embedded parameter reservation extends beyond compiler code")
    if len(parameter) < 2 or struct.unpack_from("<H", parameter)[0] != len(parameter):
        raise FormatError("invalid embedded parameter length")

    code = bytearray(raw_code)
    code[parameter_offset : parameter_offset + len(parameter)] = parameter
    rounded = ((len(code) + CODE_ALIGNMENT - 1) // CODE_ALIGNMENT) * CODE_ALIGNMENT
    final_size = rounded + CODE_POST_ALIGNMENT_BYTES
    padding_length = final_size - len(code) - 4
    if padding_length < 0:
        raise FormatError("invalid code padding length")
    code.extend(b"\x00" * padding_length)
    code.extend(struct.pack("<I", (~crc32_hgic(code)) & 0xFFFFFFFF))
    return bytes(code)


def check_finished_code(code: bytes, parameter: bytes, *, parameter_offset: int) -> dict[str, bool]:
    """Validate Taixin's embedded parameters, padded code and CRC32 trailer."""
    return {
        "embedded_parameter": (
            parameter_offset >= 0
            and parameter_offset + len(parameter) <= len(code)
            and code[parameter_offset : parameter_offset + len(parameter)] == parameter
        ),
        "code_512_alignment_plus_16": len(code) % CODE_ALIGNMENT == CODE_POST_ALIGNMENT_BYTES,
        "code_trailing_crc32_complement": (
            len(code) >= 4
            and struct.unpack_from("<I", code, len(code) - 4)[0]
            == ((~crc32_hgic(code[:-4])) & 0xFFFFFFFF)
        ),
    }


def _wire_mode_bits(value: int) -> int:
    mapping = {1: 0, 2: 1, 4: 2}
    if value not in mapping:
        raise FormatError(f"unsupported SPI wire mode: {value}")
    return mapping[value]


def _parse_spi_sequence(raw_hex: str) -> bytes:
    raw = bytes.fromhex(raw_hex.strip())
    if len(raw) < 3:
        raise FormatError(f"SPI special sequence is too short: {raw_hex!r}")
    encoded_length = 3 + raw[2]
    if len(raw) < encoded_length:
        raise FormatError(
            f"SPI special sequence declares {raw[2]} data bytes but supplies only {len(raw) - 3}"
        )
    # Taixin's INI examples sometimes carry one ignored byte after a zero-length
    # sequence (for example 06000000). BinScript/makecode serialize only the
    # command, dummy flags, data length, and declared data bytes.
    return raw[:encoded_length]


@dataclasses.dataclass(frozen=True)
class ImageConfig:
    boot_flag: int
    version: int
    load_address: int
    execute_address: int
    code_offset: int
    flash_size: int
    spi_clock_mhz: int
    driver_strength: int
    pll_source_mhz: int
    pll_enabled: bool
    debug_enabled: bool
    aes_enabled: bool
    code_crc16_enabled: bool
    read_command: int
    read_dummy_cycles: int
    clock_mode: int
    sample_delay: int
    command_wires: int
    address_wires: int
    data_wires: int
    quad_select: int
    quad_enabled: bool
    special_sequence_enabled: bool
    special_sequences: tuple[bytes, ...]
    chip_id: int
    cpu_id: int
    customer_id: int
    parameter_image_offset: int = DEFAULT_PARAM_IMAGE_OFFSET

    @classmethod
    def from_ini(cls, common: Mapping[str, str], spi: Mapping[str, str]) -> "ImageConfig":
        if parse_int(common.get("CodeType", "1")) != 1:
            raise FormatError("only SPI CodeType=1 is supported; EFLASH modes are not implemented")
        if parse_int(spi.get("AesEnable", "0")):
            raise FormatError("AES-enabled Taixin images are not supported")
        if not parse_int(spi.get("CodeCRC16", "1")):
            raise FormatError("TXW81x SPI images without CodeCRC16 are not supported")
        count = parse_int(spi.get("SpecSquenceNumbers", "0"))
        sequences = tuple(
            _parse_spi_sequence(spi[f"SpecSquence{index}"])
            for index in range(count)
            if spi.get(f"SpecSquence{index}", "").strip()
        )
        if len(sequences) != count:
            raise FormatError(f"expected {count} SPI special sequences, found {len(sequences)}")
        return cls(
            boot_flag=parse_int(spi.get("Flag", "5A69")),
            version=parse_int(spi.get("Version", "0")),
            load_address=parse_int(spi.get("CodeLoadToSramAddr", "18000000")),
            execute_address=parse_int(spi.get("CodeExeAddr", "18000000")),
            code_offset=parse_int(spi.get("CodeAddrOffset", "C00")),
            flash_size=parse_int(spi.get("SPI_SIZE", "100000")),
            spi_clock_mhz=parse_int(spi.get("SPI_CLK_MHZ", "3C")),
            driver_strength=parse_int(spi.get("DriverStrength", "0")),
            pll_source_mhz=parse_int(spi.get("PLL_SRC_MHZ", "28")),
            pll_enabled=bool(parse_int(spi.get("PLL_EN", "1"))),
            debug_enabled=bool(parse_int(spi.get("DebugInfoEn", "0"))),
            aes_enabled=bool(parse_int(spi.get("AesEnable", "0"))),
            code_crc16_enabled=bool(parse_int(spi.get("CodeCRC16", "1"))),
            read_command=parse_int(spi.get("ReadCmd", "EB")),
            read_dummy_cycles=parse_int(spi.get("ReadCmdDummy", "6")),
            clock_mode=parse_int(spi.get("ClockMode", "0")),
            sample_delay=parse_int(spi.get("SampleDelay", "55AA")),
            command_wires=parse_int(spi.get("WireModeWhenCmd", "1")),
            address_wires=parse_int(spi.get("WireModeWhenAddr", "4")),
            data_wires=parse_int(spi.get("WireModeWhenData", "4")),
            quad_select=parse_int(spi.get("WireMode4Select", "0")),
            quad_enabled=bool(parse_int(spi.get("WireMode4En", "1"))),
            special_sequence_enabled=bool(parse_int(spi.get("SpecSquenceEn", "1"))),
            special_sequences=sequences,
            chip_id=parse_int(common.get("CHIP_ID", "8410")),
            cpu_id=parse_int(common.get("CPU_ID", "0")),
            customer_id=parse_int(common.get("CustomerID", "0")),
        )


@dataclasses.dataclass(frozen=True)
class VersionInfo:
    sdk_version: int
    svn_version: int
    build_date: int
    app_version: int = 0


def build_boot_header(config: ImageConfig, code: bytes) -> bytes:
    mode = (
        (config.pll_source_mhz & 0xFF)
        | (int(config.pll_enabled) << 8)
        | (int(config.debug_enabled) << 9)
        | (int(config.aes_enabled) << 10)
        | (int(config.code_crc16_enabled) << 11)
    )
    baud_strength = (config.spi_clock_mhz & 0x3FFF) | ((config.driver_strength & 0x3) << 14)
    if config.flash_size % 0x10000:
        raise FormatError("TXW81x SPI_SIZE must be a multiple of 64 KiB")
    flash_blocks = config.flash_size // 0x10000
    code_crc = crc16_modbus(code) if config.code_crc16_enabled else 0
    without_crc = struct.pack(
        "<HBBIIIIHHHHH",
        config.boot_flag & 0xFFFF,
        config.version & 0xFF,
        0x1C,
        config.load_address & 0xFFFFFFFF,
        config.execute_address & 0xFFFFFFFF,
        config.code_offset & 0xFFFFFFFF,
        len(code) & 0xFFFFFFFF,
        code_crc,
        flash_blocks & 0xFFFF,
        baud_strength,
        mode & 0xFFFF,
        0,
    )
    return with_crc16(without_crc)


def build_spi_header(config: ImageConfig) -> bytes:
    cfg1 = (
        (config.read_dummy_cycles & 0xF)
        | ((config.clock_mode & 0x3) << 4)
        | (int(config.special_sequence_enabled) << 6)
        | (int(config.quad_enabled) << 7)
    )
    cfg2 = (
        _wire_mode_bits(config.command_wires)
        | (_wire_mode_bits(config.address_wires) << 2)
        | (_wire_mode_bits(config.data_wires) << 4)
        | ((config.quad_select & 0x3) << 6)
    )
    special = bytearray(64)
    if config.special_sequence_enabled:
        encoded = bytes([len(config.special_sequences)]) + b"".join(config.special_sequences)
        if len(encoded) > len(special):
            raise FormatError("SPI special sequence table exceeds 64 bytes")
        special[: len(encoded)] = encoded
    read_config = struct.pack(
        "<BBBBH",
        config.read_command & 0xFF,
        cfg1,
        cfg2,
        0,
        config.sample_delay & 0xFFFF,
    )
    return with_crc16(struct.pack("<BB", 1, 0x48) + read_config + special)


def build_firmware_header(config: ImageConfig, version: VersionInfo, parameter: bytes) -> bytes:
    code_crc32 = 0  # SPI CodeCRC16 is the active OpenTXW81X integrity mechanism.
    without_crc = struct.pack(
        "<BBIIIHBIH",
        2,
        0x17,
        version.sdk_version & 0xFFFFFFFF,
        version.svn_version & 0xFFFFFFFF,
        version.build_date & 0xFFFFFFFF,
        config.chip_id & 0xFFFF,
        config.cpu_id & 0xFF,
        code_crc32,
        crc16_modbus(parameter),
    )
    return with_crc16(without_crc)


def build_encryption_header(config: ImageConfig) -> bytes:
    # makecode writes customer ID only when AES is enabled.
    customer_id = config.customer_id if config.aes_enabled else 0
    return with_crc16(struct.pack("<BBH", 3, 4, customer_id & 0xFFFF))


def build_application_version_header(version: VersionInfo) -> bytes:
    # Function 4 is 40 bytes total: app, SDK and SVN versions, 24 reserved
    # bytes and CRC16. This layout is present in current Taixin output but is
    # absent from RNode's prepare_firmware.py.
    without_crc = struct.pack(
        "<BBIII",
        4,
        0x26,
        version.app_version & 0xFFFFFFFF,
        version.sdk_version & 0xFFFFFFFF,
        version.svn_version & 0xFFFFFFFF,
    ) + bytes(24)
    return with_crc16(without_crc)


def build_header_terminator(parameter_offset: int) -> bytes:
    # Function 0xFF plus the parameter offset and CRC16.
    return with_crc16(b"\xFF" + struct.pack("<I", parameter_offset & 0xFFFFFFFF))


def build_app_image(code: bytes, parameter: bytes, config: ImageConfig, version: VersionInfo) -> bytes:
    if config.aes_enabled:
        raise FormatError("AES-enabled Taixin images are not supported")
    if not config.code_crc16_enabled:
        raise FormatError("TXW81x SPI CodeCRC16 must be enabled")
    if config.code_offset + len(code) > config.flash_size:
        raise FormatError("APP image exceeds configured SPI flash capacity")
    if len(parameter) < 2 or struct.unpack_from("<H", parameter)[0] != len(parameter):
        raise FormatError("parameter block's first u16 must equal its byte length")
    headers = b"".join(
        (
            build_boot_header(config, code),
            build_spi_header(config),
            build_firmware_header(config, version, parameter),
            build_encryption_header(config),
            build_application_version_header(version),
            build_header_terminator(config.parameter_image_offset),
        )
    )
    if len(headers) > config.parameter_image_offset:
        raise FormatError("headers overlap the parameter block")
    parameter_end = config.parameter_image_offset + len(parameter)
    if parameter_end > config.code_offset:
        raise FormatError("parameter block overlaps code")
    output = bytearray(b"\xFF" * config.parameter_image_offset)
    output[: len(headers)] = headers
    output.extend(parameter)
    # Taixin makecode pads the parameter-to-code gap with zeroes.
    output.extend(b"\x00" * (config.code_offset - len(output)))
    output.extend(code)
    return bytes(output)


def _patch_boot_code_fields(prefix: bytes | bytearray, code: bytes) -> bytearray:
    if len(prefix) < 0x20:
        raise FormatError("image prefix is too small for a boot header")
    output = bytearray(prefix)
    struct.pack_into("<I", output, 0x10, len(code))
    struct.pack_into("<H", output, 0x14, crc16_modbus(code))
    struct.pack_into("<H", output, 0x1E, 0)
    struct.pack_into("<H", output, 0x1E, crc16_modbus(output[:0x1E]))
    return output


def compress_real_code(app: bytes) -> tuple[bytes, bytes, bytes]:
    parsed = parse_app_image(app)
    code_offset = parsed["boot"]["code_offset"]
    # real_code.BinScript requests APP length bytes from source offset C00;
    # bytes beyond EOF are 0xFF, yielding code + code_offset bytes of 0xFF.
    real_code = app[code_offset:] + b"\xFF" * code_offset
    xz_payload = lzma.compress(
        real_code,
        format=lzma.FORMAT_XZ,
        check=lzma.CHECK_CRC64,
        filters=[
            {
                "id": lzma.FILTER_LZMA2,
                "preset": 0,
                "dict_size": DEFAULT_COMPRESS_DICT_SIZE,
            }
        ],
    )
    descriptor_without_crc = (
        struct.pack("<III", COMPRESS_DESCRIPTOR_MAGIC, len(real_code), len(xz_payload))
        + bytes(14)
        + struct.pack("<HH", crc16_modbus(real_code), crc16_modbus(xz_payload))
    )
    descriptor = with_crc16(descriptor_without_crc)
    if len(descriptor) != 0x20:
        raise AssertionError("compression descriptor size mismatch")
    return real_code, descriptor, xz_payload


def build_ota_image(app: bytes, loader: bytes) -> bytes:
    app_info = parse_app_image(app)
    loader_info = parse_app_image(loader)
    code_offset = app_info["boot"]["code_offset"]
    if loader_info["boot"]["code_offset"] != code_offset:
        raise FormatError("APP and loader use different code offsets")
    if len(loader) != code_offset + loader_info["boot"]["code_length"]:
        raise FormatError("loader length does not match its boot header")

    _, descriptor, xz_payload = compress_real_code(app)
    compressed_code = descriptor + xz_payload
    compressed_image = bytes(_patch_boot_code_fields(app[:code_offset], compressed_code)) + compressed_code

    patched_loader = bytearray(loader)
    # Keep the loader's code address/length/checksum and function-4 identity,
    # but use the target APP's SPI, firmware and encryption settings.
    patched_loader[0x20:0x89] = app[0x20:0x89]
    patched_loader[0x16:0x1E] = app[0x16:0x1E]
    struct.pack_into("<H", patched_loader, 0x1E, 0)
    struct.pack_into("<H", patched_loader, 0x1E, crc16_modbus(patched_loader[:0x1E]))
    # The loader consumes the compressed image's leading header from 0x100 and
    # the target application's parameter area from 0x200.
    patched_loader[0x100:0x200] = compressed_image[:0x100]
    patched_loader[0x200:code_offset] = app[0x200:code_offset]
    return bytes(patched_loader) + compressed_image


def _parse_header_chain(data: bytes, start: int = 0x20) -> tuple[list[dict[str, object]], int, int]:
    headers: list[dict[str, object]] = []
    offset = start
    while offset < len(data):
        function = data[offset]
        if function == 0xFF:
            if offset + 7 > len(data):
                raise FormatError("truncated header terminator")
            raw = data[offset : offset + 7]
            if crc16_modbus(raw):
                raise FormatError("header terminator CRC16 is invalid")
            parameter_offset = struct.unpack_from("<I", raw, 1)[0]
            return headers, offset + 7, parameter_offset
        if offset + 2 > len(data):
            raise FormatError("truncated function header")
        size = data[offset + 1]
        total = size + 2
        raw = data[offset : offset + total]
        if len(raw) != total:
            raise FormatError(f"truncated function-{function} header")
        if crc16_modbus(raw):
            raise FormatError(f"function-{function} header CRC16 is invalid")
        headers.append({"function": function, "offset": offset, "size": total, "raw": raw})
        offset += total
    raise FormatError("header terminator was not found")


def parse_app_image(data: bytes) -> dict[str, object]:
    if len(data) < 0x20:
        raise FormatError("image is smaller than the boot header")
    boot_raw = data[:0x20]
    if crc16_modbus(boot_raw):
        raise FormatError("boot header CRC16 is invalid")
    values = BOOT_STRUCT.unpack(boot_raw)
    if values[0] != 0x5A69 or values[3] == 0 or values[5] == 0:
        raise FormatError("not a supported TXW/HGIC SPI image")
    boot = {
        "flag": values[0],
        "version": values[1],
        "header_size": values[2] + 4,
        "load_address": values[3],
        "execute_address": values[4],
        "code_offset": values[5],
        "code_length": values[6],
        "code_crc16": values[7],
        "flash_blocks_64k": values[8],
        "spi_clock_and_strength": values[9],
        "mode": values[10],
        "reserved": values[11],
        "header_crc16": values[12],
    }
    headers, header_end, parameter_offset = _parse_header_chain(data)
    by_function = {int(header["function"]): header for header in headers}
    required = {1, 2, 3, 4}
    if not required.issubset(by_function):
        raise FormatError(f"missing function headers: {sorted(required - set(by_function))}")
    fw_raw = by_function[2]["raw"]
    fw_values = FW_STRUCT.unpack(fw_raw)
    firmware = {
        "sdk_version": fw_values[2],
        "svn_version": fw_values[3],
        "date": fw_values[4],
        "chip_id": fw_values[5],
        "cpu_id": fw_values[6],
        "code_crc32": fw_values[7],
        "parameter_crc16": fw_values[8],
        "header_crc16": fw_values[9],
    }
    app_raw = by_function[4]["raw"]
    if len(app_raw) != 0x28:
        raise FormatError("unexpected function-4 header size")
    app_version, sdk_copy, svn_copy = struct.unpack_from("<III", app_raw, 2)
    if sdk_copy != firmware["sdk_version"] or svn_copy != firmware["svn_version"]:
        # Loader images intentionally have their own function-4 identity while
        # function-2 may be patched for the target APP. Report, but do not fail.
        version_copy_matches = False
    else:
        version_copy_matches = True
    if parameter_offset + 2 > len(data):
        raise FormatError("parameter offset is outside image")
    parameter_length = struct.unpack_from("<H", data, parameter_offset)[0]
    if parameter_length < 2 or parameter_offset + parameter_length > boot["code_offset"]:
        raise FormatError("invalid parameter block length")
    parameter = data[parameter_offset : parameter_offset + parameter_length]
    code_end = boot["code_offset"] + boot["code_length"]
    if code_end > len(data):
        raise FormatError("boot code range extends beyond image")
    code = data[boot["code_offset"] : code_end]
    checks = {
        "boot_header_crc16": crc16_modbus(boot_raw) == 0,
        "all_function_header_crc16": all(crc16_modbus(header["raw"]) == 0 for header in headers),
        "parameter_crc16": crc16_modbus(parameter) == firmware["parameter_crc16"],
        "code_crc16": not (boot["mode"] & (1 << 11)) or crc16_modbus(code) == boot["code_crc16"],
        "file_length": code_end == len(data),
        "function4_version_copy": version_copy_matches,
    }
    return {
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "boot": boot,
        "firmware": firmware,
        "application_version": app_version,
        "parameter_offset": parameter_offset,
        "parameter_length": parameter_length,
        "header_end": header_end,
        "headers": [
            {key: value for key, value in header.items() if key != "raw"}
            for header in headers
        ],
        "checks": checks,
    }


def parse_ota_image(data: bytes) -> dict[str, object]:
    loader_boot = parse_app_image(data)
    loader_length = loader_boot["boot"]["code_offset"] + loader_boot["boot"]["code_length"]
    if loader_length >= len(data):
        raise FormatError("OTA image does not contain a second image")
    loader = data[:loader_length]
    compressed = data[loader_length:]
    loader_info = parse_app_image(loader)
    compressed_info = parse_app_image(compressed)
    code_offset = compressed_info["boot"]["code_offset"]
    code_length = compressed_info["boot"]["code_length"]
    code = compressed[code_offset : code_offset + code_length]
    if len(code) < 0x20:
        raise FormatError("compressed code area is too small")
    descriptor = code[:0x20]
    if crc16_modbus(descriptor):
        raise FormatError("compression descriptor CRC16 is invalid")
    magic, raw_length, xz_length = struct.unpack_from("<III", descriptor)
    if magic != COMPRESS_DESCRIPTOR_MAGIC:
        raise FormatError("compression descriptor magic is invalid")
    raw_crc, xz_crc, descriptor_crc = struct.unpack_from("<HHH", descriptor, 0x1A)
    xz_payload = code[0x20:]
    if len(xz_payload) != xz_length:
        raise FormatError("XZ payload length does not match compression descriptor")
    real_code = lzma.decompress(xz_payload, format=lzma.FORMAT_XZ)
    checks = {
        "descriptor_crc16": crc16_modbus(descriptor) == 0,
        "raw_length": len(real_code) == raw_length,
        "raw_crc16": crc16_modbus(real_code) == raw_crc,
        "xz_crc16": crc16_modbus(xz_payload) == xz_crc,
        "total_length": loader_length + compressed_info["size"] == len(data),
    }
    return {
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "loader_length": loader_length,
        "loader": loader_info,
        "compressed_image": compressed_info,
        "compression": {
            "descriptor_magic": magic,
            "raw_length": raw_length,
            "xz_length": xz_length,
            "raw_crc16": raw_crc,
            "xz_crc16": xz_crc,
            "descriptor_crc16": descriptor_crc,
            "dictionary_size": DEFAULT_COMPRESS_DICT_SIZE,
            "real_code_sha256": hashlib.sha256(real_code).hexdigest(),
            "xz_sha256": hashlib.sha256(xz_payload).hexdigest(),
        },
        "checks": checks,
    }


def _metadata_from_template(path: Path) -> VersionInfo:
    info = parse_app_image(path.read_bytes())
    firmware = info["firmware"]
    return VersionInfo(
        sdk_version=firmware["sdk_version"],
        svn_version=firmware["svn_version"],
        build_date=firmware["date"],
        app_version=info["application_version"],
    )


def _version_info(args: argparse.Namespace) -> VersionInfo:
    template = _metadata_from_template(args.template_app) if args.template_app else None
    def choose(name: str, template_value: int | None) -> int:
        value = getattr(args, name)
        if value is not None:
            return parse_int(value)
        if template_value is not None:
            return template_value
        raise FormatError(f"--{name.replace('_', '-')} or --template-app is required")
    return VersionInfo(
        sdk_version=choose("sdk_version", template.sdk_version if template else None),
        svn_version=choose("svn_version", template.svn_version if template else None),
        build_date=parse_build_date(args.date) if args.date else (template.build_date if template else parse_build_date(None)),
        app_version=choose("app_version", template.app_version if template else 0),
    )


def command_pack_app(args: argparse.Namespace) -> int:
    common, spi = load_makecode_ini(args.makecode_ini)
    config = ImageConfig.from_ini(common, spi)
    base, code = load_code(args.input)
    parameter = extract_parameter(
        code,
        args.parameter_cfg.read_bytes(),
        length_offset=parse_int(common.get("ParamOffset", "180")),
        source_offset=args.parameter_source_offset,
    )
    version = _version_info(args)
    code = finish_compiler_code(
        code, parameter, parameter_offset=parse_int(common.get("ParamOffset", "180"))
    )
    image = build_app_image(code, parameter, config, version)
    result = parse_app_image(image)
    checks = {**result["checks"], **check_finished_code(code, parameter, parameter_offset=parse_int(common.get("ParamOffset", "180")))}
    if not all(checks.values()):
        raise FormatError(f"generated APP failed validation: {[key for key, passed in checks.items() if not passed]}")
    args.output.write_bytes(image)
    result["checks"].update(checks)
    result["input_base_address"] = base
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


def command_pack_ota(args: argparse.Namespace) -> int:
    app = args.app.read_bytes()
    app_info = parse_app_image(app)
    code = app[app_info["boot"]["code_offset"] :]
    parameter = app[app_info["parameter_offset"] : app_info["parameter_offset"] + app_info["parameter_length"]]
    if args.makecode_ini:
        common, _ = load_makecode_ini(args.makecode_ini)
        parameter_code_offset = parse_int(common.get("ParamOffset", "180"))
    else:
        parameter_code_offset = parse_int(args.parameter_code_offset)
    checks = {**app_info["checks"], **check_finished_code(code, parameter, parameter_offset=parameter_code_offset)}
    failures = [key for key, passed in checks.items() if not passed]
    if failures:
        raise FormatError(f"cannot create OTA from malformed APP: {failures}")
    image = build_ota_image(app, args.loader.read_bytes())
    result = parse_ota_image(image)
    if not all(result["checks"].values()) or not all(result["compressed_image"]["checks"].values()):
        raise FormatError("generated OTA failed integrity checks")
    args.output.write_bytes(image)
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


def command_inspect(args: argparse.Namespace) -> int:
    data = args.image.read_bytes()
    try:
        info = parse_ota_image(data)
        info["type"] = "ota"
    except (FormatError, lzma.LZMAError):
        info = parse_app_image(data)
        info["type"] = "app"
    print(json.dumps(info, indent=2, sort_keys=True))
    checks = dict(info["checks"])
    if info["type"] == "ota":
        checks.update({f"compressed_{key}": value for key, value in info["compressed_image"]["checks"].items()})
        checks.update({f"loader_{key}": value for key, value in info["loader"]["checks"].items() if key != "function4_version_copy"})
    failures = [key for key, passed in checks.items() if not passed]
    if failures:
        raise FormatError(f"firmware integrity checks failed: {failures}")
    return 0


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    app = sub.add_parser("pack-app", help="build APP.bin from raw binary or Intel HEX")
    app.add_argument("--input", type=Path, required=True, help="raw .bin or Intel HEX code image")
    app.add_argument("--parameter-cfg", type=Path, required=True, help="parameter.bincfg/parameter.cfg")
    app.add_argument("--makecode-ini", type=Path, required=True)
    app.add_argument("--output", type=Path, required=True)
    app.add_argument("--parameter-source-offset", type=parse_int, default=DEFAULT_PARAM_SOURCE_OFFSET)
    app.add_argument("--sdk-version", help="hex SDK version, e.g. 02050307")
    app.add_argument("--svn-version", help="hex SVN version")
    app.add_argument("--app-version", help="hex application version", default="0")
    app.add_argument("--date", help="build date in YYYY-MM-DD form")
    app.add_argument("--template-app", type=Path, help="copy version fields from a known APP image")
    app.set_defaults(func=command_pack_app)

    ota = sub.add_parser("pack-ota", help="build APP_compress.bin from APP.bin")
    ota.add_argument("--app", type=Path, required=True)
    ota.add_argument("--loader", type=Path, required=True, help="existing loader_compress.bin")
    ota.add_argument("--output", type=Path, required=True)
    ota.add_argument("--parameter-code-offset", default="180", help="hex embedded parameter offset, default 0x180")
    ota.add_argument("--makecode-ini", type=Path, help="derive embedded parameter offset from the same INI used for APP")
    ota.set_defaults(func=command_pack_ota)

    inspect = sub.add_parser("inspect", help="parse and validate APP or compressed OTA image")
    inspect.add_argument("image", type=Path)
    inspect.set_defaults(func=command_inspect)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_argument_parser()
    args = parser.parse_args(argv)
    try:
        return int(args.func(args))
    except (FormatError, OSError, lzma.LZMAError, ValueError) as exc:
        parser.error(str(exc))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
