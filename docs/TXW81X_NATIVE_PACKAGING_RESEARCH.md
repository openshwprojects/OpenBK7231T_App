# TXW81x native packaging research

## Status and scope

The implementation was developed in isolation and is now wired into the TXW81x target on the integration branch. The production change replaces only the post-link packaging stage and moves the TXW81x CI job from Windows/WSL to Ubuntu. Successful compilation and byte-identical software reconstruction do **not** prove that a generated image is bootable or OTA-safe on TXW81x hardware.

No proprietary Taixin executable or SDK file is committed. The implementation consumes the existing `makecode.ini`, `parameter.bincfg` and `loader_compress.bin` from an OpenTXW81X checkout at build time.

## Repositories and reference inputs

- OpenBeken: `openshwprojects/OpenBK7231T_App`
- TXW81x submodule: `NonPIayerCharacter/OpenTXW81X`
- RNode implementation: `I-AM-ENGINEER/RNode_Halow_Firmware`
- Public known-good OpenBeken Actions artifact: run `29204795766`, commit `4b43623c0cc3619b6be3aa7cfc0f4b5fbb6275e7`, merged artifact `8263481010`
- Supplied SDK archive used only for local comparison: SHA-256 `926f8bdc5e362e673043304162642e5db1ed40af0381f7bd127df4adf033d7ba`

The public Actions artifact contained:

| File | Size | SHA-256 |
|---|---:|---|
| `OpenTXW81X_main_4b43623c0cc3.bin` | 606,736 | `461b527fd88b3bae15e684d27775c2ee0409f4d09c6da0a638fd2f433568b562` |
| `OpenTXW81X_main_4b43623c0cc3_ota.img` | 424,852 | `dca07019a9f1e5f18ae9b63c175c8067ec70de9af08527abdd697b27eca15c04` |

The supplied SDK's `loader_compress.bin` had SHA-256 `253eb72ddd1eb8905c0dad57229b7fbee1d185718f3dc3f09aaabf6c80b711ba` and Git blob SHA-1 `37cf9a3184026732f09a41c569cf891da2faf889`, exactly matching the loader currently tracked by `OpenTXW81X`.

## Existing OpenBeken pipeline and intermediates

OpenBeken's top-level TXW81x target initializes `sdk/OpenTXW81X`, builds with the Linux C-SKY compiler, then calls `BinScript.exe` and `makecode.exe`. The final `APP.bin` is copied to OpenBeken's normal `.bin` output and `APP_compress.bin` is copied to `_ota.img`.

The complete observed pipeline is:

1. GNU Make and `csky-elfabiv2-*` compile and link the project.
2. OpenTXW81X produces build products under `Obj/` and `Lst/`, including:
   - `Obj/*.elf` — linked ELF;
   - `Obj/*.ihex` — Intel HEX application;
   - `Lst/*.map` — linker map;
   - object files and dependency files.
3. `BuildBIN.sh` normalizes those names:
   - `project.elf`;
   - `project.hex`;
   - `project.map`.
4. `BuildBIN.sh` copies runtime packaging inputs:
   - `parameter.bincfg` to `parameter.cfg`;
   - `sdk/chip/txw81x/loader_compress.bin` to `loader_compress.bin`.
5. `BinScript.exe BinScript.BinScript` performs two operations:
   - parses `project.hex`, fills holes with `0xFF`, remaps the lowest load address to file offset zero, and writes `txw81x_fpv.bin`;
   - reads a little-endian parameter length at raw-code offset `0x180`, extracts that many bytes from `parameter.cfg` offset `0x210`, writes the length into the first two bytes, and writes `param.bin`.
6. `makecode.exe` reads `makecode.ini`, `txw81x_fpv.bin` and `param.bin`, and emits its generated SPI image.
7. `merge.bat` copies the generated image to:
   - `program.bin`;
   - `APP.bin`;
   - a versioned normal-image filename.
