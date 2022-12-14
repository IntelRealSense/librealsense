## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#####################################################
##          rs400 advanced mode tutorial           ##
#####################################################

# First import the library
import pyrealsense2 as rs
import time
import json

DS5_product_ids = ["0AD1", "0AD2", "0AD3", "0AD4", "0AD5", "0AF6", "0AFE", "0AFF", "0B00", "0B01", "0B03", "0B07", "0B3A", "0B5C"]

def find_device_that_supports_advanced_mode() :
    ctx = rs.context()
    ds5_dev = rs.device()
    devices = ctx.query_devices();
    for dev in devices:
        if dev.supports(rs.camera_info.product_id) and str(dev.get_info(rs.camera_info.product_id)) in DS5_product_ids:
            if dev.supports(rs.camera_info.name):
                print("Found device that supports advanced mode:", dev.get_info(rs.camera_info.name))
            return dev
    raise Exception("No D400 product line device that supports advanced mode was found")

try:
    dev = find_device_that_supports_advanced_mode()
    advnc_mode = rs.rs400_advanced_mode(dev)
    print("Advanced mode is", "enabled" if advnc_mode.is_enabled() else "disabled")

    # Loop until we successfully enable advanced mode
    while not advnc_mode.is_enabled():
        print("Trying to enable advanced mode...")
        advnc_mode.toggle_advanced_mode(True)
        # At this point the device will disconnect and re-connect.
        print("Sleeping for 5 seconds...")
        time.sleep(5)
        # The 'dev' object will become invalid and we need to initialize it again
        dev = find_device_that_supports_advanced_mode()
        advnc_mode = rs.rs400_advanced_mode(dev)
        print("Advanced mode is", "enabled" if advnc_mode.is_enabled() else "disabled")

    # Get each control's current value
    print("Depth Control: \n", advnc_mode.get_depth_control())
    print("RSM: \n", advnc_mode.get_rsm())
    print("RAU Support Vector Control: \n", advnc_mode.get_rau_support_vector_control())
    print("Color Control: \n", advnc_mode.get_color_control())
    print("RAU Thresholds Control: \n", advnc_mode.get_rau_thresholds_control())
    print("SLO Color Thresholds Control: \n", advnc_mode.get_slo_color_thresholds_control())
    print("SLO Penalty Control: \n", advnc_mode.get_slo_penalty_control())
    print("HDAD: \n", advnc_mode.get_hdad())
    print("Color Correction: \n", advnc_mode.get_color_correction())
    print("Depth Table: \n", advnc_mode.get_depth_table())
    print("Auto Exposure Control: \n", advnc_mode.get_ae_control())
    print("Census: \n", advnc_mode.get_census())

    #To get the minimum and maximum value of each control use the mode value:
    query_min_values_mode = 1
    query_max_values_mode = 2
    current_std_depth_control_group = advnc_mode.get_depth_control()
    min_std_depth_control_group = advnc_mode.get_depth_control(query_min_values_mode)
    max_std_depth_control_group = advnc_mode.get_depth_control(query_max_values_mode)
    print("Depth Control Min Values: \n ", min_std_depth_control_group)
    print("Depth Control Max Values: \n ", max_std_depth_control_group)

    # Set some control with a new (median) value
    current_std_depth_control_group.scoreThreshA = int((max_std_depth_control_group.scoreThreshA - min_std_depth_control_group.scoreThreshA) / 2)
    advnc_mode.set_depth_control(current_std_depth_control_group)
    print("After Setting new value, Depth Control: \n", advnc_mode.get_depth_control())

    # Serialize all controls to a Json string
    serialized_string = advnc_mode.serialize_json()
    print("Controls as JSON: \n", serialized_string)
    as_json_object = json.loads(serialized_string)

    # We can also load controls from a json string
    # For Python 2, the values in 'as_json_object' dict need to be converted from unicode object to utf-8
    if type(next(iter(as_json_object))) != str:
        as_json_object = {k.encode('utf-8'): v.encode("utf-8") for k, v in as_json_object.items()}
    # The C++ JSON parser requires double-quotes for the json object so we need
    # to replace the single quote of the pythonic json to double-quotes
    json_string = str(as_json_object).replace("'", '\"')
    advnc_mode.load_json(json_string)

except Exception as e:
    print(e)
    pass
