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
    if platform.processor() == 'aarch64':
        # for jetson (arm)
        build = os.path.join(root, 'arm64', 'static')
    else:
        build = os.path.join( root, 'x86_64', 'static' )
else:
    build = os.path.join( root, 'win10', 'win64', 'static' )
if not os.path.isdir( build ):
    #
    build = os.path.join( root, 'build' )
    if not os.path.isdir( build ):
        # Under GHA, we use a build directory under this env variable:
        build = os.environ.get( 'WIN_BUILD_DIR' )
        if not build or not os.path.isdir( build ):
            log.w( 'repo.build directory wasn\'t found' )
            log.d( 'repo.root=', root )
            build = None


def find_pyrs():
    """
    :return: the location (absolute path) of the pyrealsense2 .so (linux) or .pyd (windows)
    """
    global build
    if not build:
        return None
    from rspy import file
    if platform.system() == 'Linux':
        for so in file.find( build, r'(^|/)pyrealsense2.*\.so$' ):
            return os.path.join( build, so )
    else:
        for pyd in file.find( build, r'(^|/)pyrealsense2.*\.pyd$' ):
            return os.path.join( build, pyd )


def find_pyrs_dir():
    """
    :return: the directory (absolute) in which pyrealsense2 lives, or None if unknown/not found
    """
    pyrs = find_pyrs()
    if pyrs:
        pyrs_dir = os.path.dirname( pyrs )
        return pyrs_dir


def find_built_exe( source, name ):
    """
    Find an executable that was built in the repo

    :param source: The location of the exe's project, e.g. tools/convert
    :param name: The name of the exe, without any platform-specific extensions like .exe
    :return: The full path, or None, of the exe
    """
    if platform.system() != 'Linux':
        name += '.exe'
    import sys
    for p in sys.path:
        exe = os.path.join( p, name )
        if os.path.isfile( exe ):
            return exe
    if platform.system() == 'Linux':
        # The Linux build leaves all the executables in their source dir
        global build
        exe = os.path.join( build, source, name )
        if os.path.isfile( exe ):
            return exe
    return None

