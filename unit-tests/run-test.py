#!python3

# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

import sys, os, subprocess, re, platform


def usage():
    ourname = os.path.basename(sys.argv[0])
    print( 'Syntax: ' + ourname + ' <test-to-run>' )
    sys.exit(2)

# Parse command-line:
test_name = None
try:
    test_name = sys.argv[1]
except IndexError as err:
    print( err )
    usage()

def find_file( root, mask ):
    pattern = re.compile( mask )
    for (path,subdirs,leaves) in os.walk( root ):
        for leaf in leaves:
            if pattern.search( leaf ):
                return os.path.join(path,leaf)

current_dir = os.path.dirname(os.path.abspath(__file__))
# this script is located in librealsense/unit-tests, so one directory up is the main repository
librealsense = os.path.dirname(current_dir)

test_path = find_file(librealsense, test_name)
if not test_path:
    print("Test file", test_name, "not found")
    sys.exit(1)

if platform.system() == 'Linux' and "microsoft" not in platform.uname()[3].lower():
    linux = True
else:
    linux = False

# Find the pyrealsense2 .pyd (.so file in linux) in the librealsense repository and add its directory to PYTHONPATH
pyrs = ""
if linux:
    pyrs = find_file(librealsense, '(^|/)pyrealsense2.*\.so$')

else:
    pyrs = find_file(librealsense, '(^|/)pyrealsense2.*\.pyd$')

# If there is no pyd/so file we can't run the python test
if not pyrs:
    print("pyrealsense2 not found. Please make sure to build with python wrappers")
    sys.exit(1)

# We need to add the directory not the file itself
pyrs_dir = os.path.dirname(pyrs)
# Add the necessary path to the PYTHONPATH environment variable so python will look for modules there
os.environ["PYTHONPATH"] = pyrs_dir
# We also need to add the path to the python packages that the tests use
os.environ["PYTHONPATH"] += os.pathsep + (current_dir + os.sep + "py")
# We can't simply change sys.path because the child process of the test won't see it

if linux:
    cmd = ["python3", test_path]
else:
    cmd = ["py", "-3", test_path]
p = subprocess.run(cmd, stderr=subprocess.PIPE, universal_newlines=True)
exit(p.returncode)