8. If compression tooling is present:
   - `real_code.BinScript` reads `APP.bin`, remaps source offset `0xC00` to output zero for a requested length equal to the full APP size, and writes `realcode.bin`; the over-read beyond the original EOF becomes `0xFF`, so the result is application code followed by `0xC00` bytes of `0xFF`;
   - `xz.exe -kfz -0 realcode.bin` writes `realcode.bin.xz`;
   - `compress.exe APP.bin loader_compress.bin compress.bin APP_compress.bin` creates the compressed second-stage image (`compress.bin`) and the concatenated OTA (`APP_compress.bin`);
   - `APP_compress.bin` is copied to a versioned compressed-image filename.
9. OpenBeken copies:
   - `APP.bin` to `output/.../OpenTXW81X_<version>.bin`;
   - `APP_compress.bin` to `output/.../OpenTXW81X_<version>_ota.img`.

The Windows-only boundary is therefore after ELF/HEX creation. The native compiler itself is not the blocker.

## `APP.bin` structure

The known-good APP layout is:

| Offset | Length | Meaning |
|---:|---:|---|
| `0x000` | `0x20` | boot header, function 0 |
| `0x020` | `0x4A` | SPI-read configuration, function 1 |
| `0x06A` | `0x19` | firmware information, function 2 |
| `0x083` | `0x06` | encryption/customer information, function 3 |
| `0x089` | `0x28` | application-version information, function 4 |
| `0x0B1` | `0x07` | terminator `0xFF`, parameter offset, CRC16 |
| `0x0B8` | to `0x200` | `0xFF` padding |
| `0x200` | `0x800` | parameter block; first u16 is its length |
| `0xA00` | to `0xC00` | zero padding |
| `0xC00` | `0x93610` | raw application code |

Reference boot fields:

- flag `0x5A69`;
- load address `0x18000000`;
- execution address `0x18000000`;
- flash code offset `0x00000C00`;
- code length `0x00093610`;
- code CRC16 `0x5779`;
- flash size field `0x0010`, meaning sixteen 64 KiB blocks for `SPI_SIZE=0x100000`;
- SPI clock `0x3C` (60 MHz);
- mode `0x0928`: PLL source 40 MHz, PLL enabled, code CRC16 enabled.

Reference firmware fields:

- SDK version `0x02050307`;
- SVN version `0x00008EB5`;
- date `0x07EA070C` (2026-07-12);
- chip ID `0x8410`;
- CPU ID `0`;
- code CRC32 field `0` because SPI code CRC16 is used;
- parameter CRC16 `0xD260`.

All boot/function headers use CRC-16/MODBUS (polynomial `0xA001`, initial value `0xFFFF`) and are stored so that CRC16 over the complete header is zero. The code and parameter CRC fields use the same CRC.

Function 4 is not implemented by the RNode script. Its observed TXW layout is:

- function `4`, size `0x26`;
- application version u32;
- SDK version u32;
- SVN version u32;
- 24 reserved zero bytes;
- CRC16.

## RNode Python implementation: what it implements

`RNode_Halow_Firmware/pack/prepare_firmware.py` implements the same underlying HGIC/Taixin SPI-image family:

- CRC-16/MODBUS and reflected CRC-32;
- 32-byte boot header;
- function-1 SPI-read header;
- function-2 firmware-information header;
- parameter placement and code placement;
- optional use of a template prefix.

It is not a drop-in TXW81x packer. Its default target and layout are TXW8301/HaLow:

| Field | RNode default | OpenBeken TXW81x |
|---|---:|---:|
| chip ID | `0x4002` | `0x8410` |
| CPU ID | `1` | `0` |
| load/execute | `0x20001000` | `0x18000000` |
| code offset | `0x2000` | `0x0C00` |
| parameter offset | `0x0200` | `0x0200` |
| flash size | RNode-specific/default logic | `0x100000` |

Additional incompatibilities found:

