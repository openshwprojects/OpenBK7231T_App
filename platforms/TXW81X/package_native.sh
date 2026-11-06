#!/bin/sh
# Native TXW81x APP.bin and APP_compress.bin packaging for OpenBeken.
# Use compiler HEX: BinScript fills address gaps with 0xFF; raw objcopy output
# may fill them differently. The packer handles parameter insertion and CRCs.
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
SDK_ROOT="$ROOT_DIR/sdk/OpenTXW81X"
PROJECT_DIR="$SDK_ROOT/project"
PACKER="$ROOT_DIR/platforms/TXW81X/txw81x_pack.py"
LOADER="$SDK_ROOT/sdk/chip/txw81x/loader_compress.bin"
PYTHON=${PYTHON:-python3}

for required in \
    "$PROJECT_DIR/project.hex" \
    "$PROJECT_DIR/parameter.bincfg" \
    "$PROJECT_DIR/makecode.ini" \
    "$SDK_ROOT/sdk/include/version.h" \
    "$SDK_ROOT/sdk/include/svn_version.h" \
    "$SDK_ROOT/sdk/include/app_version.h" \
    "$SDK_ROOT/project/sys_config.h" \
    "$SDK_ROOT/sdk/include/chip/tx_platform.h" \
    "$LOADER" \
    "$PACKER"; do
    if [ ! -f "$required" ]; then
        echo "TXW81x packaging input not found: $required" >&2
        exit 1
    fi
done

# Derive the same metadata fields used by makecode.exe from the checked-out SDK.
eval "$("$PYTHON" - "$SDK_ROOT" <<'PY'
import re
import shlex
import sys
from pathlib import Path

root = Path(sys.argv[1])

def define(path: Path, name: str) -> str:
    pattern = re.compile(rf'^\s*#\s*define\s+{re.escape(name)}\s+(.+?)\s*$')
    for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
        match = pattern.match(line)
        if match:
            return re.split(r'/\*|//', match.group(1), maxsplit=1)[0].strip().split()[0]
    raise SystemExit(f'missing #define {name} in {path}')

def number(text: str) -> int:
    text = text.strip().strip('()')
    text = re.sub(r'[uUlL]+$', '', text)
    return int(text, 0)

version_h = root / 'sdk/include/version.h'
svn_h = root / 'sdk/include/svn_version.h'
app_h = root / 'sdk/include/app_version.h'
sys_config_h = root / 'project/sys_config.h'
platform_h = root / 'sdk/include/chip/tx_platform.h'

major = number(define(version_h, 'SDK_MVER'))
branch = number(define(version_h, 'SDK_BVER'))
patch = number(define(version_h, 'SDK_PVER'))
project_name = define(sys_config_h, 'PROJECT_TYPE')
project_type = number(define(platform_h, project_name))
svn = number(define(svn_h, 'SVN_VERSION'))
app = number(define(app_h, 'APP_VERSION'))

values = {
    'TXW81X_DERIVED_SDK_VERSION': f'{major:02X}{branch:02X}{patch:02X}{project_type:02X}',
    'TXW81X_DERIVED_SVN_VERSION': f'{svn:X}',
    'TXW81X_DERIVED_IMAGE_VERSION': f'{app:X}',
}
for key, value in values.items():
    print(f'{key}={shlex.quote(value)}')
PY
)"

# Version overrides are hexadecimal. TXW81X_BUILD_DATE is YYYY-MM-DD;
# otherwise Python uses SOURCE_DATE_EPOCH or the current UTC date.
: "${TXW81X_SDK_VERSION:=$TXW81X_DERIVED_SDK_VERSION}"
: "${TXW81X_SVN_VERSION:=$TXW81X_DERIVED_SVN_VERSION}"
: "${TXW81X_IMAGE_VERSION:=$TXW81X_DERIVED_IMAGE_VERSION}"

set -- "$PYTHON" "$PACKER" pack-app \
    --input "$PROJECT_DIR/project.hex" \
    --parameter-cfg "$PROJECT_DIR/parameter.bincfg" \
    --makecode-ini "$PROJECT_DIR/makecode.ini" \
    --sdk-version "$TXW81X_SDK_VERSION" \
    --svn-version "$TXW81X_SVN_VERSION" \
    --app-version "$TXW81X_IMAGE_VERSION" \
    --output "$PROJECT_DIR/APP.bin"

if [ "${TXW81X_BUILD_DATE+x}" = x ]; then
    set -- "$@" --date "$TXW81X_BUILD_DATE"
fi

printf '%s\n' 'Packaging TXW81x APP.bin natively with Python'
"$@"

printf '%s\n' 'Packaging TXW81x APP_compress.bin natively with Python'
"$PYTHON" "$PACKER" pack-ota \
    --app "$PROJECT_DIR/APP.bin" \
    --loader "$LOADER" \
    --makecode-ini "$PROJECT_DIR/makecode.ini" \
    --output "$PROJECT_DIR/APP_compress.bin"
