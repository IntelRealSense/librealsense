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

def send_hardware_monitor_command( device, command ):
    command_input = []  # array of uint_8t

    # Parsing the command to array of unsigned integers(size should be < 8bits)
    # threw out spaces
    command = command.lower()
    command = command.replace(" ", "")
    current_uint8_t_string = ''
    for i in range(0, len(command)):
        current_uint8_t_string += command[i]
        if len(current_uint8_t_string) >= 2:
            command_input.append(int('0x' + current_uint8_t_string, 0))
            current_uint8_t_string = ''
    if current_uint8_t_string != '':
        command_input.append(int('0x' + current_uint8_t_string, 0))

    # byte_index = -1
    raw_result = rs.debug_protocol( device ).send_and_receive_raw_data( command_input )

    return raw_result[4:]

def get_update_counter( device ):
    product_line = device.get_info( rs.camera_info.product_line )
    cmd = None

    if product_line == "L500":
        cmd = "14 00 AB CD 09 00 00 00 30 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00"
    elif product_line == "D400":
        cmd = "14 00 AB CD 09 00 00 00 30 00 00 00 02 00 00 00 00 00 00 00 00 00 00 00"
    else:
        log.f( "Incompatible product line:", product_line )

    counter = send_hardware_monitor_command( device, cmd )
    return counter[0]

def reset_update_counter( device ):
    product_line = device.get_info( rs.camera_info.product_line )
    cmd = None

    if product_line == "L500":
        cmd = "14 00 AB CD 0A 00 00 00 30 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00"
    elif product_line == "D400":
        cmd = "14 00 AB CD 86 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
    else:
        log.f( "Incompatible product line:", product_line )

    send_hardware_monitor_command( device, cmd )


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
bundled_fw_version = repo.pretty_fw_version( device.get_info( rs.camera_info.recommended_firmware_version ) )
log.d( 'bundled FW version:', bundled_fw_version )

update_counter = get_update_counter( device )
log.d( 'update counter:', update_counter )
if get_update_counter( device ) >= 19:
    log.d( 'resetting update counter' )
    reset_update_counter( device )

# finding file containing image for FW update
image_name = product_line[0:2] + "XX_FW_Image-" + bundled_fw_version + ".bin"
image_mask = '(^|/)' + image_name + '$'
image_file = None
for image in file.find( repo.root, image_mask ):
    image_file = os.path.join( repo.root, image)
if not image_file:
    log.f( "Could not find image file for " + product_line + " device with FW version: " + bundled_fw_version )
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
if update_counter < 19:
    test.check_equal( get_update_counter( device ), update_counter + 1)
else:
    test.check_equal( get_update_counter( device ), 1)

test.finish()

test.print_results_and_exit()
