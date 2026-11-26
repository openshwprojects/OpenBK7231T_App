import sys
import struct

def create_oneb(output, entries):
    with open(output, "wb") as out:
        out.write(b"oneb")
        out.write(b"\x00")

        for ftype, address, filename in entries:
            with open(filename, "rb") as f:
                data = f.read()

            out.write(struct.pack("<BII", ftype, address, len(data)))
            out.write(data)

            print(f"{filename}: type=0x{ftype:X}, address=0x{address:X}, size={len(data)}")

if len(sys.argv) < 5 or (len(sys.argv) - 2) % 3:
    print("Usage: python Create_AIO.py output.bin <type> <address> <file> [...]")
    sys.exit(1)

output = sys.argv[1]
entries = [
    (int(sys.argv[i], 0), int(sys.argv[i + 1], 0), sys.argv[i + 2])
    for i in range(2, len(sys.argv), 3)
]

create_oneb(output, entries)
print(f"Created {output}")
