# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#test:device L500*

import pyrealsense2 as rs
from rspy import test

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
    "Min Distance": 95,
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

test.start("Trying to load default settings from json")
try:
    sd.load_json(data)
    visual_preset_number = depth_sensor.get_option(rs.option.visual_preset)
    visual_preset_name = rs.l500_visual_preset(int(visual_preset_number))

    test.check_equal(visual_preset_name, rs.l500_visual_preset.low_ambient_light)
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