1. It derives the boot flash-size field from the code offset. TXW81x's Taixin output stores `SPI_SIZE / 64 KiB`, yielding `0x10`, not `0xC00 / 512 = 6`.
2. It serializes special SPI-sequence strings directly. Taixin writes a leading sequence count and then each sequence as `cmd`, `dummy`, `data_length`, and exactly that many data bytes. For `06000000`, the final INI byte is ignored because `data_length` is zero.
3. It does not construct function 3, function 4, or the `0xFF + parameter-offset + CRC` terminator.
4. Without a template, its pre-code padding does not match TXW81x's `0xFF` header gap plus zero parameter-to-code gap.
5. It does not implement the `BinScript` Intel HEX remap/parameter extraction stage.
6. It does not implement Taixin's TXW81x compressed OTA container.

The new implementation retains the RNode project's MIT attribution but implements the missing TXW81x behavior explicitly.

## `APP_compress.bin` / OTA structure

The compressed OTA is not merely an XZ file. It is:

```text
patched loader_compress.bin || compressed SPI image
```

For the reference:

- patched loader length: `62,992` (`0xF610`);
- compressed SPI image length: `361,860` (`0x58584`);
- total OTA length: `424,852`.

The second image is a normal TXW SPI image. Its prefix is copied from `APP.bin`, except its boot code length, code CRC16 and boot-header CRC16 are recalculated for its compressed code region.

The compressed code region is:

```text
32-byte descriptor || XZ payload
```

Descriptor layout:

| Offset | Type | Meaning |
|---:|---|---|
| `0x00` | u32 | magic `0xA1B2C3D4` |
| `0x04` | u32 | uncompressed `realcode.bin` length |
| `0x08` | u32 | XZ payload length |
| `0x0C` | 14 bytes | reserved zeroes |
| `0x1A` | u16 | uncompressed CRC16 |
| `0x1C` | u16 | XZ CRC16 |
| `0x1E` | u16 | descriptor CRC16 |

Reference values:

- uncompressed length `606,736` (`0x94210`);
- XZ length `358,756` (`0x57964`);
- uncompressed CRC16 `0x5B2A`;
- XZ CRC16 `0x83B5`;
- compressed-image code length `358,788` (`0x57984`).

The legacy Windows `xz.exe -0` output uses an LZMA2 dictionary of 32 KiB. Current XZ defaults for preset 0 use 256 KiB and are not byte-identical. Python is exact when called with:

```python
lzma.compress(
    real_code,
    format=lzma.FORMAT_XZ,
    check=lzma.CHECK_CRC64,
    filters=[{"id": lzma.FILTER_LZMA2, "preset": 0, "dict_size": 32768}],
)
```

Loader patching observed and reproduced:

1. preserve the loader's own code address, length, CRC and function-4 identity;
2. copy the target APP's function-1, function-2 and function-3 headers into the loader;
3. copy target APP flash-size/SPI-mode boot fields and recalculate the loader boot-header CRC;
4. copy the compressed image's first `0x100` bytes into loader offset `0x100`;
5. copy the target APP parameter/padding area `[0x200, 0xC00)` into the loader;
6. concatenate patched loader and compressed image.

This produced a byte-identical reference OTA.

## Native software demonstration

The post-build conversion was executed without Wine, Windows executables or WSL using Python 3.13.5 on x86-64 Linux. The commands were:

```bash
python3 txw81x_pack.py pack-app \
  --input txw81x_fpv.bin \
  --parameter-cfg parameter.bincfg \
  --makecode-ini makecode.ini \
  --sdk-version 02050307 \
  --svn-version 8EB5 \
  --app-version 0 \
  --date 2026-07-12 \
  --output APP.bin

python3 txw81x_pack.py pack-ota \
  --app APP.bin \
  --loader loader_compress.bin \
  --output APP_compress.bin

cmp APP.bin OpenTXW81X_main_4b43623c0cc3.bin
cmp APP_compress.bin OpenTXW81X_main_4b43623c0cc3_ota.img
```

