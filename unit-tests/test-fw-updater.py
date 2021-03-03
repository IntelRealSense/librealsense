# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:priority 0
#test:device L500*
#test:device D400*

import pyrealsense2 as rs, sys, os, subprocess
from rspy import devices, log, test

def get_bundled_fw_version( product_line ):
    """
    :param product_line: product line of a given device (ex. L500, D400)
    :return: the bundled FW version for this device
    """
    global fw_versions_file
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

def has_newer_fw( current_fw, bundled_fw ):
    """
    :param current_fw: current FW version of a device
    :param bundled_fw: bundled FW version of the same device
    :return: True if the bundled version is newer than the current one
    """
    current_fw_digits = current_fw.split( '.' )
    bundled_fw_digits = bundled_fw.split( '.' )
    if len( current_fw_digits ) != len( bundled_fw_digits ):
        log.e( "Either the devices FW (", current_fw, ") or the bundled FW(", bundled_fw, ") was of an invalid format")
        sys.exit(1)
    for curr, bundled in zip( current_fw_digits, bundled_fw_digits ):
        if int(bundled) > int(curr):
            return True
        if int(bundled) < int(curr):
            return False
    return False


if not devices.acroname:
    log.i("No acroname found, not updating device FW")
    sys.exit(0)

# this script is in unit-tests directory
librealsense = os.path.dirname( os.path.dirname( os.path.abspath( __file__ )) )
# common/fw/firmware-version.h contains the bundled FW versions for all product lines
fw_versions_file = os.path.join( librealsense, 'common', 'fw', 'firmware-version.h' )
if not os.path.isfile( fw_versions_file ):
    log.e( "Expected to find a file containing FW versions at", fw_versions_file, ", but the file was not found" )
    sys.exit(1)
# build/RelWithDebInfo/rs-fw-update.exe performs a FW update for a connected device
fw_updater_exe = os.path.join( librealsense, 'build', 'RelWithDebInfo', 'rs-fw-update.exe' )
if not os.path.isfile( fw_updater_exe ):
    log.e("Expected to find FW update tool at", fw_updater_exe, ", but the file was not found")
    sys.exit(1)
# build/common/fw contains the image files for the FW. Needed as an argument for fw_updater_exe
fw_image_dir = os.path.join( librealsense, 'build', 'common', 'fw' )


devices.query()
sn_list = devices.all()
# acroname should insure there is always 1 available device
if len( sn_list ) != 1:
    log.e( "Expected 1 device, got", len( sn_list ) )
    sys.exit(1)
device = devices.get( list( sn_list )[0] )
current_fw_version = device.get_info( rs.camera_info.firmware_version )
product_line =  device.get_info( rs.camera_info.product_line )
bundled_fw_version = get_bundled_fw_version( product_line )
if not bundled_fw_version:
    log.e( "No FW version found in", fw_versions_file, "for product line", product_line )
    sys.exit(1)
if not has_newer_fw( current_fw_version, bundled_fw_version ):
    log.i( "No update needed: FW version is already", current_fw_version)
    sys.exit(0)
image_file = fw_image_dir + os.sep + product_line[0:2] + "XX_FW_Image-" + bundled_fw_version + ".bin"
if not os.path.isfile( image_file ):
    log.e("Expected to find file containing image for FW at", fw_updater_exe, ", but the file was not found")
    sys.exit(1)
try:
    subprocess.run([fw_updater_exe, '-f', image_file])
except Exception as e:
    test.unexpected_exception()

