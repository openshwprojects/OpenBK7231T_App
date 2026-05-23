import os
import subprocess
import re
import sys
import shutil
import argparse
import time
import json
import http.client
import socket
import urllib.request
import urllib.error
import urllib.parse

# Configuration
# Auto-detect repository root based on script location
_script_dir = os.path.dirname(os.path.abspath(__file__))
TOOL_DOCKER_DIR = _script_dir if os.path.basename(_script_dir) == "docker" else os.path.join(_script_dir, "docker")
if os.path.basename(_script_dir) == "docker":
    # If we are in docker/, repo root is one level up
    REPO_ROOT = os.path.dirname(_script_dir)
else:
    # Assume script is in root
    REPO_ROOT = _script_dir

# Initialize derived paths
DOCKER_DIR = TOOL_DOCKER_DIR
PLATFORMS_DIR = os.path.join(REPO_ROOT, "platforms")
CONFIG_FILE = os.path.join(REPO_ROOT, "src", "obk_config.h")
DEFAULT_BUILD_VOLUME = "openbk_build_data"
DEFAULT_RTK_TOOLCHAIN_VOLUME = "openbk_rtk_toolchain"
DEFAULT_ESPRESSIF_TOOLS_VOLUME = "openbk_espressif_tools"
DEFAULT_ESP8266_TOOLS_VOLUME = "openbk_esp8266_tools"
DEFAULT_MBEDTLS_CACHE_VOLUME = "openbk_mbedtls_cache"
DEFAULT_CSKY_W800_CACHE_VOLUME = "openbk_csky_w800_cache"
DEFAULT_CSKY_TXW_CACHE_VOLUME = "openbk_csky_txw_cache"
DEFAULT_PIP_CACHE_VOLUME = "openbk_pip_cache"
GCC_ARM_ARCHIVE_NAME = "gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2"
DEFAULT_GCC_ARM_URLS = [
    # Prefer github.com raw URL (works with large/LFS-backed files in this mirror).
    "https://github.com/DeDaMrAzR/ghpages/raw/refs/heads/main/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2",
    # Keep blob/raw.githubusercontent fallbacks for compatibility.
    "https://github.com/DeDaMrAzR/ghpages/blob/main/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2",
    "https://raw.githubusercontent.com/DeDaMrAzR/ghpages/main/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2",
]

def update_paths(src_dir):
    """
    Updates global path variables based on the provided source directory.
    If src_dir is not provided, paths remain as initialized at module load.
    """
    global REPO_ROOT, DOCKER_DIR, PLATFORMS_DIR, CONFIG_FILE
    DOCKER_DIR = TOOL_DOCKER_DIR
    if src_dir:
        REPO_ROOT = os.path.abspath(src_dir)
    PLATFORMS_DIR = os.path.join(REPO_ROOT, "platforms")
    CONFIG_FILE = os.path.join(REPO_ROOT, "src", "obk_config.h")


def _normalize_github_blob_url(url):
    """
    Converts GitHub blob URLs to direct raw download URLs.
    """
    match = re.match(r"^https://github\.com/([^/]+)/([^/]+)/blob/([^/]+)/(.+)$", url)
    if match:
        owner, repo, ref, path = match.groups()
        return f"https://github.com/{owner}/{repo}/raw/refs/heads/{ref}/{path}"
    return url


def _is_valid_gcc_archive(path):
    """
    Basic sanity checks for the required tar.bz2 toolchain archive.
    Rejects tiny placeholder/LFS-pointer-like files and non-bzip payloads.
    """
    try:
        if not os.path.isfile(path):
            return False
        size = os.path.getsize(path)
        # Real archive is ~100MB. Keep threshold low but enough to reject pointers/html stubs.
        if size < 1024 * 1024:
            return False
        with open(path, "rb") as f:
            sig = f.read(3)
        # bzip2 signature
        return sig == b"BZh"
    except OSError:
        return False


def ensure_gcc_arm_archive():
    """
    Ensures docker/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 exists.
    If missing, attempts to download it from mirror URLs.
    """
    archive_path = os.path.join(DOCKER_DIR, GCC_ARM_ARCHIVE_NAME)
    if _is_valid_gcc_archive(archive_path):
        return
    if os.path.isfile(archive_path):
        print(f"Existing {GCC_ARM_ARCHIVE_NAME} looks invalid, re-downloading...")

    # Allow overriding URL from environment for custom mirrors.
    env_url = os.environ.get("OPENBK_GCC_ARM_URL", "").strip()
    urls = []
    if env_url:
        urls.append(env_url)
    urls.extend(DEFAULT_GCC_ARM_URLS)

    last_error = None
    for raw_url in urls:
        url = _normalize_github_blob_url(raw_url)
        tmp_path = archive_path + ".tmp"
        try:
            print(f"GCC archive missing, downloading from: {url}")
            with urllib.request.urlopen(url, timeout=60) as resp, open(tmp_path, "wb") as out_f:
                shutil.copyfileobj(resp, out_f)

            if not _is_valid_gcc_archive(tmp_path):
                raise RuntimeError("Downloaded file is not a valid toolchain tar.bz2 archive")

            os.replace(tmp_path, archive_path)
            print(f"Saved {GCC_ARM_ARCHIVE_NAME} to {archive_path}")
            return
        except Exception as e:
            last_error = e
            if os.path.exists(tmp_path):
                try:
                    os.remove(tmp_path)
                except OSError:
                    pass
            print(f"Warning: failed to download from {url}: {e}", file=sys.stderr)

    raise RuntimeError(
        f"Missing required archive '{GCC_ARM_ARCHIVE_NAME}' in {DOCKER_DIR} "
        f"and automatic download failed. Last error: {last_error}"
    )


