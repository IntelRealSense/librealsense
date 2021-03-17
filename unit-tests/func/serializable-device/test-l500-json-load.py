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

def json_to_dict( json ):
    """
    :param json: a string representing a json file
    :return: a dictionary with all settings
    """
    translation_table = dict.fromkeys(map(ord, '",\''), None)
    json_dict = {}
    for line in json.splitlines():
        if ':' not in line: # ignoring lines that are not for settings such as empty lines
            continue
        setting, value = line.split(':')
        setting = setting.strip().translate(translation_table)
        value = value.strip().translate(translation_table)
        json_dict[ setting ] = value
    return json_dict

def log_settings_differences( data ):
    global depth_sensor, sd
    depth_sensor.set_option(rs.option.visual_preset, int(rs.l500_visual_preset.low_ambient_light))
    actual_data = str( sd.serialize_json() )
    data_dict = json_to_dict( data )
    actual_data_dict = json_to_dict( actual_data )
    log.debug_indent()
    try:
        # logging the differences in the settings between the expected and the actual values
        for key in actual_data_dict.keys():
            if key not in data_dict:
                log.d( "New setting added to json:", key)
            elif "Visual Preset" in key or "Temperature" in key or "temperature" in key:
                # the line regarding the visual preset will always be different because we load 1 from data but set it to
                # 3 for low ambient. Also all lines regarding temperatures depend on the camera and don't affect the preset
                continue
            elif data_dict[ key ] != actual_data_dict[ key ]:
                log.d( key, "was expected to have value of", data_dict[ key ],
                       "but actually had value of", actual_data_dict[ key ])
    finally:
        log.debug_unindent()

#############################################################################################
# This test checks backward compatibility to old json files that saved with default preset
# The default preset is deprecated but json files that saved with default preset
# should be supported
# Here, we expect the data to fit a pre-defined "low-ambient" preset
# Note that, because FW defines them, the defaults can change from version to version and we need to keep this
# up-to-date with the latest. If the FW changes, this test will fail and output (in debug mode) any settings that
# changed...


low_ambient_data_with_default_preset = """
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
    "Visual Preset": 1
}
"""

test.start("Trying to load settings with default preset from json")
try:
    sd.load_json( low_ambient_data_with_default_preset )
    visual_preset_number = depth_sensor.get_option(rs.option.visual_preset)
    visual_preset_name = rs.l500_visual_preset(int(visual_preset_number))

    # if this check fails it is most likely because FW changed the default settings
    equal = test.check_equal(visual_preset_name, rs.l500_visual_preset.low_ambient_light)
    if not equal:
        log.w( "It is possible that FW changed the default settings of the camera." )
        log_settings_differences( low_ambient_data_with_default_preset )
except:
    test.unexpected_exception()
test.finish()

#############################################################################################
# There is no default preset: so when one is specified, the code should calculate the preset!
# Here, we intentionally should not fit into any of the defined presets, and check that the result is CUSTOM

wrong_data_with_default_preset = """
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
    "Min Distance": 490,
    "Noise Estimation": 0.0,
    "Noise Filtering": 4,
    "Post Processing Sharpening": 1,
    "Pre Processing Sharpening": 0.0,
    "Receiver Gain": 18,
    "Reset Camera Accuracy Health": 0.0,
    "Sensor Mode": 0.0,
    "Trigger Camera Accuracy Health": 0.0,
    "Visual Preset": 1
}
"""

test.start("Trying to load wrong settings, should get custom preset")
try:
    sd.load_json( wrong_data_with_default_preset )
    visual_preset_number = depth_sensor.get_option(rs.option.visual_preset)
    visual_preset_name = rs.l500_visual_preset(int(visual_preset_number))

    test.check_equal(visual_preset_name, rs.l500_visual_preset.custom)
except:
    test.unexpected_exception()
test.finish()

#############################################################################################
# Here, the json specifies a "low-ambient" preset -- so the values of the individual controls (that are part of this preset)
# should not matter. The result of loading the following should still result in LOW_AMBIENT even when the controls
# do not fit (because they're overridden).

wrong_data_with_low_ambient_preset = """
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
    "Min Distance": 490,
    "Noise Estimation": 0.0,
    "Noise Filtering": 4,
    "Post Processing Sharpening": 1,
    "Pre Processing Sharpening": 0.0,
    "Receiver Gain": 18,
    "Reset Camera Accuracy Health": 0.0,
    "Sensor Mode": 0.0,
    "Trigger Camera Accuracy Health": 0.0,
    "Visual Preset": 3
}
"""

test.start("Trying to load wrong settings with specified preset")
try:
    sd.load_json( wrong_data_with_low_ambient_preset )
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
