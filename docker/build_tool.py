import os
import subprocess
import re
import sys
import shutil
import argparse
import time

# Configuration
# Auto-detect repository root based on script location
_script_dir = os.path.dirname(os.path.abspath(__file__))
if os.path.basename(_script_dir) == "docker":
    # If we are in docker/, repo root is one level up
    REPO_ROOT = os.path.dirname(_script_dir)
else:
    # Assume script is in root
    REPO_ROOT = _script_dir

# Initialize derived paths
DOCKER_DIR = os.path.join(REPO_ROOT, "docker")
PLATFORMS_DIR = os.path.join(REPO_ROOT, "platforms")
CONFIG_FILE = os.path.join(REPO_ROOT, "src", "obk_config.h")

def update_paths(src_dir):
    """
    Updates global path variables based on the provided source directory.
    If src_dir is not provided, paths remain as initialized at module load.
    """
    global REPO_ROOT, DOCKER_DIR, PLATFORMS_DIR, CONFIG_FILE
    if src_dir:
        REPO_ROOT = os.path.abspath(src_dir)
        DOCKER_DIR = os.path.join(REPO_ROOT, "docker")
        PLATFORMS_DIR = os.path.join(REPO_ROOT, "platforms")
        CONFIG_FILE = os.path.join(REPO_ROOT, "src", "obk_config.h")


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

def get_platforms():
    return sorted(PLATFORMS.keys())

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

    drivers = set()
    
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
    
    # Matches driver definitions: #define ENABLE_DRIVER_... 1  OR  #define ENABLE_NTP 1
    driver_regex = re.compile(r'^\s*#\s*define\s+(ENABLE_\w+)\s+1')
    
    # Special non-driver names we want to capture
    special_names = ["ENABLE_NTP", "ENABLE_I2C", "ENABLE_TASMOTADEVICEGROUPS"]

    for line in lines:
        line = line.strip()
        
        # 1. Driver Definition
        match = driver_regex.search(line)
        if match:
            # If current block is active, add driver
            if all(active_stack):
                d_name = match.group(1)
                if d_name.startswith("ENABLE_DRIVER_") or d_name in special_names:
                    drivers.add(d_name)
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
            
    return sorted(list(drivers))

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
    # Resolve all dependencies
    all_drivers = resolve_dependencies(selected_drivers)
    
    # Read original config
    with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    new_lines = []
    # Match any #define ENABLE_... or #define ENABLE_DRIVER_... followed by a number
    driver_regex = re.compile(r'#\s*define\s+(ENABLE_\w+)\s+(\d+)')

    for line in lines:
        match = driver_regex.search(line)
        if match:
            driver_name = match.group(1)
            
            # IS this a driver we are managing?
            # We manage anything starting with ENABLE_DRIVER_ or our special list
            is_managed = driver_name.startswith("ENABLE_DRIVER_") or \
                         driver_name in ["ENABLE_NTP", "ENABLE_I2C", "ENABLE_TASMOTADEVICEGROUPS"]
            
            if is_managed:
                if driver_name in all_drivers:
                    # Force enable
                    new_lines.append(f"#define {driver_name}\t1\n")
                else:
                    # Force disable
                    new_lines.append(f"// #define {driver_name}\t0 // Disabled by build_tool\n")
            else:
                 new_lines.append(line)
        else:
            new_lines.append(line)
            
    return "".join(new_lines)

def check_docker_running():
    """
    Checks if Docker daemon is running by attempting 'docker info'.
    Returns True if running, False otherwise.
    """
    try:
        result = subprocess.run(
            ["docker", "info"],
            capture_output=True,
            timeout=5
        )
        return result.returncode == 0
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return False

def build_docker_image(no_cache=False):
    print("Building Docker image...")
    cmd = ["docker", "build", "-t", "openbk_builder", "."]
    if no_cache:
        cmd.insert(2, "--no-cache")
    subprocess.run(cmd, cwd=DOCKER_DIR, check=True)

