# SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import re

internal_version_file = 'src/esp_utils_versions.h'
internal_version_macross = [
    {
        'file': 'library.properties',
        'macro': {
            'major': 'ESP_UTILS_VERSION_MAJOR',
            'minor': 'ESP_UTILS_VERSION_MINOR',
            'patch': 'ESP_UTILS_VERSION_PATCH'
        },
    },
    {
        'file': 'esp_utils_conf.h',
        'macro': {
            'major': 'ESP_UTILS_CONF_VERSION_MAJOR',
            'minor': 'ESP_UTILS_CONF_VERSION_MINOR',
            'patch': 'ESP_UTILS_CONF_VERSION_PATCH'
        },
    },
]
file_version_macros = [
    {
        'file': 'esp_utils_conf.h',
        'macro': {
            'major': 'ESP_UTILS_CONF_FILE_VERSION_MAJOR',
            'minor': 'ESP_UTILS_CONF_FILE_VERSION_MINOR',
            'patch': 'ESP_UTILS_CONF_FILE_VERSION_PATCH'
        },
    },
]
arduino_version_file = 'library.properties'


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
        for internal_version_macros in internal_version_macross:
            version = extract_file_version(internal_version_path, internal_version_macros)
            if version:
                print(f"Internal version: '{internal_version_macros['file']}': {version}")
                internal_versions.append(version)
            else:
                print(f"'{internal_version_macros['file']}' version not found")
                sys.exit(1)

        # Check file versions
        for i in range(2, len(sys.argv)):
            file_path = sys.argv[i]

            if file_path in internal_version_file:
                print(f"Skipping '{file_path}'")
                continue

            src_file = os.path.join(search_directory, os.path.basename(file_path))
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
