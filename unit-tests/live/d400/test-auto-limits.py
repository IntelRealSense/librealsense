# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:device D435I
# test:device D455

import pyrealsense2 as rs
from rspy import test

#############################################################################################
with test.closure("Gain/Exposure auto limits"):
    ctx = rs.context()
    device_list = ctx.query_devices()
    devices = [device_list.front(), device_list.front()]
    limits_toggle = [rs.option.auto_exposure_limit_toggle, rs.option.auto_gain_limit_toggle]
    limits_value = [rs.option.auto_exposure_limit, rs.option.auto_gain_limit]
    picked_sensor = [{},{}]
    # 1. Scenario 1:
    #          - Change control value few times
    #         - Turn toggle off
    #          - Turn toggle on
    #          - Check that control limit value is the latest value
    # 2. Scenario 2:
    #         - Init 2 devices
    #          - Change control value for each device to different values
    #          - Toggle off each control
    #          - Toggle on each control
    #          - Check that control limit value in each device is set to the device cached value

    for i in range(2):# 2 controls
        for j in range(2):#2 devices
            sensor = devices[j].first_depth_sensor()
            picked_sensor[j][limits_value[i]] = sensor
            option_range = sensor.get_option_range(limits_value[i])
            # 1. Scenario 1:
            # - Change control value few times
            # - Turn toggle off
            # - Turn toggle on
            # - Check that control limit value is the latest value
            values = [option_range.min + 5.0, option_range.max / 4.0, option_range.max - 5.0]
            for val in values:
                sensor.set_option(limits_value[i], val)
            sensor.set_option(limits_toggle[i], 0.0)  # off
            sensor.set_option(limits_toggle[i], 1.0)  # on
            limit = sensor.get_option(limits_value[i])
            test.check_equal(limit, values[2])

    # 2. Scenario 2:
    #       - Init 2 devices
    #        - toggle on both dev1 and dev2 and set two distinct values for the auto-exposure /gain.
    #        - toggle both dev1and dev2 off.
    #        2.1. toggle dev1 on :
    #                  * verify that the limit value is the value that was stored(cached) in dev1.
    #                  * verify that for dev2 both the limitand the toggle values are similar to those of dev1
    #        2.2. toggle dev2 on :
    #                  * verify that the limit value is the value that was stored(cached) in dev2.

    for i in range(2):  # exposure or gain
        s1 = picked_sensor[0][limits_value[i]]
        s2 = picked_sensor[1][limits_value[i]]

        option_range = s1.get_option_range(limits_value[i]) #should be same range from both sensors
        s1.set_option(limits_value[i], option_range.max / 4.0)
        s1.set_option(limits_toggle[i], 0.0)  # off
        s2.set_option(limits_value[i], option_range.max - 5.0)
        s2.set_option(limits_toggle[i], 0.0)  # off

        #2.1
        s1.set_option(limits_toggle[i], 1.0)  # on
        limit1 = s1.get_option(limits_value[i])
        test.check_equal(limit1, option_range.max / 4.0)
        # keep toggle of dev2 off
        limit2 = s2.get_option(limits_value[i])
        test.check_equal(limit1, limit2)

        # 2.2
        s2.set_option(limits_toggle[i], 1.0)  # on
        limit2 = s2.get_option(limits_value[i])
        test.check_equal(limit2, option_range.max - 5.0)

#############################################################################################
test.print_results_and_exit()
