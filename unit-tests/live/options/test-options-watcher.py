# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device each(D400*)

import pyrealsense2 as rs
from rspy import test
from rspy import log
import time

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()
product_line = dev.get_info(rs.camera_info.product_line)

changed_options = 0

def notification_callback( opt_list ):
    global changed_options
    log.d( "notification_callback called with {} options".format( len( opt_list ) ) )
    for opt in opt_list:
        log.d( "    Changed option {}".format( opt ) )
        if not depth_sensor.is_option_read_only( opt ): # Ignore accidental temperature changes
            changed_options = changed_options + 1

depth_sensor.on_options_changed( notification_callback )

with test.closure( 'disabling auto exposure' ): # Need to disable or changing gain/exposure might automatically disable it
    depth_sensor.set_option( rs.option.enable_auto_exposure, 0 )
    test.check_equal( depth_sensor.get_option( rs.option.enable_auto_exposure ), 0.0 )
    time.sleep( 1.5 ) # default options-watcher update interval is 1 second

with test.closure( 'set one option' ):
    changed_options = 0
    current_gain = depth_sensor.get_option( rs.option.gain )
    depth_sensor.set_option( rs.option.gain , current_gain + 1 )
    test.check_equal( depth_sensor.get_option( rs.option.gain ), current_gain + 1 )
    time.sleep( 1.5 ) # default options-watcher update interval is 1 second
    test.check_equal( changed_options, 1 )
    changed_options = 0
    
with test.closure( 'set multiple options' ):
    current_gain = depth_sensor.get_option( rs.option.gain )
    depth_sensor.set_option( rs.option.gain , current_gain + 1 )
    test.check_equal( depth_sensor.get_option( rs.option.gain ), current_gain + 1 )
    current_exposure = depth_sensor.get_option( rs.option.exposure )
    depth_sensor.set_option( rs.option.exposure , current_exposure + 1 )
    test.check_equal( depth_sensor.get_option( rs.option.exposure ), current_exposure + 1 )
    time.sleep( 2.5 ) # default options-watcher update interval is 1 second, multiple options might be updated on different intervals
    test.check_equal( changed_options, 2 )
    changed_options = 0
    
test.print_results_and_exit()