# Unified Platform Definition
# Key: User-friendly name (Menu Item)
# Value: dict with:
#   - 'target': The exact Makefile target (e.g. OpenBK7231N)
#   - 'macros': List of C macros active for this platform (for driver filtering)
PLATFORMS = {
    # Beken
    "BK7231N":  {"target": "OpenBK7231N",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231N", "PLATFORM_BK7231N | PLATFORM_BEKEN_NEW"]},
    "BK7231N_ALT": {"target": "OpenBK7231N_ALT", "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231N", "PLATFORM_BEKEN_NEW"]},
    "BK7231T":  {"target": "OpenBK7231T",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231T"]},
    "BK7231T_ALT": {"target": "OpenBK7231T_ALT", "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231T", "PLATFORM_BEKEN_NEW"]},
    "BK7231U":  {"target": "OpenBK7231U",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231U"]},
    "BK7238":   {"target": "OpenBK7238",   "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7238"]},
    "BK7252":   {"target": "OpenBK7252",   "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7252"]},
    "BK7252N":  {"target": "OpenBK7252N",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7252N"]},
    
    # ESP-IDF (All share PLATFORM_ESPIDF for driver config usually)
    "ESP32":    {"target": "OpenESP32",    "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C2":  {"target": "OpenESP32C2",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C3":  {"target": "OpenESP32C3",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C5":  {"target": "OpenESP32C5",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C6":  {"target": "OpenESP32C6",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C61": {"target": "OpenESP32C61", "macros": ["PLATFORM_ESPIDF"]},
    "ESP32S2":  {"target": "OpenESP32S2",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32S3":  {"target": "OpenESP32S3",  "macros": ["PLATFORM_ESPIDF"]},
    
    # Other
    "ESP8266":  {"target": "OpenESP8266",  "macros": ["PLATFORM_ESP8266"]},
    "BL602":    {"target": "OpenBL602",    "macros": ["PLATFORM_BL602"]},
    "W800":     {"target": "OpenW800",     "macros": ["PLATFORM_W800"]},
    "W600":     {"target": "OpenW600",     "macros": ["PLATFORM_W600"]},
    "ECR6600":  {"target": "OpenECR6600",  "macros": ["PLATFORM_ECR6600"]},
    "LN882H":   {"target": "OpenLN882H",   "macros": ["PLATFORM_LN882H"]},
    "LN8825":   {"target": "OpenLN8825",   "macros": ["PLATFORM_LN8825"]},
    "TR6260":   {"target": "OpenTR6260",   "macros": ["PLATFORM_TR6260"]},
    "RDA5981":  {"target": "OpenRDA5981",  "macros": ["PLATFORM_RDA5981"]},
    "TXW81X":   {"target": "OpenTXW81X",   "macros": ["PLATFORM_TXW81X"]},
    
    # Realtek
    "RTL8710A":  {"target": "OpenRTL8710A",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8710A"]},
    "RTL8710B":  {"target": "OpenRTL8710B",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8710B"]},
    "RTL8720D":  {"target": "OpenRTL8720D",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8720D"]},
    "RTL8720E":  {"target": "OpenRTL8720E",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8720E"]},
    "RTL8721DA": {"target": "OpenRTL8721DA", "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8721DA"]},
    "RTL87X0C":  {"target": "OpenRTL87X0C",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL87X0C"]},
    
    # XR
    "XR809":     {"target": "OpenXR809",     "macros": ["PLATFORM_XR809"]},
    "XR806":     {"target": "OpenXR806",     "macros": ["PLATFORM_XR806"]},
    "XR872":     {"target": "OpenXR872",     "macros": ["PLATFORM_XR872"]},
}



def update_paths(src_dir):
    """
    Updates global path variables based on the provided source directory.
    If src_dir is not provided, paths remain as initialized at module load.
    """
    global REPO_ROOT, DOCKER_DIR, PLATFORMS_DIR, CONFIG_FILE
    DOCKER_DIR = TOOL_DOCKER_DIR
    if src_dir:
        REPO_ROOT = os.path.abspath(src_dir)
    PLATFORMS_DIR = os.path.join(REPO_ROOT, "platforms")
    CONFIG_FILE = os.path.join(REPO_ROOT, "src", "obk_config.h")


def _normalize_github_blob_url(url):
    """
    Converts GitHub blob URLs to direct raw download URLs.
    """
    match = re.match(r"^https://github\.com/([^/]+)/([^/]+)/blob/([^/]+)/(.+)$", url)
    if match:
        owner, repo, ref, path = match.groups()
        return f"https://github.com/{owner}/{repo}/raw/refs/heads/{ref}/{path}"
    return url


def _is_valid_gcc_archive(path):
    """
    Basic sanity checks for the required tar.bz2 toolchain archive.
    Rejects tiny placeholder/LFS-pointer-like files and non-bzip payloads.
    """
    try:
        if not os.path.isfile(path):
            return False
        size = os.path.getsize(path)
        # Real archive is ~100MB. Keep threshold low but enough to reject pointers/html stubs.
        if size < 1024 * 1024:
            return False
        with open(path, "rb") as f:
            sig = f.read(3)
        # bzip2 signature
        return sig == b"BZh"
    except OSError:
        return False


def ensure_gcc_arm_archive():
    """
    Ensures docker/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 exists.
    If missing, attempts to download it from mirror URLs.
    """
    archive_path = os.path.join(DOCKER_DIR, GCC_ARM_ARCHIVE_NAME)
    if _is_valid_gcc_archive(archive_path):
        return
    if os.path.isfile(archive_path):
        print(f"Existing {GCC_ARM_ARCHIVE_NAME} looks invalid, re-downloading...")

    # Allow overriding URL from environment for custom mirrors.
    env_url = os.environ.get("OPENBK_GCC_ARM_URL", "").strip()
    urls = []
    if env_url:
        urls.append(env_url)
    urls.extend(DEFAULT_GCC_ARM_URLS)

    last_error = None
    for raw_url in urls:
        url = _normalize_github_blob_url(raw_url)
        tmp_path = archive_path + ".tmp"
        try:
            print(f"GCC archive missing, downloading from: {url}")
            with urllib.request.urlopen(url, timeout=60) as resp, open(tmp_path, "wb") as out_f:
                shutil.copyfileobj(resp, out_f)

            if not _is_valid_gcc_archive(tmp_path):
                raise RuntimeError("Downloaded file is not a valid toolchain tar.bz2 archive")

            os.replace(tmp_path, archive_path)
            print(f"Saved {GCC_ARM_ARCHIVE_NAME} to {archive_path}")
            return
        except Exception as e:
            last_error = e
            if os.path.exists(tmp_path):
                try:
                    os.remove(tmp_path)
                except OSError:
                    pass
            print(f"Warning: failed to download from {url}: {e}", file=sys.stderr)

    raise RuntimeError(
        f"Missing required archive '{GCC_ARM_ARCHIVE_NAME}' in {DOCKER_DIR} "
        f"and automatic download failed. Last error: {last_error}"
    )


# Unified Platform Definition
# Key: User-friendly name (Menu Item)
# Value: dict with:
#   - 'target': The exact Makefile target (e.g. OpenBK7231N)
#   - 'macros': List of C macros active for this platform (for driver filtering)
PLATFORMS = {
    # Beken
    "BK7231N":  {"target": "OpenBK7231N",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231N", "PLATFORM_BK7231N | PLATFORM_BEKEN_NEW"]},
    "BK7231N_ALT": {"target": "OpenBK7231N_ALT", "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231N", "PLATFORM_BEKEN_NEW"]},
    "BK7231T":  {"target": "OpenBK7231T",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231T"]},
    "BK7231T_ALT": {"target": "OpenBK7231T_ALT", "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231T", "PLATFORM_BEKEN_NEW"]},
    "BK7231U":  {"target": "OpenBK7231U",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231U"]},
    "BK7238":   {"target": "OpenBK7238",   "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7238"]},
    "BK7252":   {"target": "OpenBK7252",   "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7252"]},
    "BK7252N":  {"target": "OpenBK7252N",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7252N"]},
    
    # ESP-IDF (All share PLATFORM_ESPIDF for driver config usually)
    "ESP32":    {"target": "OpenESP32",    "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C2":  {"target": "OpenESP32C2",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C3":  {"target": "OpenESP32C3",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C5":  {"target": "OpenESP32C5",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C6":  {"target": "OpenESP32C6",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C61": {"target": "OpenESP32C61", "macros": ["PLATFORM_ESPIDF"]},
    "ESP32S2":  {"target": "OpenESP32S2",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32S3":  {"target": "OpenESP32S3",  "macros": ["PLATFORM_ESPIDF"]},
    
    # Other
    "ESP8266":  {"target": "OpenESP8266",  "macros": ["PLATFORM_ESP8266"]},
    "BL602":    {"target": "OpenBL602",    "macros": ["PLATFORM_BL602"]},
    "W800":     {"target": "OpenW800",     "macros": ["PLATFORM_W800"]},
    "W600":     {"target": "OpenW600",     "macros": ["PLATFORM_W600"]},
    "ECR6600":  {"target": "OpenECR6600",  "macros": ["PLATFORM_ECR6600"]},
    "LN882H":   {"target": "OpenLN882H",   "macros": ["PLATFORM_LN882H"]},
    "LN8825":   {"target": "OpenLN8825",   "macros": ["PLATFORM_LN8825"]},
    "TR6260":   {"target": "OpenTR6260",   "macros": ["PLATFORM_TR6260"]},
    "RDA5981":  {"target": "OpenRDA5981",  "macros": ["PLATFORM_RDA5981"]},
    "TXW81X":   {"target": "OpenTXW81X",   "macros": ["PLATFORM_TXW81X"]},
    
    # Realtek
    "RTL8710A":  {"target": "OpenRTL8710A",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8710A"]},
    "RTL8710B":  {"target": "OpenRTL8710B",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8710B"]},
    "RTL8720D":  {"target": "OpenRTL8720D",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8720D"]},
    "RTL8720E":  {"target": "OpenRTL8720E",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8720E"]},
    "RTL8721DA": {"target": "OpenRTL8721DA", "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8721DA"]},
    "RTL87X0C":  {"target": "OpenRTL87X0C",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL87X0C"]},
    
    # XR
    "XR809":     {"target": "OpenXR809",     "macros": ["PLATFORM_XR809"]},
    "XR806":     {"target": "OpenXR806",     "macros": ["PLATFORM_XR806"]},
    "XR872":     {"target": "OpenXR872",     "macros": ["PLATFORM_XR872"]},
}



def update_paths(src_dir):
    """
    Updates global path variables based on the provided source directory.
    If src_dir is not provided, paths remain as initialized at module load.
    """
    global REPO_ROOT, DOCKER_DIR, PLATFORMS_DIR, CONFIG_FILE
    DOCKER_DIR = TOOL_DOCKER_DIR
    if src_dir:
        REPO_ROOT = os.path.abspath(src_dir)
    PLATFORMS_DIR = os.path.join(REPO_ROOT, "platforms")
    CONFIG_FILE = os.path.join(REPO_ROOT, "src", "obk_config.h")


def _normalize_github_blob_url(url):
    """
    Converts GitHub blob URLs to direct raw download URLs.
    """
    match = re.match(r"^https://github\.com/([^/]+)/([^/]+)/blob/([^/]+)/(.+)$", url)
    if match:
        owner, repo, ref, path = match.groups()
        return f"https://github.com/{owner}/{repo}/raw/refs/heads/{ref}/{path}"
    return url


def _is_valid_gcc_archive(path):
    """
    Basic sanity checks for the required tar.bz2 toolchain archive.
    Rejects tiny placeholder/LFS-pointer-like files and non-bzip payloads.
    """
    try:
        if not os.path.isfile(path):
            return False
        size = os.path.getsize(path)
        # Real archive is ~100MB. Keep threshold low but enough to reject pointers/html stubs.
        if size < 1024 * 1024:
            return False
        with open(path, "rb") as f:
            sig = f.read(3)
        # bzip2 signature
        return sig == b"BZh"
    except OSError:
        return False


def ensure_gcc_arm_archive():
    """
    Ensures docker/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 exists.
    If missing, attempts to download it from mirror URLs.
    """
    archive_path = os.path.join(DOCKER_DIR, GCC_ARM_ARCHIVE_NAME)
    if _is_valid_gcc_archive(archive_path):
        return
    if os.path.isfile(archive_path):
        print(f"Existing {GCC_ARM_ARCHIVE_NAME} looks invalid, re-downloading...")

    # Allow overriding URL from environment for custom mirrors.
    env_url = os.environ.get("OPENBK_GCC_ARM_URL", "").strip()
    urls = []
    if env_url:
        urls.append(env_url)
    urls.extend(DEFAULT_GCC_ARM_URLS)

    last_error = None
    for raw_url in urls:
        url = _normalize_github_blob_url(raw_url)
        tmp_path = archive_path + ".tmp"
        try:
            print(f"GCC archive missing, downloading from: {url}")
            with urllib.request.urlopen(url, timeout=60) as resp, open(tmp_path, "wb") as out_f:
                shutil.copyfileobj(resp, out_f)

            if not _is_valid_gcc_archive(tmp_path):
                raise RuntimeError("Downloaded file is not a valid toolchain tar.bz2 archive")

            os.replace(tmp_path, archive_path)
            print(f"Saved {GCC_ARM_ARCHIVE_NAME} to {archive_path}")
            return
        except Exception as e:
            last_error = e
            if os.path.exists(tmp_path):
                try:
                    os.remove(tmp_path)
                except OSError:
                    pass
            print(f"Warning: failed to download from {url}: {e}", file=sys.stderr)

    raise RuntimeError(
        f"Missing required archive '{GCC_ARM_ARCHIVE_NAME}' in {DOCKER_DIR} "
        f"and automatic download failed. Last error: {last_error}"
    )


# Unified Platform Definition
# Key: User-friendly name (Menu Item)
# Value: dict with:
#   - 'target': The exact Makefile target (e.g. OpenBK7231N)
#   - 'macros': List of C macros active for this platform (for driver filtering)
PLATFORMS = {
    # Beken
    "BK7231N":  {"target": "OpenBK7231N",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231N", "PLATFORM_BK7231N | PLATFORM_BEKEN_NEW"]},
    "BK7231T":  {"target": "OpenBK7231T",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231T"]},
    "BK7231U":  {"target": "OpenBK7231U",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7231U"]},
    "BK7238":   {"target": "OpenBK7238",   "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7238"]},
    "BK7252":   {"target": "OpenBK7252",   "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7252"]},
    "BK7252N":  {"target": "OpenBK7252N",  "macros": ["PLATFORM_BEKEN", "PLATFORM_BK7252N"]},
    
    # ESP-IDF (All share PLATFORM_ESPIDF for driver config usually)
    "ESP32":    {"target": "OpenESP32",    "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C2":  {"target": "OpenESP32C2",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C3":  {"target": "OpenESP32C3",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C5":  {"target": "OpenESP32C5",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C6":  {"target": "OpenESP32C6",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32C61": {"target": "OpenESP32C61", "macros": ["PLATFORM_ESPIDF"]},
    "ESP32S2":  {"target": "OpenESP32S2",  "macros": ["PLATFORM_ESPIDF"]},
    "ESP32S3":  {"target": "OpenESP32S3",  "macros": ["PLATFORM_ESPIDF"]},
    
    # Other
    "ESP8266":  {"target": "OpenESP8266",  "macros": ["PLATFORM_ESP8266"]},
    "BL602":    {"target": "OpenBL602",    "macros": ["PLATFORM_BL602"]},
    "W800":     {"target": "OpenW800",     "macros": ["PLATFORM_W800"]},
    "W600":     {"target": "OpenW600",     "macros": ["PLATFORM_W600"]},
    "ECR6600":  {"target": "OpenECR6600",  "macros": ["PLATFORM_ECR6600"]},
    "LN882H":   {"target": "OpenLN882H",   "macros": ["PLATFORM_LN882H"]},
    "LN8825":   {"target": "OpenLN8825",   "macros": ["PLATFORM_LN8825"]},
    "TR6260":   {"target": "OpenTR6260",   "macros": ["PLATFORM_TR6260"]},
    "RDA5981":  {"target": "OpenRDA5981",  "macros": ["PLATFORM_RDA5981"]},
    "TXW81X":   {"target": "OpenTXW81X",   "macros": ["PLATFORM_TXW81X"]},
    
    # Realtek
    "RTL8710A":  {"target": "OpenRTL8710A",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8710A"]},
    "RTL8710B":  {"target": "OpenRTL8710B",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8710B"]},
    "RTL8720D":  {"target": "OpenRTL8720D",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8720D"]},
    "RTL8720E":  {"target": "OpenRTL8720E",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8720E"]},
    "RTL8721DA": {"target": "OpenRTL8721DA", "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL8721DA"]},
    "RTL87X0C":  {"target": "OpenRTL87X0C",  "macros": ["PLATFORM_REALTEK", "PLATFORM_RTL87X0C"]},
    
    # XR
    "XR809":     {"target": "OpenXR809",     "macros": ["PLATFORM_XR809"]},
    "XR806":     {"target": "OpenXR806",     "macros": ["PLATFORM_XR806"]},
    "XR872":     {"target": "OpenXR872",     "macros": ["PLATFORM_XR872"]},
}

# OTA-capable targets and expected firmware file suffixes.
# Keep this list conservative and explicit to prevent accidental cross-platform uploads.
OTA_TARGET_EXTENSIONS = {
    "OpenBK7231N": ".rbl",
    "OpenBK7231N_ALT": ".rbl",
    "OpenBK7231T": ".rbl",
    "OpenBK7231T_ALT": ".rbl",
    "OpenBK7231U": ".rbl",
    "OpenBK7238": ".rbl",
    "OpenBK7252": ".rbl",
    "OpenBK7252N": ".rbl",
    "OpenBL602": ".bin.xz.ota",
    "OpenESP8266": ".img",
    "OpenESP32": ".img",
    "OpenESP32C2": ".img",
    "OpenESP32C3": ".img",
    "OpenESP32C6": ".img",
    "OpenESP32S2": ".img",
    "OpenESP32S3": ".img",
    "OpenLN882H": "_OTA.bin",
    "OpenTR6260": ".bin",
    "OpenECR6600": "_ota.img",
    "OpenW600": ".img",
    "OpenW800": ".img",
    "OpenRTL87X0C": ".img",
    "OpenRTL8710B": ".img",
    "OpenXR806": "_ota.img",
}

# All flash sizes supported by the build system.
ALL_FLASH_SIZES = ["1MB", "2MB", "4MB", "8MB", "16MB"]

# Flash sizes that are actually valid per platform, derived from partition matrix tests.
# Platforms NOT listed here support all flash sizes (their SDKs use fixed linker scripts
# and silently ignore FLASH_SIZE, so all sizes build successfully).
# Only ESP-IDF platforms with mandatory partition tables are restricted.
FLASH_SIZE_RESTRICTIONS = {
    # ESP32C3 requires a partitions.csv; OBK only provides layouts from 4MB upward.
    # 1MB and 2MB builds fail at link stage with partition table errors.
    "ESP32C3": ["4MB", "8MB", "16MB"],
}


def get_valid_flash_sizes(platform_name):
    """
    Returns the list of valid flash sizes for the given platform.
    Platforms not in FLASH_SIZE_RESTRICTIONS support all sizes.
    """
    return FLASH_SIZE_RESTRICTIONS.get(platform_name, ALL_FLASH_SIZES)


def get_platforms():
    return sorted(PLATFORMS.keys())


def get_target_platform(platform_name):
    if platform_name in PLATFORMS:
        return PLATFORMS[platform_name]["target"]
    return platform_name


def is_platform_ota_capable(platform_name):
    target = get_target_platform(platform_name)
    return target in OTA_TARGET_EXTENSIONS


def get_ota_extension_for_platform(platform_name):
    target = get_target_platform(platform_name)
    return OTA_TARGET_EXTENSIONS.get(target)


def _normalize_platform_token(value):
    if not value:
        return ""
    token = re.sub(r"[^A-Za-z0-9]", "", str(value).upper())
    if token.startswith("OPEN"):
        token = token[4:]
    return token


def find_ota_image_file(output_dir, platform_name):
    """
    Finds the newest OTA artifact matching selected platform and configured extension.
    Searches recursively under output_dir.
    """
    target = get_target_platform(platform_name)
    ext = get_ota_extension_for_platform(platform_name)
    if not ext:
        raise RuntimeError(f"Platform '{platform_name}' ({target}) is not marked OTA-capable.")

    root = os.path.abspath(output_dir)
    if not os.path.isdir(root):
        raise RuntimeError(f"Output directory does not exist: {root}")

    target_lower = target.lower()
    ext_lower = ext.lower()
    matches = []
    for dirpath, _, filenames in os.walk(root):
        for name in filenames:
            lname = name.lower()
            if lname.startswith(target_lower) and lname.endswith(ext_lower):
                full = os.path.join(dirpath, name)
                try:
                    mtime = os.path.getmtime(full)
                except OSError:
                    continue
                matches.append((mtime, full))

    if not matches:
        raise RuntimeError(
            f"No OTA file found for {target} with extension '{ext}' under:\n{root}"
        )

    matches.sort(key=lambda x: x[0], reverse=True)
    return matches[0][1]


def query_device_platform(host, timeout=8):
    """
    Queries OBK device for platform/chipset info.
    Returns a dict with best-effort fields:
      - source: status2|api_info|none
      - raw_platform: e.g. BK7231N / OpenBK7231N
      - token: normalized token, e.g. BK7231N
      - raw: raw response snippet
    """
    host = (host or "").strip()
    if not host:
        raise RuntimeError("Device host is empty.")

    base = f"http://{host}"

    # 1) Tasmota-like status endpoint
    try:
        url = f"{base}/cm?cmnd=status%202"
        with urllib.request.urlopen(url, timeout=timeout) as resp:
            txt = resp.read().decode("utf-8", errors="replace")
        obj = json.loads(txt)
        statusfwr = obj.get("StatusFWR", {}) if isinstance(obj, dict) else {}
        raw_platform = statusfwr.get("Hardware") or statusfwr.get("Build") or ""
        return {
            "source": "status2",
            "raw_platform": raw_platform,
            "token": _normalize_platform_token(raw_platform),
            "raw": txt[:400],
        }
    except Exception:
        pass

    # 2) OBK REST info endpoint
    try:
        url = f"{base}/api/info"
        with urllib.request.urlopen(url, timeout=timeout) as resp:
            txt = resp.read().decode("utf-8", errors="replace")
        obj = json.loads(txt)
        raw_platform = ""
        if isinstance(obj, dict):
            raw_platform = obj.get("chipset") or obj.get("build") or ""
        return {
            "source": "api_info",
            "raw_platform": raw_platform,
            "token": _normalize_platform_token(raw_platform),
            "raw": txt[:400],
        }
    except Exception:
        pass

    return {"source": "none", "raw_platform": "", "token": "", "raw": ""}


def is_device_platform_compatible(platform_name, device_platform_token):
    """
    Basic compatibility check between selected target and detected device token.
    """
    target_token = _normalize_platform_token(get_target_platform(platform_name))
    device_token = _normalize_platform_token(device_platform_token)
    if not target_token or not device_token:
        return False
    if target_token == device_token:
        return True
    # Treat *_ALT targets as OTA-compatible with their base chip token.
    if target_token.endswith("ALT"):
        target_token = target_token[:-3]
    if device_token.endswith("ALT"):
        device_token = device_token[:-3]
    return target_token == device_token


def _upload_ota_file_rtl_chunked(
    host,
    data,
    timeout=30,
    first_chunk_size=256,
    first_chunk_delay_sec=0.35,
    chunk_size=512,
    chunk_delay_sec=0.004,
):
    """
    RTL87X0C migration helper:
    send OTA body in small chunks so legacy firmware exits pre-buffer loop early
    and continues OTA streaming from socket.
    """
    conn = None
    try:
        conn = http.client.HTTPConnection(host, timeout=timeout)
        conn.connect()
        try:
            conn.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        except Exception:
            pass
        conn.putrequest("POST", "/api/ota")
        conn.putheader("Content-Type", "application/octet-stream")
        conn.putheader("Content-Length", str(len(data)))
        conn.putheader("Connection", "close")
        conn.endheaders()

        view = memoryview(data)
        total = len(data)
        sent = 0

        # Prime parser with a small body fragment and a pause.
        # This helps legacy RTL firmware break out of pre-buffer growth loop
        # and switch to OTA streaming from socket.
        prime = min(first_chunk_size, total)
        if prime > 0:
            conn.send(view[:prime])
            sent = prime
            if first_chunk_delay_sec > 0:
                time.sleep(first_chunk_delay_sec)

        while sent < total:
            next_sent = min(sent + chunk_size, total)
            conn.send(view[sent:next_sent])
            sent = next_sent
            if chunk_delay_sec > 0:
                time.sleep(chunk_delay_sec)

        resp = conn.getresponse()
        status = getattr(resp, "status", 200)
        body = resp.read().decode("utf-8", errors="replace")
        return {"ok": 200 <= status < 300, "status": status, "body": body[:500]}
    except Exception as e:
        return {"ok": False, "status": 0, "body": str(e)}
    finally:
        if conn is not None:
            try:
                conn.close()
            except Exception:
                pass


def upload_ota_file(host, ota_file_path, timeout=30, device_platform_token=""):
    """
    Uploads OTA image as raw binary body to /api/ota.
    """
    host = (host or "").strip()
    if not host:
        raise RuntimeError("Device host is empty.")
    ota_file_path = os.path.abspath(ota_file_path)
    if not os.path.isfile(ota_file_path):
        raise RuntimeError(f"OTA file not found: {ota_file_path}")

    with open(ota_file_path, "rb") as f:
        data = f.read()

    token = _normalize_platform_token(device_platform_token or "")
    if token == "RTL87X0C":
        res = _upload_ota_file_rtl_chunked(host, data, timeout=timeout)
        if not res.get("ok"):
            # One slower fallback pass for stubborn legacy builds.
            res = _upload_ota_file_rtl_chunked(
                host,
                data,
                timeout=max(timeout, 60),
                first_chunk_size=128,
                first_chunk_delay_sec=0.6,
                chunk_size=256,
                chunk_delay_sec=0.01,
            )
            res["rtl_fallback"] = True
        else:
            res["rtl_fallback"] = False
        return res

    url = f"http://{host}/api/ota"
    req = urllib.request.Request(url=url, data=data, method="POST")
    req.add_header("Content-Type", "application/octet-stream")
    req.add_header("Content-Length", str(len(data)))
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            status = getattr(resp, "status", 200)
            body = resp.read().decode("utf-8", errors="replace")
        return {"ok": True, "status": status, "body": body[:500]}
    except urllib.error.HTTPError as e:
        try:
            body = e.read().decode("utf-8", errors="replace")
        except Exception:
            body = str(e)
        return {"ok": False, "status": getattr(e, "code", 0), "body": body[:500]}
    except Exception as e:
        return {"ok": False, "status": 0, "body": str(e)}


def send_obk_command(host, command, timeout=10):
    """
    Sends a command using OBK command endpoint:
      /cm?cmnd=<command>
    """
    host = (host or "").strip()
    if not host:
        raise RuntimeError("Device host is empty.")
    command = str(command or "").strip()
    if not command:
        raise RuntimeError("Command is empty.")

    encoded = urllib.parse.quote(command, safe="")
    url = f"http://{host}/cm?cmnd={encoded}"
    req = urllib.request.Request(url=url, method="GET")

    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            status = getattr(resp, "status", 200)
            body = resp.read().decode("utf-8", errors="replace")
        return {"ok": True, "status": status, "body": body[:500]}
    except urllib.error.HTTPError as e:
        try:
            body = e.read().decode("utf-8", errors="replace")
        except Exception:
            body = str(e)
        return {"ok": False, "status": getattr(e, "code", 0), "body": body[:500]}
    except Exception as e:
        return {"ok": False, "status": 0, "body": str(e)}


def restart_device(host, timeout=10):
    """
    Sends 'Restart 1' command. Some firmware versions may reset the HTTP socket quickly;
    in that case a connection error can still mean the reboot was accepted.
    """
    res = send_obk_command(host, "Restart 1", timeout=timeout)
    if res.get("ok"):
        return res

    body = (res.get("body") or "").lower()
    likely_reboot_disconnect = (
        "connection" in body or "timed out" in body or "reset" in body
    )
    if likely_reboot_disconnect:
        return {"ok": True, "status": res.get("status", 0), "body": res.get("body", "")}

    return res


def restart_device_after_ota(host, device_platform_token="", timeout=10):
    """
    Sends Restart 1 after OTA upload, with RTL87X0C-specific timing workaround.

    On RTL87X0C, OTA finalize path may need a short settle time before command
    endpoint is reliable again. Keep behavior unchanged for all other platforms.
    """
    token = _normalize_platform_token(device_platform_token or "")
    is_rtl87x0c = (token == "RTL87X0C")

    # Default flow for all platforms except RTL87X0C.
    if not is_rtl87x0c:
        res = restart_device(host, timeout=timeout)
        res["rtl_delayed"] = False
        res["attempts"] = 1
        return res

    # RTL87X0C-only: allow OTA endpoint to settle before issuing restart.
    time.sleep(2.0)
    attempts = 3
    last = {"ok": False, "status": 0, "body": ""}
    for attempt in range(1, attempts + 1):
        last = restart_device(host, timeout=timeout)
        if last.get("ok"):
            last["rtl_delayed"] = True
            last["attempts"] = attempt
            return last
        if attempt < attempts:
            time.sleep(1.0)

    last["rtl_delayed"] = True
    last["attempts"] = attempts
    return last

def get_available_drivers(platform_name):
    """
    Parses obk_config.h to find drivers available for the specific platform.
    Returns a list of driver names.
    
    Logic:
    1. Base drivers (no #if protection)
    2. Drivers in #if blocks matching the platform's macros
    """
    if not os.path.exists(CONFIG_FILE):
        return []

    # Get macros active for this platform
    if platform_name in PLATFORMS:
        active_macros = set(PLATFORMS[platform_name]["macros"])
    else:
        active_macros = set()
    active_macros.add("COMMON") # Virtual macro for root level

    drivers = {}
    
    # Parser State
    # Stack of active contexts. True = we are inside an active block. False = inactive.
    # We start with [True] because root level is always active.
    active_stack = [True] 

    with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # Regex Helper
    # Matches: #if PLATFORM_... or #elif PLATFORM_...
    # We capture the directive (if/elif) and the condition
    cond_regex = re.compile(r'^\s*#\s*(if|elif)\s+(.*)')
    endif_regex = re.compile(r'^\s*#\s*endif')
    else_regex = re.compile(r'^\s*#\s*else')
    
    # Matches driver definitions: #define ENABLE_DRIVER_... 1  OR  #define ENABLE_NTP 0
    driver_regex = re.compile(r'^\s*#\s*define\s+(ENABLE_\w+)\s+(\d+)')
    
    # Special non-driver names we want to capture (visible in GUI)
    special_names = ["ENABLE_NTP", "ENABLE_I2C", "ENABLE_OBK_BERRY"]

    # Features forced to 1 by default (Hidden from GUI, but managed)
    FORCED_FEATURES = [
        "ENABLE_OBK_SCRIPTING",
        "ENABLE_LITTLEFS",
        "ENABLE_MQTT",
        "ENABLE_HA_DISCOVERY",
        "ENABLE_TASMOTA_JSON",
        "ENABLE_TASMOTADEVICEGROUPS"
    ]

    for line in lines:
        line = line.strip()
        
        # 1. Driver Definition
        match = driver_regex.search(line)
        if match:
            # If current block is active, add driver
            if all(active_stack):
                d_name = match.group(1)
                if d_name.startswith("ENABLE_DRIVER_") or d_name in special_names:
                    drivers[d_name] = int(match.group(2))
            continue
            
        # 2. Conditionals
        # Check #endif
        if endif_regex.match(line):
            if len(active_stack) > 1:
                active_stack.pop()
            continue
            
        # Check #if / #elif
        match = cond_regex.match(line)
        if match:
            directive = match.group(1)
            condition = match.group(2)
            
            # Simple heuristic: Does the condition string contain any of our active macros?
            # e.g. condition="PLATFORM_BEKEN" -> True
            # e.g. condition="PLATFORM_W800" -> False
            # This handles "PLATFORM_BK7231N || PLATFORM_BEKEN_NEW" roughly correctly if one matches.
            
            # Remove comments/trailing chars
            clean_cond = condition.split('//')[0].strip()
            
            # Check if this condition is "relevant" (contains PLATFORM_ or WINDOWS)
            is_platform_check = "PLATFORM_" in clean_cond or "WINDOWS" in clean_cond
            
            if not is_platform_check:
                # Treat non-platform checks (e.g. #if ENABLE_DRIVER_X) as "Pass through"
                # If parent is active, we assume this inner block MIGHT be active, 
                # but for 'available drivers' discovery, we usually want to see them all 
                # unless they are explicitly platform-gated.
                # For simplicity in this tool, we ignore non-platform guards and keep current state.
                # (A full C preprocessor interaction is out of scope)
                if directive == "if":
                    active_stack.append(active_stack[-1]) # Push same state
                # elif is ignored here because we didn't push a new state type
                continue

            # Evaluate condition against our active macros
            # Logic: If any active macro is present in the condition string, consider it True.
            # This works for "PLATFORM_A || PLATFORM_B".
            # It fails for "!PLATFORM_A" (treats as True), but that's rare in top-level platform blocks here.
            is_match = any(m in clean_cond for m in active_macros)
            
            if directive == "if":
                active_stack.append(is_match)
            elif directive == "elif":
                # elif replaces the top of the stack
                if len(active_stack) > 1:
                    active_stack.pop()
                    active_stack.append(is_match)
            
    return drivers

# Dependency mapping for drivers that require other modules
# Key: Driver name, Value: List of required dependencies
DRIVER_DEPENDENCIES = {
    # BL-family power metering drivers all require BL_SHARED, which requires NTP
    "ENABLE_DRIVER_BL0937": ["ENABLE_NTP"],
    "ENABLE_DRIVER_BL0942": ["ENABLE_NTP"],
    "ENABLE_DRIVER_BL0942SPI": ["ENABLE_NTP"],
    "ENABLE_DRIVER_CSE7766": ["ENABLE_NTP"],
    "ENABLE_DRIVER_CSE7761": ["ENABLE_NTP"],
    "ENABLE_DRIVER_RN8209": ["ENABLE_NTP"],
    # HLW8112SPI is independent, no dependencies
}

def resolve_dependencies(selected_drivers):
    """
    Recursively resolve all driver dependencies.
    Returns a set including the original drivers plus all their dependencies.
    """
    resolved = set(selected_drivers)
    added = True
    
    while added:
        added = False
        for driver in list(resolved):
            if driver in DRIVER_DEPENDENCIES:
                for dep in DRIVER_DEPENDENCIES[driver]:
                    if dep not in resolved:
                        resolved.add(dep)
                        added = True
    
    return list(resolved)

def create_custom_config(selected_drivers):
    # Return a modified obk_config.h content with selected drivers enabled.
    all_drivers = resolve_dependencies(selected_drivers)

    # Read original config
    with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    new_lines = []
    driver_regex = re.compile(r'#\s*define\s+(ENABLE_\w+)\s+(\d+)')
    special_names = ["ENABLE_NTP", "ENABLE_I2C", "ENABLE_OBK_BERRY"]
    FORCED_FEATURES = [
        "ENABLE_OBK_SCRIPTING",
        "ENABLE_LITTLEFS",
        "ENABLE_MQTT",
        "ENABLE_HA_DISCOVERY",
        "ENABLE_TASMOTA_JSON",
        "ENABLE_TASMOTADEVICEGROUPS"
    ]

    for line in lines:
        match = driver_regex.search(line)
        if match:
            driver_name = match.group(1)
            is_managed = (driver_name.startswith("ENABLE_DRIVER_") or
                          driver_name in special_names or
                          driver_name in FORCED_FEATURES)
            if is_managed:
                if driver_name in FORCED_FEATURES:
                    new_lines.append(f"#define {driver_name}\t1\n")
                elif driver_name in all_drivers:
                    new_lines.append(f"#define {driver_name}\t1\n")
                else:
                    new_lines.append(f"// #define {driver_name}\t0 // Disabled by build_tool\n")
            else:
                new_lines.append(line)
        else:
            new_lines.append(line)

    return "".join(new_lines)


def check_docker_running(creationflags=0):
    """
    Checks if Docker daemon is running by attempting 'docker info'.
    Returns True if running, False otherwise.
    """
    try:
        result = subprocess.run(
            ["docker", "info"],
            capture_output=True,
            timeout=5,
            encoding='utf-8',
            errors='replace',
            creationflags=creationflags
        )
        return result.returncode == 0
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return False

def get_builder_image_mode(image_name="openbk_builder", creationflags=0):
    """
    Detects how to invoke builder image:
    - "entrypoint": image has ENTRYPOINT (new layout)
    - "legacy_cmd": image relies on CMD /build_target_platforms.sh (legacy layout)
    """
    try:
        result = subprocess.run(
            [
                "docker", "image", "inspect", image_name,
                "--format", "{{json .Config.Entrypoint}}|{{json .Config.Cmd}}"
            ],
            capture_output=True,
            timeout=5,
            encoding="utf-8",
            errors="replace",
            creationflags=creationflags,
        )
        if result.returncode != 0:
            return "entrypoint"

        out = (result.stdout or "").strip()
        if not out:
            return "entrypoint"

        entrypoint_json, _, cmd_json = out.partition("|")
        has_entrypoint = entrypoint_json not in ("", "null", "[]")
        has_legacy_cmd = "/build_target_platforms.sh" in cmd_json

        if has_entrypoint:
            return "entrypoint"
        if has_legacy_cmd:
            return "legacy_cmd"
    except (subprocess.TimeoutExpired, FileNotFoundError):
        pass
    return "entrypoint"

def build_docker_image(no_cache=False, creationflags=0):
    print("Building Docker image...")
    ensure_gcc_arm_archive()
    cmd = ["docker", "build", "-t", "openbk_builder", "."]
    if no_cache:
        cmd.insert(2, "--no-cache")
    subprocess.run(
        cmd,
        cwd=DOCKER_DIR,
        check=True,
        encoding='utf-8',
        errors='replace',
        creationflags=creationflags
    )

def run_docker_build(
    output_dir,
    platform,
    custom_config_path,
    version=None,
    clean=False,
    flash_size=None,
    volume_name=DEFAULT_BUILD_VOLUME,
    rtk_toolchain_volume=DEFAULT_RTK_TOOLCHAIN_VOLUME,
    espressif_tools_volume=DEFAULT_ESPRESSIF_TOOLS_VOLUME,
    esp8266_tools_volume=DEFAULT_ESP8266_TOOLS_VOLUME,
    mbedtls_cache_volume=DEFAULT_MBEDTLS_CACHE_VOLUME,
    csky_w800_cache_volume=DEFAULT_CSKY_W800_CACHE_VOLUME,
    csky_txw_cache_volume=DEFAULT_CSKY_TXW_CACHE_VOLUME,
    pip_cache_volume=DEFAULT_PIP_CACHE_VOLUME,
    log_file_path=None,
    stream_output=True,
    txw_packager=None,
    creationflags=0,
):
    if platform in PLATFORMS:
        target_platform = PLATFORMS[platform]["target"]
    else:
        target_platform = platform
    print(f"Running Docker build for {platform} (Target: {target_platform})...")

    abs_output_dir = os.path.abspath(output_dir)
    os.makedirs(abs_output_dir, exist_ok=True)

    cmd = [
        "docker", "run", "--rm",
        "-v", f"{REPO_ROOT}:/app/source:ro",
        "-v", f"{abs_output_dir}:/app/output",
        "-v", f"{custom_config_path}:/app/custom_config.h:ro",
        "-v", f"{volume_name}:/app/build",
    ]
    if rtk_toolchain_volume:
        cmd.extend(["-v", f"{rtk_toolchain_volume}:/opt/rtk-toolchain"])
    if espressif_tools_volume:
        cmd.extend(["-v", f"{espressif_tools_volume}:/app/espressif-cache"])
    if esp8266_tools_volume:
        cmd.extend(["-v", f"{esp8266_tools_volume}:/app/esp8266-cache"])
    if mbedtls_cache_volume:
        cmd.extend(["-v", f"{mbedtls_cache_volume}:/app/mbedtls-cache"])
    if csky_w800_cache_volume:
        cmd.extend(["-v", f"{csky_w800_cache_volume}:/app/csky-w800-cache"])
    if csky_txw_cache_volume:
        cmd.extend(["-v", f"{csky_txw_cache_volume}:/app/csky-txw-cache"])
    if pip_cache_volume:
        cmd.extend(["-v", f"{pip_cache_volume}:/app/pip-cache"])

    if version:
        cmd.extend(["-e", f"APP_VERSION={version}"])
    if clean:
        cmd.extend(["-e", "CLEAN_BUILD=1"])
    if flash_size:
        cmd.extend(["-e", f"FLASH_SIZE={flash_size}"])
        if "ESP" in target_platform:
            if flash_size in ["4MB", "8MB", "16MB"]:
                cmd.extend(["-e", "OBK_VARIANT=2"])
            else:
                cmd.extend(["-e", "OBK_VARIANT=1"])
    if txw_packager:
        cmd.extend(["-e", f"TXW_PACKAGER_MODE={txw_packager}"])

    try:
        is_dst = time.daylight and time.localtime().tm_isdst > 0
        local_tz = time.tzname[1] if is_dst else time.tzname[0]
        cmd.extend(["-e", f"TZ={local_tz}"])
    except Exception:
        pass

    image_mode = get_builder_image_mode("openbk_builder", creationflags=creationflags)
    if image_mode == "legacy_cmd":
        cmd.extend(["openbk_builder", "/build_target_platforms.sh", target_platform])
    else:
        cmd.extend(["openbk_builder", target_platform])

    start_time = time.time()
    log_handle = None
    if log_file_path:
        abs_log_path = os.path.abspath(log_file_path)
        os.makedirs(os.path.dirname(abs_log_path), exist_ok=True)
        log_handle = open(abs_log_path, "w", encoding="utf-8", errors="replace")
        log_handle.write(f"Command: {' '.join(cmd)}\n\n")

    try:
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            encoding="utf-8",
            errors="replace",
            bufsize=1,
            universal_newlines=True,
            creationflags=creationflags,
        )
        if process.stdout:
            for line in process.stdout:
                if stream_output:
                    try:
                        print(line, end="")
                    except UnicodeEncodeError:
                        safe_line = line.encode(
                            sys.stdout.encoding or "utf-8", errors="replace"
                        ).decode(sys.stdout.encoding or "utf-8", errors="replace")
                        print(safe_line, end="")
                if log_handle:
                    log_handle.write(line)
                    log_handle.flush()

        return_code = process.wait()
        if return_code != 0:
            raise subprocess.CalledProcessError(return_code, cmd)
    finally:
        if log_handle:
            log_handle.close()

    end_time = time.time()
    print(f"Build task finished in {end_time - start_time:.2f} seconds.")
    return end_time - start_time


def get_default_flash_size(platform):
    """Returns a sensible default flash size for the given platform."""
    if "ESP32" in platform:
        return "4MB"   # Safe minimum for ESP-IDF partition tables
    if "ESP8266" in platform:
        return "2MB"
    if "BK7231" in platform:
        return "2MB"
    return "2MB"


def main():
    parser = argparse.ArgumentParser(description="OpenBK7231T_App Build Tool")
    parser.add_argument("--clean", action="store_true", help="Perform a clean build (make clean) before building")
    parser.add_argument("--no-cache", action="store_true", help="Force rebuild of Docker image (ignore cache)")
    parser.add_argument("--flash-size", help="Flash size (e.g. 1MB, 2MB, 4MB)")
    parser.add_argument(
        "--txw-packager",
        choices=["auto", "wine", "fallback"],
        default="auto",
        help="OpenTXW81X post-packager mode: auto, wine (CI-like), fallback (Linux emulation)",
    )
    parser.add_argument("--src", help="Path to the repository root (needed if running from outside)")
    args = parser.parse_args()

    update_paths(args.src)

    start_time = time.time()
    print("--- OpenBK7231T_App Build Tool ---")

    # 1. Output Folder
    default_out = "test_OUT"
    output_dir = input(f"Enter Output Folder [{default_out}]: ").strip() or default_out

    # 2. Platform Selection
    platforms = get_platforms()
    print("\nAvailable Platforms:")
    for i, p in enumerate(platforms):
        print(f"{i+1}. {p}")

    try:
        p_idx = int(input("\nSelect Platform (Number): ")) - 1
        if 0 <= p_idx < len(platforms):
            selected_platform = platforms[p_idx]
        else:
            print("Invalid selection.")
            return
    except ValueError:
        print("Invalid input.")
        return

    # 3. Driver Selection (Platform Aware)
    print(f"\nScanning available drivers for {selected_platform}...")
    drivers_dict = get_available_drivers(selected_platform)
    all_drivers = sorted(drivers_dict.keys())

    selected_drivers_list = []

    if not all_drivers:
        print("No drivers definitions found (or config file missing).")
    else:
        print("\nSelect Drivers (space-separated numbers, e.g., '1 5 10', or Enter for None):")
        for i, d in enumerate(all_drivers):
            print(f"{i+1:3}. {d}")

        while True:
            drivers_input = input("\nSelection: ").strip()
            if drivers_input.lower() in ['q', 'quit', 'exit']:
                print("Exiting.")
                return
            if not drivers_input:
                break
            try:
                temp_selection = []
                normalized_input = drivers_input.replace(',', ' ')
                indices = [int(i) - 1 for i in normalized_input.split()]
                valid = True
                for idx in indices:
                    if 0 <= idx < len(all_drivers):
                        temp_selection.append(all_drivers[idx])
                    else:
                        print(f"Index {idx+1} out of range.")
                        valid = False
                if valid:
                    selected_drivers_list = temp_selection
                    break
            except ValueError:
                print("Invalid input. Please enter numbers separated by spaces (e.g. '1 5') or 'q' to quit.")

    # 4. Flash Size Selection
    valid_sizes = get_valid_flash_sizes(selected_platform)
    selected_flash_size = args.flash_size
    if not selected_flash_size:
        default_flash = get_default_flash_size(selected_platform)
        hint = f" (valid for {selected_platform}: {', '.join(valid_sizes)})"
        flash_input = input(f"\nEnter Flash Size [{default_flash}]{hint}: ").strip()
        selected_flash_size = flash_input if flash_input else default_flash

    # Normalize: "4" -> "4MB"
    if selected_flash_size and selected_flash_size.isdigit():
        selected_flash_size += "MB"

    # Validate against known restrictions before launching Docker
    if selected_flash_size and selected_flash_size not in valid_sizes:
        print(f"\nWARNING: '{selected_flash_size}' is not supported for {selected_platform}.")
        print(f"  Valid sizes: {', '.join(valid_sizes)}")
        print(f"  Builds with this size are known to fail (partition table not provided).")
        if input("  Continue anyway? (y/n): ").lower() != 'y':
            print("Aborted.")
            return

    # Version Input
    version_input = input("\nEnter Build Version/Name [Auto-generated]: ").strip()

    # Check if any dependencies were auto-added
    auto_added = set(resolve_dependencies(selected_drivers_list)) - set(selected_drivers_list)

    # Confirm
    print("\nConfiguration:")
    print(f"Output: {output_dir}")
    print(f"Platform: {selected_platform}")
    print(f"Flash Size: {selected_flash_size}")
    print(f"TXW Packager Mode: {args.txw_packager}")
    print(f"Drivers: {selected_drivers_list if selected_drivers_list else 'None'}")
    if auto_added:
        print(f"Auto-enabled dependencies: {sorted(list(auto_added))}")
    if version_input:
        print(f"Version: {version_input}")
    if args.clean:
        print("Option: CLEAN BUILD enabled")

    if input("\nProceed? (y/n): ").lower() != 'y':
        print("Aborted.")
        return

    # Generate custom config
    temp_config_content = create_custom_config(selected_drivers_list)
    temp_config_path = os.path.join(REPO_ROOT, "temp_obk_config.h")
    with open(temp_config_path, 'w', encoding='utf-8') as f:
        f.write(temp_config_content)

    try:
        if not check_docker_running():
            print("\nERROR: Docker is not running!")
            print("Please start Docker Desktop and try again.")
            return

        build_docker_image(no_cache=args.no_cache)

        run_docker_build(
            output_dir,
            selected_platform,
            temp_config_path,
            version_input if version_input else None,
            clean=args.clean,
            flash_size=selected_flash_size,
            txw_packager=args.txw_packager,
        )

    finally:
        if os.path.exists(temp_config_path):
            os.remove(temp_config_path)
        elapsed = time.time() - start_time
        print(f"\nScript execution time: {elapsed:.2f} seconds")

if __name__ == "__main__":
    main()
