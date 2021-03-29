# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import os

# this script is located in librealsense/unit-tests/py/rspy, so main repository is:
root = os.path.dirname( os.path.dirname( os.path.dirname( os.path.dirname( os.path.abspath( __file__ )))))

def pretty_fw_version( fw_version_as_string ):
    """ return a version with zeros removed """
    return '.'.join( [str(int(c)) for c in fw_version_as_string.split( '.' )] )
