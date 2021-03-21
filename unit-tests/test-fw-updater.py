# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

# we want this test to run first so that all tests run with updated FW versions, so we give it priority 0
#test:priority 0
#test:device L500*
#test:device D400*

import pyrealsense2 as rs, sys, os, subprocess, re
from rspy import devices, log, test, file, repo

if not devices.acroname:
    log.i( "No Acroname library found; skipping device FW update" )
    sys.exit(0)
# Following will throw if no acroname module is found
from rspy import acroname
try:
    devices.acroname.discover()
except acroname.NoneFoundError as e:
    log.f( e )
# Remove acroname -- we're likely running inside run-unit-tests in which case the
# acroname hub is likely already connected-to from there and we'll get an error
# thrown ('failed to connect to acroname (result=11)'). We do not need it -- just
# needed to verify it is available above...
devices.acroname = None

# find the update tool exe
fw_updater_exe = None
for tool in file.find( repo.root, '(^|/)rs-fw-update.exe$' ):
    fw_updater_exe = os.path.join( repo.root, tool)
if not fw_updater_exe:
    log.f( "Could not find the update tool file (rs-fw-update.exe)" )

devices.query( monitor_changes = False )
sn_list = devices.all()
# acroname should ensure there is always 1 available device
if len( sn_list ) != 1:
    log.f( "Expected 1 device, got", len( sn_list ) )
device = devices.get( list( sn_list )[0] )
log.d( 'found:', device )
current_fw_version = repo.pretty_fw_version( device.get_info( rs.camera_info.firmware_version ))
log.d( 'FW version:', current_fw_version )
product_line =  device.get_info( rs.camera_info.product_line )
log.d( 'product line:', product_line )
bundled_fw_version = device.get_info( rs.camera_info.recommended_firmware_version )
log.d( 'bundled FW version:', bundled_fw_version )

# finding file containing image for FW update
image_name = product_line[0:2] + "XX_FW_Image-" + bundled_fw_version + ".bin"
image_mask = '(^|/)' + image_name + '$'
image_file = None
for image in file.find( repo.root, image_mask ):
    image_file = os.path.join( repo.root, image)
if not image_file:
    log.f( "Could not find image file for" + product_line + "device with FW version:" + bundled_fw_version )
test.start( "Update FW" )
try:
    cmd = [fw_updater_exe, '-f', image_file]
    log.d( 'running:', cmd )
    subprocess.run( cmd )
except Exception as e:
    test.unexpected_exception()

# make sure update worked
devices.query( monitor_changes = False )
sn_list = devices.all()
device = devices.get( list( sn_list )[0] )
current_fw_version = repo.pretty_fw_version( device.get_info( rs.camera_info.firmware_version ))
test.check_equal( current_fw_version, bundled_fw_version )
test.finish()
test.print_results_and_exit()
