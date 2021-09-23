# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import os, platform
from rspy import log

# this script is located in librealsense/unit-tests/py/rspy, so main repository is:
root = os.path.dirname( os.path.dirname( os.path.dirname( os.path.dirname( os.path.abspath( __file__ )))))

# Usually we expect the build directory to be directly under the root, named 'build'
# ... but first check the expected LibCI build directories:
#
if platform.system() == 'Linux':
    build = os.path.join( root, 'x86_64', 'static' )
else:
    build = os.path.join( root, 'win10', 'win64', 'static' )
if not os.path.isdir( build ):
    #
    build = os.path.join( root, 'build' )
    if not os.path.isdir( build ):
        log.w( 'repo.build directory wasn\'t found' )
        log.d( 'repo.root=', root )
        build = None


def find_pyrs():
    """
    :return: the location (absolute path) of the pyrealsense2 .so (linux) or .pyd (windows)
    """
    global build
    from rspy import file
    if platform.system() == 'Linux':
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


def find_built_exe( source, name ):
    """
    Find an executable that was built in the repo

    :param source: The location of the exe's project, e.g. tools/convert
    :param name: The name of the exe, without any platform-specific extensions like .exe
    :return: The full path, or None, of the exe
    """
    exe = None
    if platform.system() == 'Linux':
        global build
        exe = os.path.join( build, source, name )
        if not os.path.isfile( exe ):
            return None
    else:
        # In Windows, the name will be without extension and we need to find it somewhere
        # in the path
        import sys
        for p in sys.path:
            exe = os.path.join( p, name + '.exe' )
            if os.path.isfile( exe ):
                break
        else:
            return None
    return exe

