# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import os

# this script is located in librealsense/unit-tests/py/rspy, so main repository is:
root = os.path.dirname( os.path.dirname( os.path.dirname( os.path.dirname( os.path.abspath( __file__ )))))

# Usually we expect the build directory to be directly under the root, named 'build'
build = os.path.join( root, 'win10', 'win64', 'static' )
if not os.path.isdir( build ):
    build = os.path.join( root, 'build' )
    if not os.path.isdir( build ):
        build = None


def find_pyrs():
    """
    :return: the location (absolute path) of the pyrealsense2 .so (linux) or .pyd (windows)
    """
    global build
    import platform
    system = platform.system()
    linux = ( system == 'Linux'  and  "microsoft" not in platform.uname()[3].lower() )
    from rspy import file
    if linux:
        for so in file.find( build, '(^|/)pyrealsense2.*\.so$' ):
            return os.path.join( build, so )
    else:
        for pyd in file.find( build, '(^|/)pyrealsense2.*\.pyd$' ):
            return os.path.join( build, pyd )


def find_pyrs_dir():
    """
    :return: the directory (absolute) in which pyrealsense2 lives, or None if unknown/not found
    """
    pyrs = find_pyrs()
    if pyrs:
        pyrs_dir = os.path.dirname( pyrs )
        return pyrs_dir


def pretty_fw_version( fw_version_as_string ):
    """
    :return: a version with leading zeros removed, so as to be a little easier to read
    """
    return '.'.join( [str(int(c)) for c in fw_version_as_string.split( '.' )] )
