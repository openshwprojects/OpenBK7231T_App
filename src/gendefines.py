import re
import sys
import argparse

def generate_platform_defines(input_file):
    # Read the input file
    if input_file:
        with open(input_file, 'r') as file:
            lines = file.readlines()
    else:
        lines = sys.stdin.readlines()

    # Extract "#define" statements and remove duplicates
    defines = set()
    for line in lines:
        match = re.match(r'^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)', line)
        if match:
            defines.add(match.group(0).strip())

    # empty output content
    output_lines = []

    # Add initial lines
    output_lines.append('#pragma once\n')
    output_lines.append('#define tstr(x) #x\n')
    output_lines.append('#define str(x) tstr(x)\n')
    output_lines.append('#include "obk_config.h"\n\n')
    output_lines.append('char platformdefines[]=""\n')
    output_lines.append('#ifdef OBK_VARIANT\n')
    output_lines.append('"OBK_VARIANT=" str(OBK_VARIANT) "<br>\\n"\n')
    output_lines.append('#endif\n')

    # loop define statements
    for define in defines:
        match = re.match(r'^#define ENABLE_([^ \t]*)', define)
        if match:
            output_lines.append(f'#ifdef ENABLE_{match.group(1)}\n')
            output_lines.append(f'"{match.group(1)}<br>\\n"\n')
            output_lines.append('#endif\n')

    # append ";" to end "platformdefines[]" string definition
    output_lines.append(';\n')
    # write result
    sys.stdout.write(''.join(output_lines))

# check for parameter (input file) or read from stdio
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate platform defines from a C header file.')
    parser.add_argument('input_file', nargs='?', help='Input header file (optional, use stdin if missing).')
    args = parser.parse_args()

    generate_platform_defines(args.input_file)

