# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import test

devices = test.find_devices_by_product_line_or_exit(rs.product_line.L500)
device = devices[0]
depth_sensor = device.first_depth_sensor()
sd = rs.serializable_device(device)

visual_preset_number = depth_sensor.get_option(rs.option.visual_preset)
visual_preset_name = rs.l500_visual_preset(int(visual_preset_number))

#############################################################################################
# The default preset is scheduled to be removed, once that happens this test is irrelevant and should be removed
if visual_preset_name == rs.l500_visual_preset.default:
    test.start("Trying to load default settings from json. Should fail")
    try:
        serialized_json = sd.serialize_json()
        sd.load_json(serialized_json)
    except RuntimeError as e:
        test.check_exception(e, RuntimeError, "The Default preset signifies that the controls have not been changed"
                                              " since initialization, the API does not support changing back to this"
                                              " state, Please choose one of the other presets")
    else:
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