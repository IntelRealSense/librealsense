# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

# we want this test to run first so that all tests run with updated FW versions, so we give it priority 0
#test:priority 0
#test:device each(L500*)
#test:device each(D400*)

import pyrealsense2 as rs, sys, os, subprocess
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
for tool in file.find( repo.build, '(^|/)rs-fw-update.exe$' ):
    fw_updater_exe = os.path.join( repo.build, tool )
if not fw_updater_exe:
    log.f( "Could not find the update tool file (rs-fw-update.exe)" )

devices.query( monitor_changes = False )
sn_list = devices.all()
# acroname should ensure there is always 1 available device
if len( sn_list ) != 1:
    log.f( "Expected 1 device, got", len( sn_list ) )
device = devices.get_first( sn_list ).handle
log.d( 'found:', device )
product_line = device.get_info( rs.camera_info.product_line )
log.d( 'product line:', product_line )

###############################################################################
#
test.start( "Update FW" )
# check if recovery. If so recover
recovered = False
if device.is_update_device():
    log.d( "recovering device ..." )
    try:
        # TODO: this needs to improve for L535
        image_name = product_line[:-2] + "XX_FW_Image-"
        image_mask = '(^|/)' + image_name + '(\d+\.){4}bin$'
        image_file = None
        for image in file.find( repo.root, image_mask ):
            image_file = image
        if not image_file:
            log.f( "Could not find image file for", product_line, "recovery device" )

        cmd = [fw_updater_exe, '-r', '-f', image_file]
        log.d( 'running:', cmd )
        subprocess.run( cmd )
        recovered = True
    except Exception as e:
        test.unexpected_exception()
        log.f( "Unexpected error while trying to recover device:", e )
    else:
        devices.query( monitor_changes = False )
        device = devices.get_first( devices.all() ).handle

current_fw_version = repo.pretty_fw_version( device.get_info( rs.camera_info.firmware_version ))
log.d( 'FW version:', current_fw_version )
bundled_fw_version = repo.pretty_fw_version( device.get_info( rs.camera_info.recommended_firmware_version ) )
log.d( 'bundled FW version:', bundled_fw_version )

def compare_fw_versions( v1, v2 ):
    """
    :param v1: left FW version
    :param v2: right FW version
    :return: 1 if v1 > v2; -1 is v1 < v2; 0 if they're equal
    """
    v1_list = v1.split( '.' )
    v2_list = v2.split( '.' )
    if len(v1_list) != 4:
        raise RuntimeError( "FW version (left) '" + v1 + "' is invalid" )
    if len(v2_list) != 4:
        raise RuntimeError( "FW version (right) '" + v2 + "' is invalid" )
    for n1, n2 in zip( v1_list, v2_list ):
        if int(n1) > int(n2):
            return 1
        if int(n1) < int(n2):
            return -1
    return 0

if compare_fw_versions( current_fw_version, bundled_fw_version ) == 0:
    # Current is same as bundled
    if recovered or test.context != 'nightly':
        # In nightly, we always update; otherwise we try to save time, so do not do anything!
        log.d( 'versions are same; skipping FW update' )
        test.finish()
        test.print_results_and_exit()
else:
    # It is expected that, post-recovery, the FW versions will be the same
    test.check( not recovered, abort_if_failed = True )

update_counter = get_update_counter( device )
log.d( 'update counter:', update_counter )
if update_counter >= 19:
    log.d( 'resetting update counter' )
    reset_update_counter( device )
    update_counter = 0

# finding file containing image for FW update
image_name = product_line[0:2] + "XX_FW_Image-" + bundled_fw_version + ".bin"
image_mask = '(^|/)' + image_name + '$'
image_file = None
for image in file.find( repo.root, image_mask ):
    image_file = os.path.join( repo.root, image)
if not image_file:
    log.f( "Could not find image file for " + product_line + " device with FW version: " + bundled_fw_version )

cmd = [fw_updater_exe, '-f', image_file]
log.d( 'running:', cmd )
sys.stdout.flush()
subprocess.run( cmd )   # may throw

# make sure update worked
devices.query( monitor_changes = False )
sn_list = devices.all()
device = devices.get_first( sn_list ).handle
current_fw_version = repo.pretty_fw_version( device.get_info( rs.camera_info.firmware_version ))
test.check_equal( current_fw_version, bundled_fw_version )
new_update_counter = get_update_counter( device )
# According to FW: "update counter zeros if you load newer FW than (ever) before"
if new_update_counter > 0:
    test.check_equal( new_update_counter, update_counter + 1 )

test.finish()
#
###############################################################################

test.print_results_and_exit()
