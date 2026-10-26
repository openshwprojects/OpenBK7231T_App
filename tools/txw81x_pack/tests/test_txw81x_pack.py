"""TXW81x packer regression tests.

From the repository root:
    python3 -m unittest discover -s tools/txw81x_pack/tests -v

For reference tests, also set TXW81X_REFERENCE_DIR to the directory containing
files named in reference_manifest.json and TXW81X_LOADER to the SDK's
loader_compress.bin. Without these inputs, KnownGoodTaixinTests is skipped.
Reference reconstruction does not replace testing both packagers against the
same original compiler-generated project.hex.
"""
from __future__ import annotations

import importlib.util
import contextlib
import io
import zlib
import json
import os
import struct
import tempfile
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).resolve().parents[1] / "txw81x_pack.py"
SPEC = importlib.util.spec_from_file_location("txw81x_pack", MODULE_PATH)
assert SPEC and SPEC.loader
pack = importlib.util.module_from_spec(SPEC)
import sys
sys.modules[SPEC.name] = pack
SPEC.loader.exec_module(pack)

MANIFEST = json.loads((Path(__file__).with_name("reference_manifest.json")).read_text())


def txw81x_config() -> pack.ImageConfig:
    sequences = tuple(
        pack._parse_spi_sequence(value)
        for value in ("06000000", "0100020002", "31000102", "05800101")
    )
    return pack.ImageConfig(
        boot_flag=0x5A69,
        version=0,
        load_address=0x18000000,
        execute_address=0x18000000,
        code_offset=0xC00,
        flash_size=0x100000,
        spi_clock_mhz=0x3C,
        driver_strength=0,
        pll_source_mhz=0x28,
        pll_enabled=True,
        debug_enabled=False,
        aes_enabled=False,
        code_crc16_enabled=True,
        read_command=0xEB,
        read_dummy_cycles=6,
        clock_mode=0,
        sample_delay=0x55AA,
        command_wires=1,
        address_wires=4,
        data_wires=4,
        quad_select=0,
        quad_enabled=True,
        special_sequence_enabled=True,
        special_sequences=sequences,
        chip_id=0x8410,
        cpu_id=0,
        customer_id=0x1001,
    )