def run_docker_build(output_dir, platform, custom_config_path, version=None, clean=False, flash_size=None):
    # Resolve Make target (e.g., BK7231N -> OpenBK7231N)
    if platform in PLATFORMS:
        target_platform = PLATFORMS[platform]["target"]
    else:
        target_platform = platform # Fallback
    print(f"Running Docker build for {platform} (Target: {target_platform})...")
    
    # Ensure output dir exists
    abs_output_dir = os.path.abspath(output_dir)
    os.makedirs(abs_output_dir, exist_ok=True)
    
    cmd = [
        "docker", "run", "--rm",
        "-v", f"{REPO_ROOT}:/app/source:ro",
        "-v", f"{abs_output_dir}:/app/output",
        "-v", f"{custom_config_path}:/app/custom_config.h:ro",
        "-v", "openbk_build_data:/app/build", # Named volume for caching
    ]

    
    if version:
        cmd.extend(["-e", f"APP_VERSION={version}"])
        
    if clean:
        cmd.extend(["-e", "CLEAN_BUILD=1"])
        
    if flash_size:
        cmd.extend(["-e", f"FLASH_SIZE={flash_size}"])

    # Pass Host Timezone
    try:
        is_dst = time.daylight and time.localtime().tm_isdst > 0
        local_tz = time.tzname[1] if is_dst else time.tzname[0]
        cmd.extend(["-e", f"TZ={local_tz}"])
    except Exception:
        pass 
    
    cmd.extend([
        "openbk_builder",
        target_platform
    ])
    
    start_time = time.time() # Use time.time for wall clock, os.times is process time
    subprocess.run(cmd, check=True)
    end_time = time.time()
    
    print(f"Build task finished in {end_time - start_time:.2f} seconds.")

def get_default_flash_size(platform):
    """
    Returns a default flash size based on platform.
    """
    if "ESP32" in platform:
        return "4MB" # Safe default for C3/S3 etc.
    if "ESP8266" in platform:
        return "2MB" # Most modules use 2MB or 4MB (1MB is rare now but supported)
    if "BK7231" in platform:
        return "2MB" # Standard for T/N
    return "2MB" # Generic default

def main():
    parser = argparse.ArgumentParser(description="OpenBK7231T_App Build Tool")
    parser.add_argument("--clean", action="store_true", help="Perform a clean build (make clean) before building")
    parser.add_argument("--no-cache", action="store_true", help="Force rebuild of Docker image (ignore cache)")
    parser.add_argument("--flash-size", help="Flash size (e.g. 1MB, 2MB, 4MB)")
    parser.add_argument("--src", help="Path to the repository root (needed if running from outside)")
    args = parser.parse_args()

    # Update paths based on --src
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
    
    # Use new parser
    all_drivers = get_available_drivers(selected_platform)
    
    selected_drivers = []
    selected_drivers_list = [] # Initialize to empty list to avoid UnboundLocalError
    
    if not all_drivers:
        print("No drivers definitions found (or config file missing).")
    else:
        print("\nSelect Drivers (space-separated numbers, e.g., '1 5 10', or Enter for None):")
        # Show in columns or list
        for i, d in enumerate(all_drivers):
            print(f"{i+1:3}. {d}")
            
        while True:
            drivers_input = input("\nSelection: ").strip()
            if drivers_input.lower() in ['q', 'quit', 'exit']:
                print("Exiting.")
                return

            if not drivers_input:
                break # None selected

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
    # If not provided via CLI, ask interactively
    selected_flash_size = args.flash_size
    if not selected_flash_size:
        default_flash = get_default_flash_size(selected_platform)
        flash_input = input(f"\nEnter Flash Size (e.g. 1MB, 2MB, 4MB) [{default_flash}]: ").strip()
        selected_flash_size = flash_input if flash_input else default_flash

    # Normalize Flash Size: If user enters just "4", convert to "4MB"
    if selected_flash_size and selected_flash_size.isdigit():
        selected_flash_size += "MB"

    # Version Input
    version_input = input("\nEnter Build Version/Name [Auto-generated]: ").strip()

    # Check if any dependencies were auto-added
    auto_added = set(resolve_dependencies(selected_drivers_list)) - set(selected_drivers_list)
    
    # Confirm
    print("\nConfiguration:")
    print(f"Output: {output_dir}")
    print(f"Platform: {selected_platform}")
    print(f"Flash Size: {selected_flash_size}")
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
        # Check if Docker is running first
        if not check_docker_running():
            print("\n❌ ERROR: Docker is not running!")
            print("Please start Docker Desktop and try again.")
            return
        
        # Build Docker Image (ensure it exists/updated)
        build_docker_image(no_cache=args.no_cache)
        
        # Run Build
        run_docker_build(output_dir, selected_platform, temp_config_path, version_input if version_input else None, clean=args.clean, flash_size=selected_flash_size)
        
    finally:
        # Cleanup
        if os.path.exists(temp_config_path):
            os.remove(temp_config_path)
            
        elapsed = time.time() - start_time
        print(f"\nScript execution time: {elapsed:.2f} seconds")

if __name__ == "__main__":
    main()
