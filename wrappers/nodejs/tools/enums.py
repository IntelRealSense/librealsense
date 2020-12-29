# -*- coding: utf-8 -*-
import re
import os
import sys
import argparse


ENUM_H_REGEXP = r'^[ ]*(RS2_\w+)'
ENUM_CPP_REGEXP = r'^[ ]*_FORCE_SET_ENUM\((\w+)\)'
ENUM_JS_REGEXP = r'RS2\.(RS2_\w+)'


def files_gen(path, ends):
    for root, dirs, files in os.walk(path):
        for file in files:
            if file.endswith(ends):
                yield os.path.join(root, file)


def get_first_by_regexp(line, regexp):
    res = re.findall(regexp, line)
    if res:
        return res[0]
    return ''


def get_enums_from_folder(folder_path, ends, regexp):
    file_paths = files_gen(folder_path, ends)
    enums = []
    for file_path in file_paths:
        enums += get_enums_from_file(file_path, regexp)
    return enums


def get_enums_from_file(file_path, regexp):
    enums = []
    with open(file_path) as f:
        for line in f:
            enum = get_first_by_regexp(line, regexp)
            if enum:
                enums.append(enum)

    return enums


def run(include_folder_path, addon_folder_path):

    include_enums = get_enums_from_folder(include_folder_path, '.h', ENUM_H_REGEXP)
    cpp_enums = get_enums_from_folder(addon_folder_path, '.cpp', ENUM_CPP_REGEXP)
    # js_enums = get_enums_from_file(js_file_path, ENUM_JS_REGEXP)

    include_enums.sort()
    cpp_enums.sort()
    # js_enums.sort()

    # print(len(include_enums), len(cpp_enums), len(js_enums))
    return list(set(include_enums) - set(cpp_enums))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Ping script')
    parser.add_argument('-i', action='store', dest='include_folder_path', required=True)
    parser.add_argument('-a', action='store', dest='addon_folder_path', required=True)
    parser.add_argument('-v', '--verbose', action='store_true', help='print not user enums')

    args = parser.parse_args()

    missed = run(args.include_folder_path, args.addon_folder_path)

    if missed:
        missed.sort()
        message = "[ERROR] Node.js wrapper has missing enum values: %s" % (', '.join(missed))
        sys.exit(message if args.verbose else 1)

    sys.exit()
