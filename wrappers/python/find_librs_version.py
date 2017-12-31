import io
import re
import sys
import os

librs_version = ''
if len(sys.argv) < 2:
    print("Error! Usage: find_librs_version.py <absolute_path_to_librealsense>")
    exit(1)

librealsense_dir = sys.argv[1]
rs_h_path = os.path.join(librealsense_dir, 'include/librealsense2/rs.h')
print("Extracting version from: ", rs_h_path)
with io.open(rs_h_path, 'r') as f:
    file_content = f.read()
    major = re.search(r"#define\s*RS2_API_MAJOR_VERSION\s*(\d+)",file_content)
    if not major:
        raise Exception('No major number')
    librs_version += major.group(1)
    librs_version += '.'
    minor = re.search(r"#define\s*RS2_API_MINOR_VERSION\s*(\d+)",file_content)
    if not minor:
        raise Exception('No minor number')
    librs_version += minor.group(1)
    librs_version += '.'
    patch = re.search(r"#define\s*RS2_API_PATCH_VERSION\s*(\d+)",file_content)
    if not patch:
        raise Exception('No patch number')
    librs_version += patch.group(1)

    print("Librealsense Version: ", librs_version)
    outfile = os.path.join(librealsense_dir, 'wrappers/python/pyrealsense2/_version.py')
    print("Writing version to: ", outfile)
    with open(outfile, 'w') as f:
        f.write('__version__ = "{}"'.format(librs_version))
