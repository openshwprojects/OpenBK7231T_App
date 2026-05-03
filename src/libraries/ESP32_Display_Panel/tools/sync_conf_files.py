# SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import shutil

exclude_files = [
    './examples/arduino/board/board_dynamic_config/esp_panel_drivers_conf.h',
    './examples/platformio/lvgl_v8_port/src/lv_conf.h',
]
exclude_dirs = [
    r'.*build.*',
    r'.*pio.*',
]


def is_same_path(path1, path2):
    try:
        return os.path.samefile(path1, path2)
    except FileNotFoundError:
        return False
    except OSError:
        return False


def is_in_directory(file_path, directory):
    import re
    file_path = os.path.realpath(file_path)

    # Check if the file path matches any of the exclude directory patterns
    for pattern in exclude_dirs:
        if re.search(pattern, file_path):
            return True
    return False


def is_same_file(file1, file2):
    # Check if both files exist
    if not os.path.exists(file1) or not os.path.exists(file2):
        return False

    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        file1_content = f1.read()
        file2_content = f2.read()

    return file1_content == file2_content


def is_exclude_file(file_path):
    for exclude_file in exclude_files:
        if is_same_path(file_path, exclude_file):
            return True
    return False


def replace_files(template_directory, search_directory, file_path):
    if os.path.dirname(file_path) == '':
        file_path = os.path.join(search_directory, file_path)

    filename = os.path.basename(file_path)
    src_file = os.path.join(template_directory, filename)

    if is_exclude_file(file_path) or not os.path.exists(src_file):
        print(f"Skip '{file_path}'")
        return

    if is_same_path(file_path, src_file):
        for root, dirs, files in os.walk(search_directory):
            need_exclude = False
            for exclude_dir in exclude_dirs:
                if is_in_directory(root, exclude_dir):
                    need_exclude = True
                    break

            if root == template_directory or need_exclude:
                continue

            for file in files:
                if file == filename:
                    dst_file = os.path.join(root, file)

                    if is_exclude_file(dst_file):
                        print(f"Skip '{dst_file}'")
                        continue

                    if not is_same_file(src_file, dst_file):
                        print(f"Replacing '{dst_file}' with '{src_file}'...")
                        shutil.copy(src_file, dst_file)
    else:
        for exclude_dir in exclude_dirs:
            need_exclude = False
            if is_in_directory(file_path, exclude_dir):
                need_exclude = True

        if not need_exclude and not is_same_file(src_file, file_path):
            print(f"Restoring '{file_path}' with '{src_file}'...")
            shutil.copy(src_file, file_path)


if __name__ == '__main__':
    if len(sys.argv) >= 3:
        template_directory = sys.argv[1]
        search_directory = sys.argv[2]

        for i in range(3, len(sys.argv)):
            file_path = sys.argv[i]
            replace_files(template_directory, search_directory, file_path)

        print('Replacement completed.')
