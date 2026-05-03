# SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import re

exclude_dirs = [
    './build',
    './examples/platformio/lvgl_v8_port/.pio'
]
internal_version_file = 'src/esp_panel_versions.h'
include_files = [
    'examples/arduino/board/board_dynamic_config/esp_panel_drivers_conf.h'
]
internal_version_macros = [
    {
        'file': 'library.properties',
        'macro': {
            'major': 'ESP_PANEL_VERSION_MAJOR',
            'minor': 'ESP_PANEL_VERSION_MINOR',
            'patch': 'ESP_PANEL_VERSION_PATCH'
        },
    },
    {
        'file': 'esp_panel_drivers_conf.h',
        'macro': {
            'major': 'ESP_PANEL_DRIVERS_CONF_VERSION_MAJOR',
            'minor': 'ESP_PANEL_DRIVERS_CONF_VERSION_MINOR',
            'patch': 'ESP_PANEL_DRIVERS_CONF_VERSION_PATCH'
        },
    },
    {
        'file': 'esp_panel_board_custom_conf.h',
        'macro': {
            'major': 'ESP_PANEL_BOARD_CUSTOM_VERSION_MAJOR',
            'minor': 'ESP_PANEL_BOARD_CUSTOM_VERSION_MINOR',
            'patch': 'ESP_PANEL_BOARD_CUSTOM_VERSION_PATCH'
        },
    },
    {
        'file': 'esp_panel_board_supported_conf.h',
        'macro': {
            'major': 'ESP_PANEL_BOARD_SUPPORTED_VERSION_MAJOR',
            'minor': 'ESP_PANEL_BOARD_SUPPORTED_VERSION_MINOR',
            'patch': 'ESP_PANEL_BOARD_SUPPORTED_VERSION_PATCH'
        },
    },
]
file_version_macros = [
    {
        'file': 'esp_panel_drivers_conf.h',
        'macro': {
            'major': 'ESP_PANEL_DRIVERS_CONF_FILE_VERSION_MAJOR',
            'minor': 'ESP_PANEL_DRIVERS_CONF_FILE_VERSION_MINOR',
            'patch': 'ESP_PANEL_DRIVERS_CONF_FILE_VERSION_PATCH'
        },
    },
    {
        'file': 'esp_panel_board_custom_conf.h',
        'macro': {
            'major': 'ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR',
            'minor': 'ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MINOR',
            'patch': 'ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_PATCH'
        },
    },
    {
        'file': 'esp_panel_board_supported_conf.h',
        'macro': {
            'major': 'ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MAJOR',
            'minor': 'ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_MINOR',
            'patch': 'ESP_PANEL_BOARD_SUPPORTED_FILE_VERSION_PATCH'
        },
    },
]
arduino_version_file = 'library.properties'


def is_in_directory(file_path, directory):
    directory = os.path.realpath(directory)
    file_path = os.path.realpath(file_path)

    return file_path.startswith(directory)


def extract_file_version(file_path, version_dict):
    file_contents = []
    content_str = ''
    with open(file_path, 'r') as file:
        file_contents.append(file.readlines())
        for content in file_contents:
            content_str = ''.join(content)

    version_macro = version_dict['macro']
    major_version = re.search(r'#define ' + version_macro['major'] + r' (\d+)', content_str)
    minor_version = re.search(r'#define ' + version_macro['minor'] + r' (\d+)', content_str)
    patch_version = re.search(r'#define ' + version_macro['patch'] + r' (\d+)', content_str)

    if major_version and minor_version and patch_version:
        return {'file': version_dict['file'], 'version': major_version.group(1) + '.' + minor_version.group(1) + '.' + patch_version.group(1)}

    return None


def extract_arduino_version(file_path):
    file_contents = []
    content_str = ''
    with open(file_path, 'r') as file:
        file_contents.append(file.readlines())
        for content in file_contents:
            content_str = ''.join(content)

    version = re.search(r'version=(\d+\.\d+\.\d+)', content_str)

    if version:
        return {'file': os.path.basename(file_path), 'version': version.group(1)}

    return None


if __name__ == '__main__':
    if len(sys.argv) >= 3:
        search_directory = sys.argv[1]

        # Extract internal versions
        internal_version_path = os.path.join(search_directory, internal_version_file)
        internal_versions = []
        print(f"Internal version extracted from '{internal_version_path}")
        for internal_version_macro in internal_version_macros:
            version = extract_file_version(internal_version_path, internal_version_macro)
            if version:
                print(f"Internal version: '{internal_version_macro['file']}': {version}")
                internal_versions.append(version)
            else:
                print(f"'{internal_version_macro['file']}' version not found")
                sys.exit(1)

        # Check file versions
        for i in range(2, len(sys.argv)):
            file_path = sys.argv[i]

            if file_path in internal_version_file:
                print(f'Checking all files')

                for root, dirs, files in os.walk(search_directory):
                    if root == search_directory:
                        continue

                    need_exclude = False
                    for exclude_dir in exclude_dirs:
                        if is_in_directory(root, exclude_dir):
                            need_exclude = True
                            break

                    if need_exclude:
                        continue

                    for file in files:
                        file_need_check = False
                        for file_version_macro in file_version_macros:
                            if file == file_version_macro['file'] and root not in exclude_dirs:
                                file_need_check = True
                                break

                        if file_need_check:
                            versions = extract_file_version(os.path.join(root, file), file_version_macro)
                            if versions:
                                print(f"File version extracted from '{os.path.join(root, file)}': {versions}")
                                for internal_version in internal_versions:
                                    if (internal_version['file'] == versions['file']) and (internal_version['version'] != versions['version']):
                                        print(f"Version mismatch: '{internal_version['file']}'")
                                        sys.exit(1)

            # src_file = os.path.join(search_directory, os.path.basename(file_path))
            src_file = file_path
            for file_version in file_version_macros:
                versions = extract_file_version(src_file, file_version)
                if versions:
                    print(f"File version extracted from '{src_file}': {versions}")
                    for internal_version in internal_versions:
                        if (internal_version['file'] == versions['file']) and (internal_version['version'] != versions['version']):
                            print(f"Version mismatch: '{internal_version['file']}'")
                            sys.exit(1)
                    print(f'Version matched')

        # Extract arduino version
        arduino_version_path = os.path.join(search_directory, arduino_version_file)
        arduino_version = extract_arduino_version(arduino_version_path)
        print(f"Arduino version extracted from '{arduino_version_path}")
        if arduino_version:
            print(f'Arduino version: {arduino_version}')
        else:
            print(f'Arduino version not found')

        # Check arduino version
        for internal_version in internal_versions:
            if (internal_version['file'] == arduino_version_file) and (internal_version['version'] != arduino_version['version']):
                print(f"Arduino version mismatch: '{internal_version['file']}'")
                sys.exit(1)
        print(f'Arduino Version matched')
