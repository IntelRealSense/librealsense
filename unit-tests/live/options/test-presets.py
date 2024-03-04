# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device each(D400*)

# See FW stability issue RSDSO-18908
# test:retries 2

import pyrealsense2 as rs
from rspy import test
from rspy import log
from rspy import tests_wrapper as tw

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()
product_line = dev.get_info(rs.camera_info.product_line)

tw.start_wrapper( dev )

with test.closure( 'visual preset support', on_fail=test.ABORT ): # No use continuing the test if there is no preset support
    test.check( depth_sensor.supports( rs.option.visual_preset ) )

with test.closure( 'set presets' ):
    depth_sensor.set_option( rs.option.visual_preset, int(rs.rs400_visual_preset.high_accuracy ) )
    test.check( depth_sensor.get_option( rs.option.visual_preset ) == rs.rs400_visual_preset.high_accuracy )
    depth_sensor.set_option( rs.option.visual_preset, int(rs.rs400_visual_preset.default ) )
    test.check( depth_sensor.get_option( rs.option.visual_preset ) == rs.rs400_visual_preset.default )

with test.closure( 'save/load preset' ):
    am_dev = rs.rs400_advanced_mode(dev)
    saved_values = am_dev.serialize_json()
    depth_control_group = am_dev.get_depth_control()
    depth_control_group.textureCountThreshold = 250
    am_dev.set_depth_control( depth_control_group )
    test.check( depth_sensor.get_option( rs.option.visual_preset ) == rs.rs400_visual_preset.custom )
    
    am_dev.load_json( saved_values )
    test.check( am_dev.get_depth_control().textureCountThreshold != 250 )

with test.closure( 'setting color options' ):
    # Using Hue to test if setting visual preset changes color sensor settings.
    # Not all cameras support Hue (e.g. D457) but using common setting like Gain or Exposure is dependant on auto-exposure logic
    # This test is intended to check new D500 modules logic of not updating color sensor setting, while keeping legacy
    # D400 devices behavior of updating it. For this purpose it is OK if not all module types will be tested.
    if color_sensor.supports( rs.option.hue ):
        color_sensor.set_option( rs.option.hue, 123 )
        test.check( color_sensor.get_option( rs.option.hue ) == 123 )
    
        depth_sensor.set_option( rs.option.visual_preset, int(rs.rs400_visual_preset.default ) )
        if product_line == "D400":
            # D400 devices set color options as part of preset setting
            test.check( color_sensor.get_option( rs.option.hue ) != 123 )
        elif product_line == "D500":
            # D500 devices do not set color options as part of preset setting
            test.check( color_sensor.get_option( rs.option.hue ) == 123 )
        else:
            raise RuntimeError( 'unsupported product line' )

tw.stop_wrapper( dev )    
test.print_results_and_exit()
