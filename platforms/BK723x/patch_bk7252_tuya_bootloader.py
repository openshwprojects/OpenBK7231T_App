#!/usr/bin/env python3
import argparse
import hashlib
import pathlib
import subprocess
import tempfile


RAW_BOOTLOADER_SHA256 = "6342d53961e743dfe73c852b31c7083f08f740fc27444cb75ebcb22d4b1031f8"
WRAPPER_OFFSET = 0xD300
ORIGINAL_WRITE = 0x7778
CALL_SITES = (0x6774, 0x6816)


def thumb_bl(site, target):
    offset = target - (site + 4)
    if offset & 1:
        raise ValueError("unaligned Thumb BL target")
    if not (-(1 << 22) <= offset < (1 << 22)):
        raise ValueError("Thumb BL target out of range")

    imm = offset >> 1
    hi = 0xF000 | ((imm >> 11) & 0x7FF)
    lo = 0xF800 | (imm & 0x7FF)
    return hi.to_bytes(2, "little") + lo.to_bytes(2, "little")


def tool_path(toolchain, name):
    path = toolchain / name
    if path.exists():
        return path

    exe_path = toolchain / (name + ".exe")
    if exe_path.exists():
        return exe_path

    return path


def build_wrapper(toolchain, source):
    gcc = tool_path(toolchain, "arm-none-eabi-gcc")
    nm = tool_path(toolchain, "arm-none-eabi-nm")
    objcopy = tool_path(toolchain, "arm-none-eabi-objcopy")

    with tempfile.TemporaryDirectory(prefix="bk7252_tuya_bl_") as tmp:
        tmp_path = pathlib.Path(tmp)
        obj = tmp_path / "wrap.o"
        elf = tmp_path / "wrap.elf"
        binary = tmp_path / "wrap.bin"

        subprocess.run([
            str(gcc), "-c", str(source), "-o", str(obj),
            "-Os", "-mthumb", "-mcpu=arm9tdmi",
            "-ffreestanding", "-fno-builtin", "-fno-pic",
            "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
        ], check=True)
        subprocess.run([
            str(gcc), "-nostdlib", "-Wl,-Ttext=0x%08x" % WRAPPER_OFFSET,
            "-Wl,-e,tuya_fal_partition_write",
            "-o", str(elf), str(obj),
        ], check=True)
        nm_output = subprocess.check_output([str(nm), "-n", str(elf)], text=True)
        for line in nm_output.splitlines():
            parts = line.split()
            if len(parts) >= 3 and parts[2] == "tuya_fal_partition_write":
                entry = int(parts[0], 16)
                if entry != WRAPPER_OFFSET:
                    raise RuntimeError("wrapper entry at 0x%04x, expected 0x%04x" % (entry, WRAPPER_OFFSET))
                break
        else:
            raise RuntimeError("wrapper entry symbol not found")

        subprocess.run([
            str(objcopy), "-O", "binary", "-j", ".text", str(elf), str(binary),
        ], check=True)

        return binary.read_bytes()


def patch_bootloader(raw_path, output_path, wrapper):
    raw = bytearray(raw_path.read_bytes())
    raw_sha = hashlib.sha256(raw).hexdigest()
    if raw_sha != RAW_BOOTLOADER_SHA256:
        raise RuntimeError("unexpected BK7252 bootloader SHA256: %s" % raw_sha)

    for site in CALL_SITES:
        expected = thumb_bl(site, ORIGINAL_WRITE)
        found = bytes(raw[site:site + 4])
        if found != expected:
            raise RuntimeError("unexpected call at 0x%04x: got %s expected %s" % (site, found.hex(), expected.hex()))
        raw[site:site + 4] = thumb_bl(site, WRAPPER_OFFSET)

    wrapper_end = WRAPPER_OFFSET + len(wrapper)
    if len(raw) > WRAPPER_OFFSET:
        raise RuntimeError("wrapper offset overlaps bootloader body")
    if wrapper_end > 0x10000:
        raise RuntimeError("patched bootloader exceeds 64K logical boot partition")

    raw.extend(b"\xff" * (WRAPPER_OFFSET - len(raw)))
    raw[WRAPPER_OFFSET:wrapper_end] = wrapper
    output_path.write_bytes(raw)

    print("Patched BK7252 Tuya bootloader: wrapper %d bytes at 0x%04x" % (len(wrapper), WRAPPER_OFFSET))


def main():
    parser = argparse.ArgumentParser(description="Patch the BK7252 RT bootloader to coeff-encrypt app writes for Tuya OTA.")
    parser.add_argument("raw_bootloader", type=pathlib.Path)
    parser.add_argument("output_bootloader", type=pathlib.Path)
    parser.add_argument("toolchain_bin", type=pathlib.Path)
    parser.add_argument("wrapper_source", type=pathlib.Path)
    args = parser.parse_args()

    wrapper = build_wrapper(args.toolchain_bin, args.wrapper_source)
    patch_bootloader(args.raw_bootloader, args.output_bootloader, wrapper)


if __name__ == "__main__":
    main()
