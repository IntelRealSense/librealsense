# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import os

# this script is located in librealsense/unit-tests/py/rspy, so main repository is:
root = os.path.dirname( os.path.dirname( os.path.dirname( os.path.dirname( os.path.abspath( __file__ )))))

def get_bundled_fw_version( product_line ):
    """
    :param product_line: product line of a given device (ex. L500, D400)
    :return: the bundled FW version for this device
    """
    # common/fw/firmware-version.h contains the bundled FW versions for all product lines
    fw_versions_file = os.path.join(root, 'common', 'fw', 'firmware-version.h')
    if not os.path.isfile(fw_versions_file):
        log.e("Expected to find a file containing FW versions at", fw_versions_file, ", but the file was not found")
        sys.exit(1)

    fw_versions = open(fw_versions_file, 'r')
    for line in fw_versions:
        words = line.split()
        if len( words ) == 0 or words[0] != "#define":
            continue
        if product_line[0:2] in words[1]: # the file contains FW versions for L5XX or D4XX devices, so we only look at the first 2 characters
            fw_versions.close()
            return words[2][1:-1] # remove "" from beginning and end of FW version
    fw_versions.close()
    return None

def pretty_fw_version( fw_version_as_string ):
    """ return a version with zeros removed """
    return '.'.join( [str(int(c)) for c in fw_version_as_string.split( '.' )] )
