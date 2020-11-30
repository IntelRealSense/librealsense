import sys
sys.path.insert(0,"C://Users//mmirbach//git//librealsense//build//RelWithDebInfo")
sys.path.insert(0,"C://Users//mmirbach//git//librealsense//unit-tests//py")

from time import sleep
import pyrealsense2 as rs, test

devices = test.find_devices_by_product_line_or_exit(rs.product_line.L500)
device = devices[0]
depth_sensor = device.first_depth_sensor()
sd = rs.serializable_device(device)

visual_preset_number = depth_sensor.get_option(rs.option.visual_preset)
visual_preset_name = rs.l500_visual_preset(int(visual_preset_number))

#############################################################################################
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
        sd.load_json(serialized_json)
        visual_preset_number = depth_sensor.get_option(rs.option.visual_preset)
        visual_preset_name = rs.l500_visual_preset(int(visual_preset_number))
        test.check_equal(visual_preset_name, preset)
    except:
        test.unexpected_exception()
test.finish()

#############################################################################################
test.print_results_and_exit()