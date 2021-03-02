# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#test:device L500*
#test:device D400*

import pyrealsense2 as rs, sys, os, subprocess
from rspy import devices, log

try:
    from rspy import acroname
except ModuleNotFoundError:
    log.i( "no acroname found, not updating device FW")
    sys.exit(0)

librealsense = os.path.dirname( os.path.dirname( os.path.abspath( __file__ )) )
# common/fw/firmware-version.h contains the latest FW versions for all product lines
fw_versions_file = librealsense + os.sep + 'common' + os.sep + 'fw' + os.sep + 'firmware-version.h'
if not os.path.isfile( fw_versions_file ):
    log.e( "expected to find a file at", fw_versions_file, "containing FW versions but the file was not found" )
    sys.exit(1)
# build/RelWithDebInfo/rs-fw-update.exe performs a FW update for a connected device
fw_updater_exe = librealsense + os.sep + 'build' + os.sep + 'RelWithDebInfo' + os.sep + 'rs-fw-update.exe'
if not os.path.isfile( fw_updater_exe ):
    log.e( "FW update tool not found" )
    sys.exit(1)
# build/common/fw contains the image files for the FW. Needed as an argument for fw_updater_exe
fw_image_dir = librealsense + os.sep + 'build' + os.sep + 'common' + os.sep + 'fw'

def get_latest_fw_version( product_line ):
    """
    :param product_line: product line of a given device (ex. L500, D400)
    :return: the latest FW version for this device
    """
    global fw_versions
    for line in fw_versions:
        words = line.split()
        if len( words ) == 0 or words[0] != "#define":
            continue
        if product_line[0:2] in words[1]: # the file contains FW versions for L5XX or D4XX devices, so we only look at the first 2 characters
            return words[2][1:-1] # remove "" from beginning and end of FW version

def has_newer_fw( current_fw, latest_fw ):
    """
    :param current_fw: current FW version of a device
    :param latest_fw: latest FW version of the same device
    :return: True if the latest version is newer than the current one
    """
    for curr, latest in zip( current_fw.split( '.' ), latest_fw.split( '.' ) ):
        if int(latest) > int(curr):
            return True
        if int(latest) < int(curr):
            return False
    return False

devices.query()
sn_list = devices.all()
# acroname should insure there is always 1 available device
if len( sn_list ) != 1:
    log.e( "think of error message" )
    sys.exit(1)
device = devices.get( list( sn_list )[0] )
current_fw_version = device.get_info( rs.camera_info.firmware_version )
product_line =  device.get_info( rs.camera_info.product_line )
fw_versions = open( fw_versions_file, 'r' )
latest_fw_version = get_latest_fw_version(product_line)
if has_newer_fw( current_fw_version, latest_fw_version ):
    image_file = fw_image_dir + os.sep + product_line[0:2] + "XX_FW_Image-" + latest_fw_version + ".bin"
    if not os.path.isfile(image_file):
        log.e("file containing image for FW not found")
    subprocess.run([fw_updater_exe, '-f', image_file])

fw_versions.close()