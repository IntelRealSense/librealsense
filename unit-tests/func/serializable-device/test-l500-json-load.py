# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#test:device L500*

import pyrealsense2 as rs
from rspy import test, log

devices = test.find_devices_by_product_line_or_exit(rs.product_line.L500)
device = devices[0]
depth_sensor = device.first_depth_sensor()
sd = rs.serializable_device(device)

visual_preset_number = depth_sensor.get_option(rs.option.visual_preset)
visual_preset_name = rs.l500_visual_preset(int(visual_preset_number))

#############################################################################################
# This test checks backward compatibility to old json files that saved with default preset
# The default preset is deprecated but json files that saved with default preset
# should be support

data = """
{
    "Alternate IR": 0.0,
    "Apd Temperature": -9999,
    "Confidence Threshold": 1,
    "Depth Offset": 4.5,
    "Depth Units": 0.000250000011874363,
    "Digital Gain": 2,
    "Enable IR Reflectivity": 0.0,
    "Enable Max Usable Range": 0.0,
    "Error Polling Enabled": 1,
    "Frames Queue Size": 16,
    "Freefall Detection Enabled": 1,
    "Global Time Enabled": 0.0,
    "Host Performance": 0.0,
    "Humidity Temperature": 32.8908233642578,
    "Inter Cam Sync Mode": 0.0,
    "Invalidation Bypass": 0.0,
    "LDD temperature": 32.1463623046875,
    "Laser Power": 100,
    "Ma Temperature": 39.667610168457,
    "Mc Temperature": 31.6955661773682,
    "Min Distance": 190,
    "Noise Estimation": 0.0,
    "Noise Filtering": 4,
    "Post Processing Sharpening": 1,
    "Pre Processing Sharpening": 0.0,
    "Receiver Gain": 18,
    "Reset Camera Accuracy Health": 0.0,
    "Sensor Mode": 0.0,
    "Trigger Camera Accuracy Health": 0.0,
    "Visual Preset": 1,
    "Zero Order Enabled": 0.0,
    "stream-depth-format": "Z16",
    "stream-fps": "30",
    "stream-height": "480",
    "stream-ir-format": "Y8",
    "stream-width": "640"
}
"""

def log_settings_differences():
    global data, depth_sensor, sd
    depth_sensor.set_option(rs.option.visual_preset, int(rs.l500_visual_preset.low_ambient_light))
    actual_data = str( sd.serialize_json() )
    log.debug_indent()
    try:
        # logging the differences in the settings between the expected and the actual values
        log.d( "Printing differences between expected and actual settings values:" )
        for expected_line, actual_line in zip( data.splitlines()[2:-1], actual_data.splitlines()[1:-1] ):
            # data starts with an empty line and a '{' and ends with a '}', so we cut out the first 2 lines and the last
            # line to ignore them. Same with the actual data ony it doesn't start with an empty line
            if "Visual Preset" in expected_line or "Temperature" in expected_line or "temperature" in expected_line:
                # the line regarding the visual preset will always be different because we load 1 from data but set it to
                # 3 for low ambient. Also all lines regarding temperatures depend on the camera and don't affect the preset
                continue
            if expected_line != actual_line:
                log.d( "Expected to find line:", expected_line)
                log.d( "But found:            ", actual_line)
    finally:
        log.debug_unindent()

test.start("Trying to load default settings from json")
try:
    sd.load_json(data)
    visual_preset_number = depth_sensor.get_option(rs.option.visual_preset)
    visual_preset_name = rs.l500_visual_preset(int(visual_preset_number))

    # if this check fails it is most likely because FW changed the default settings
    equal = test.check_equal(visual_preset_name, rs.l500_visual_preset.low_ambient_light)
    if not equal:
        log.w( "It is possible that FW changed the default settings of the camera." )
        log_settings_differences()
except:
    test.unexpected_exception()
test.finish()

#############################################################################################
test.start("Trying to load non default presets")
presets = [rs.l500_visual_preset.custom,
           rs.l500_visual_preset.no_ambient_light,
           rs.l500_visual_preset.low_ambient_light,
           rs.l500_visual_preset.max_range,
           rs.l500_visual_preset.short_range]

for preset in presets:
    try:
        depth_sensor.set_option(rs.option.visual_preset, int(preset))
        serialized_json = sd.serialize_json()
        # Changing the preset to make sure the load function changes it back
        if preset == rs.l500_visual_preset.custom:
            depth_sensor.set_option(rs.option.visual_preset, int(rs.l500_visual_preset.short_range))
        else:
            depth_sensor.set_option(rs.option.visual_preset, int(rs.l500_visual_preset.custom))
        sd.load_json(serialized_json)
        visual_preset_number = depth_sensor.get_option(rs.option.visual_preset)
        visual_preset_name = rs.l500_visual_preset(int(visual_preset_number))
        test.check_equal(visual_preset_name, preset)
    except:
        test.unexpected_exception()
test.finish()

#############################################################################################
test.print_results_and_exit()