class AlgorithmTests(unittest.TestCase):
    def test_crc_vectors(self) -> None:
        data = b"123456789"
        self.assertEqual(pack.crc16_modbus(data), 0x4B37)
        self.assertEqual(pack.crc32_hgic(data), 0xCBF43926)

    def test_spi_zero_data_sequence_ignores_ini_padding_byte(self) -> None:
        self.assertEqual(pack._parse_spi_sequence("06000000"), bytes.fromhex("060000"))

    def test_intel_hex_remap_and_gap_fill(self) -> None:
        # Two records at 0x18000000 and 0x18000010 with an eight-byte gap.
        text = "\n".join(
            (
                ":020000041800E2",
                ":080000000102030405060708D4",
                ":04001000090A0B0CC2",
                ":00000001FF",
            )
        )
        base, binary = pack.ihex_to_binary(text)
        self.assertEqual(base, 0x18000000)
        self.assertEqual(binary[:8], bytes(range(1, 9)))
        self.assertEqual(binary[8:16], b"\xFF" * 8)
        self.assertEqual(binary[16:], bytes.fromhex("090A0B0C"))

    def test_parameter_extraction(self) -> None:
        code = bytearray(0x300)
        struct.pack_into("<H", code, 0x180, 0x20)
        source = bytearray(0x210 + 0x20)
        source[0x210:] = bytes(range(0x20))
        parameter = pack.extract_parameter(bytes(code), bytes(source), length_offset=0x180)
        self.assertEqual(len(parameter), 0x20)
        self.assertEqual(parameter[:2], b"\x20\x00")
        self.assertEqual(parameter[2:], bytes(range(2, 0x20)))

    def test_finalise_code_inserts_parameters_and_crc32_trailer(self) -> None:
        parameter = bytearray(0x80)
        struct.pack_into("<H", parameter, 0, len(parameter))
        parameter[2:] = bytes((index * 13) & 0xFF for index in range(len(parameter) - 2))
        raw = bytearray((index * 7 + 1) & 0xFF for index in range(0x9A2))
        struct.pack_into("<H", raw, 0x180, len(parameter))
        raw[0x182 : 0x180 + len(parameter)] = bytes(len(parameter) - 2)
        result = pack.finish_compiler_code(bytes(raw), bytes(parameter), parameter_offset=0x180)
        self.assertEqual(len(result), 0xA10)
        self.assertEqual(result[:0x180], raw[:0x180])
        self.assertEqual(result[0x180 : 0x180 + len(parameter)], parameter)
        self.assertEqual(result[0x180 + len(parameter) : len(raw)], raw[0x180 + len(parameter) :])
        self.assertEqual(result[len(raw) : -4], bytes(len(result) - len(raw) - 4))
        self.assertEqual(struct.unpack_from("<I", result, len(result) - 4)[0], (~zlib.crc32(result[:-4])) & 0xFFFFFFFF)
        self.assertTrue(all(pack.check_finished_code(result, parameter, parameter_offset=0x180).values()))

    def test_code_size_alignment_at_boundaries(self) -> None:
        parameter = struct.pack("<H", 0x20) + bytes(range(2, 0x20))
        for length, expected in ((0x600, 0x610), (0x601, 0x810), (0x7FF, 0x810), (0x800, 0x810)):
            with self.subTest(length=length):
                raw = bytearray(b"\x5A" * length)
                struct.pack_into("<H", raw, 0x180, len(parameter))
                packed = pack.finish_compiler_code(raw, parameter, parameter_offset=0x180)
                self.assertEqual(len(packed), expected)
                self.assertTrue(all(pack.check_finished_code(packed, parameter, parameter_offset=0x180).values()))

    def test_reject_unsupported_image_configurations(self) -> None:
        for common, spi in (({"CodeType": "2"}, {}), ({"CodeType": "1"}, {"AesEnable": "1"}),
                            ({"CodeType": "1"}, {"CodeCRC16": "0"})):
            with self.subTest(common=common, spi=spi):
                with self.assertRaises(pack.FormatError):
                    pack.ImageConfig.from_ini(common, spi)

    def test_bad_parameter_reservation_is_rejected(self) -> None:
        parameter = struct.pack("<H", 0x80) + bytes(0x7E)
        with self.assertRaises(pack.FormatError):
            pack.finish_compiler_code(bytes(0x1C0), parameter, parameter_offset=0x180)

    def test_cli_packs_raw_hex_and_detects_corrupted_output(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            directory = Path(tmpdir)
            raw = bytearray((index * 7 + 5) & 0xFF for index in range(0x701))
            struct.pack_into("<H", raw, 0x180, 0x20)
            raw[0x182 : 0x1A0] = bytes(0x1E)
            parameter = struct.pack("<H", 0x20) + bytes(range(2, 0x20))
            cfg = bytearray(0x210 + len(parameter))
            cfg[0x210:] = parameter
            (directory / "parameter.bincfg").write_bytes(cfg)
            (directory / "makecode.ini").write_text("[COMMON]\nCodeType=1\nParamOffset=180\n[SPI]\nCodeCRC16=1\nSpecSquenceNumbers=0\n", encoding="utf-8")
            def record(address: int, kind: int, data: bytes) -> str:
                payload = bytes((len(data), address >> 8, address & 0xFF, kind)) + data
                return ":" + (payload + bytes(((-sum(payload)) & 0xFF,))).hex().upper()
            records = [record(0, 4, struct.pack(">H", 0x1800))]
            records.extend(record(i, 0, raw[i:i + 32]) for i in range(0, len(raw), 32))
            records.append(record(0, 1, b""))
            (directory / "project.hex").write_text("\n".join(records) + "\n", encoding="ascii")
            output = directory / "APP.bin"
            cmd = ["pack-app", "--input", str(directory / "project.hex"), "--parameter-cfg", str(directory / "parameter.bincfg"),
                   "--makecode-ini", str(directory / "makecode.ini"), "--sdk-version", "02050307", "--svn-version", "8EB5",
                   "--date", "2026-10-26", "--output", str(output)]
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(pack.main(cmd), 0)
            image = output.read_bytes()
            parsed = pack.parse_app_image(image)
            self.assertTrue(all(parsed["checks"].values()))
            code = image[parsed["boot"]["code_offset"]:]
            self.assertEqual(len(code), 0x810)
            self.assertTrue(all(pack.check_finished_code(code, parameter, parameter_offset=0x180).values()))
            corrupt = bytearray(image)
            corrupt[-20] ^= 0x01
            (directory / "corrupt.bin").write_bytes(corrupt)
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as ex:
                    pack.main(["inspect", str(directory / "corrupt.bin")])
            self.assertNotEqual(ex.exception.code, 0)

    def test_flash_capacity_is_checked(self) -> None:
        config = txw81x_config()
        parameter = struct.pack("<H", 0x20) + bytes(0x1E)
        version = pack.VersionInfo(0x02050307, 0x8EB5, 0x07EA0A1A, 0)
        with self.assertRaises(pack.FormatError):
            pack.build_app_image(bytes(config.flash_size - config.code_offset + 1), parameter, config, version)

    def test_ota_command_rejects_missing_embedded_parameter(self) -> None:
        config = txw81x_config()
        parameter = struct.pack("<H", 0x20) + bytes(range(2, 0x20))
        raw = bytearray(b"\x00" * 0x900)
        struct.pack_into("<H", raw, 0x180, 0x20)
        version = pack.VersionInfo(0x02050307, 0x8EB5, 0x07EA0A1A, 0)
        # A superficially valid APP (header, parameter CRC and code CRC pass)
        # must still be rejected if the vendor's code insertion was omitted.
        bad_app = pack.build_app_image(raw, parameter, config, version)
        self.assertTrue(all(pack.parse_app_image(bad_app)["checks"].values()))
        with tempfile.TemporaryDirectory() as tmpdir:
            app_path = Path(tmpdir) / "APP.bin"
            app_path.write_bytes(bad_app)
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as raised:
                    pack.main(["pack-ota", "--app", str(app_path), "--loader", str(Path(tmpdir) / "unused.bin"),
                               "--output", str(Path(tmpdir) / "APP_compress.bin")])
            self.assertNotEqual(raised.exception.code, 0)

    def test_synthetic_app_is_deterministic_and_self_consistent(self) -> None:
        code = bytearray((index * 17 + 3) & 0xFF for index in range(0x700))
        struct.pack_into("<H", code, 0x180, 0x40)
        parameter = bytearray(0x40)
        struct.pack_into("<H", parameter, 0, len(parameter))
        parameter[2:] = bytes((index * 11) & 0xFF for index in range(len(parameter) - 2))
        version = pack.VersionInfo(0x02050307, 0x1234, 0x07EA0716, 0)
        first = pack.build_app_image(bytes(code), bytes(parameter), txw81x_config(), version)
        second = pack.build_app_image(bytes(code), bytes(parameter), txw81x_config(), version)
        self.assertEqual(first, second)
        info = pack.parse_app_image(first)
        self.assertTrue(all(info["checks"].values()))


class KnownGoodTaixinTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        reference_dir = os.environ.get("TXW81X_REFERENCE_DIR")
        loader_path = os.environ.get("TXW81X_LOADER")
        if not reference_dir or not loader_path:
            raise unittest.SkipTest(
                "set TXW81X_REFERENCE_DIR and TXW81X_LOADER to run known-good Taixin integration tests"
            )
        cls.reference_dir = Path(reference_dir)
        cls.loader_path = Path(loader_path)
        cls.app_path = cls.reference_dir / MANIFEST["app"]["filename"]
        cls.ota_path = cls.reference_dir / MANIFEST["ota"]["filename"]
        for path in (cls.app_path, cls.ota_path, cls.loader_path):
            if not path.is_file():
                raise unittest.SkipTest(f"reference file is missing: {path}")

    def test_reference_hashes_and_parsed_fields(self) -> None:
        app = self.app_path.read_bytes()
        ota = self.ota_path.read_bytes()
        loader = self.loader_path.read_bytes()
        self.assertEqual(pack.hashlib.sha256(app).hexdigest(), MANIFEST["app"]["sha256"])
        self.assertEqual(pack.hashlib.sha256(ota).hexdigest(), MANIFEST["ota"]["sha256"])
        self.assertEqual(pack.hashlib.sha256(loader).hexdigest(), MANIFEST["loader"]["sha256"])

        app_info = pack.parse_app_image(app)
        expected_app = MANIFEST["app"]
        self.assertEqual(app_info["size"], expected_app["size"])
        self.assertEqual(app_info["boot"]["code_offset"], expected_app["code_offset"])
        self.assertEqual(app_info["boot"]["code_length"], expected_app["code_length"])
        self.assertEqual(app_info["boot"]["code_crc16"], expected_app["code_crc16"])
        self.assertEqual(app_info["parameter_offset"], expected_app["parameter_offset"])
        self.assertEqual(app_info["parameter_length"], expected_app["parameter_length"])
        self.assertEqual(app_info["firmware"]["parameter_crc16"], expected_app["parameter_crc16"])
        self.assertEqual(app_info["firmware"]["sdk_version"], expected_app["sdk_version"])
        self.assertEqual(app_info["firmware"]["svn_version"], expected_app["svn_version"])
        self.assertEqual(app_info["firmware"]["date"], expected_app["date"])
        self.assertEqual(app_info["firmware"]["chip_id"], expected_app["chip_id"])
        self.assertEqual(app_info["firmware"]["cpu_id"], expected_app["cpu_id"])
        self.assertTrue(all(app_info["checks"].values()))

        ota_info = pack.parse_ota_image(ota)
        expected_ota = MANIFEST["ota"]
        self.assertEqual(ota_info["size"], expected_ota["size"])
        self.assertEqual(ota_info["loader_length"], expected_ota["loader_length"])
        self.assertEqual(ota_info["compressed_image"]["size"], expected_ota["compressed_image_length"])
        self.assertEqual(
            ota_info["compressed_image"]["boot"]["code_length"], expected_ota["compressed_code_length"]
        )
        self.assertEqual(ota_info["compression"]["xz_length"], expected_ota["xz_length"])
        self.assertEqual(ota_info["compression"]["raw_length"], expected_ota["real_code_length"])
        self.assertEqual(ota_info["compression"]["raw_crc16"], expected_ota["real_code_crc16"])
        self.assertEqual(ota_info["compression"]["xz_crc16"], expected_ota["xz_crc16"])
        self.assertTrue(all(ota_info["checks"].values()))

    def test_app_repack_is_byte_identical(self) -> None:
        app = self.app_path.read_bytes()
        info = pack.parse_app_image(app)
        code_offset = info["boot"]["code_offset"]
        parameter_offset = info["parameter_offset"]
        parameter_length = info["parameter_length"]
        code = app[code_offset:]
        parameter = app[parameter_offset : parameter_offset + parameter_length]
        version = pack.VersionInfo(
            info["firmware"]["sdk_version"],
            info["firmware"]["svn_version"],
            info["firmware"]["date"],
            info["application_version"],
        )
        rebuilt = pack.build_app_image(code, parameter, txw81x_config(), version)
        self.assertEqual(rebuilt, app)

    def test_rebuild_from_prepack_code_matches_vendor_app_and_ota(self) -> None:
        """Regenerate parameters, padding and CRC32 from reconstructed raw code.

        This input is derived from reference output, not a captured compiler HEX.
        """
        app = self.app_path.read_bytes()
        ota = self.ota_path.read_bytes()
        info = pack.parse_app_image(app)
        code = app[info["boot"]["code_offset"]:]
        parameter = app[info["parameter_offset"]:info["parameter_offset"] + info["parameter_length"]]
        # Strip the CRC32 and trailing zeroes to exercise code preparation.
        # This reconstruction does not establish the original compiler length.
        raw = bytearray(code[:-4].rstrip(b"\x00"))
        raw[0x180 : 0x180 + len(parameter)] = bytes(len(parameter))
        raw[0x180 : 0x182] = parameter[:2]
        completed = pack.finish_compiler_code(raw, parameter, parameter_offset=0x180)
        self.assertEqual(completed, code)
        version = pack.VersionInfo(
            info["firmware"]["sdk_version"], info["firmware"]["svn_version"],
            info["firmware"]["date"], info["application_version"],
        )
        rebuilt = pack.build_app_image(completed, parameter, txw81x_config(), version)
        self.assertEqual(rebuilt, app)
        self.assertEqual(pack.build_ota_image(rebuilt, self.loader_path.read_bytes()), ota)

    def test_ota_repack_is_byte_identical(self) -> None:
        app = self.app_path.read_bytes()
        expected = self.ota_path.read_bytes()
        loader = self.loader_path.read_bytes()
        rebuilt = pack.build_ota_image(app, loader)
        self.assertEqual(rebuilt, expected)


if __name__ == "__main__":
    unittest.main(verbosity=2)