Both `cmp` operations succeeded. The complete compiler and packaging path has also run on `ubuntu-22.04`. A later same-input comparison used the raw code and metadata from the July 22 WSL/Taixin build: both the wrapper-generated `APP.bin` and `APP_compress.bin` were byte-identical to that build. This avoids the invalid comparison of separately compiled applications containing different `__DATE__`/`__TIME__` strings.

## Test coverage

Always-run tests cover:

- CRC vectors;
- Intel HEX checksum validation, address remapping and `0xFF` gap filling;
- parameter extraction;
- Taixin special-sequence parsing;
- deterministic synthetic APP generation;
- parser/checksum self-consistency.

Opt-in known-good tests cover:

- recorded SHA-256 values;
- every important APP field and checksum;
- OTA segmentation and compression descriptor;
- byte-identical APP regeneration;
- byte-identical OTA regeneration.

Reference integration result:

```text
Ran 8 tests in 3.038s
OK
```

## Uncertainties and limits

- Real hardware has not been tested. Byte identity to a Taixin-generated reference is strong format evidence, not a bootability proof.
- The current implementation targets `CodeType=1` SPI and the exact OpenTXW81X options. EFLASH modes are out of scope.
- AES/encryption has not been validated. Function 3 is generated according to the observed non-AES behavior, but encrypted output is not implemented.
- Version metadata is derived by `package_native.sh` from the checked-out OpenTXW81X headers. The build date follows UTC or `SOURCE_DATE_EPOCH`; explicit environment overrides remain available for forensic reproducibility.
- The OTA loader is an input from the existing submodule. Reimplementing or rebuilding that loader is not part of this task.
- The function-4 interpretation is based on repeated structure and exact reconstruction; no public Taixin specification for that function was found.
- The compressed descriptor field names are inferred from behavior and checksum validation. Its complete vendor naming is unknown.

## Required hardware validation before integration

1. Build OpenBeken normally and retain the Taixin-generated APP/OTA plus raw code and metadata.
2. Generate Python APP/OTA from the same raw inputs and confirm byte identity.
3. Flash the Python APP through the established wired method to a recoverable TXW81x device.
4. Verify cold boot, repeated power cycles, Wi-Fi, settings persistence and normal OpenBeken functions.
5. Exercise OTA with serial logging and a recovery route available.
6. Verify OTA success, reboot, settings retention, rollback/recovery behavior and a subsequent OTA.
7. Repeat with at least one materially different firmware size and, ideally, a second TXW81x board/device.
8. Keep the change in draft/review status until those hardware tests pass; software equivalence alone is not a bootability or OTA-safety guarantee.

## October 2026 parity-review correction

A review of three independently archived original Windows/WSL APP and OTA pairs
(July 12, July 22 and October 25, 2026) found that serializing code directly
from `project.hex` was incomplete. The original `makecode.exe` additionally:

1. Replaces the reserved code region beginning at `ParamOffset=0x180` with
   the parameter block extracted from `parameter.bincfg`. The flash parameter
   area at offset `0x200` alone is insufficient.
2. Pads raw code to a 512-byte boundary, retains twelve more zero bytes,
   and appends the little-endian complemented CRC32 over the preceding code.
   The final code length is `ceil(raw_len / 0x200) * 0x200 + 0x10`.

The old Python-generated outputs omitted both transformations but still
passed image-header CRC16 checks and the earlier round-trip tests, because
those tests used code extracted from already packaged Windows firmware.

With both transformations implemented, CLI-level Intel-HEX-to-APP and APP-to-OTA
reconstructions match the original APP and OTA **byte-for-byte** for all three
reference pairs. The unprepared code used in these tests was reconstructed by
removing the reference's code padding/trailer and clearing the embedded
parameter reservation; this is not a substitute for an independent capture of
Taixin's original `project.hex`. That exact same-input comparison and real
hardware boot/OTA validation are still required before treating the build as
production-proven.

The SDK `script.7z` documentation independently describes the 32-byte
`compress_s` descriptor and the special low-memory `xz -0` mode; the newer
v2.5.4.7-45354 SDK does not supply native Linux replacements for `BinScript.exe`
and `makecode.exe`.
