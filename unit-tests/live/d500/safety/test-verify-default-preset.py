# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:device D585S
# test:donotrun:!linux

import pyrealsense2 as rs
from rspy import test,log, tests_wrapper

ctx = rs.context()
dev = ctx.query_devices()[0]
tests_wrapper.start_wrapper(dev)
depth_sensor = dev.first_depth_sensor()

test.start("Check default preset values")
depth_sensor.set_option(rs.option.visual_preset, int(rs.rs400_visual_preset.default))

# check default preset is set
if test.check(depth_sensor.get_option(rs.option.visual_preset) == int(rs.rs400_visual_preset.default)):
    advnc_mode = rs.rs400_advanced_mode(dev)
    test.check(advnc_mode.is_enabled())

    print("Depth Control: \n", advnc_mode.get_depth_control())
    depth_control = advnc_mode.get_depth_control()
    test.check(depth_control.deepSeaMedianThreshold == 97)
    test.check(depth_control.deepSeaSecondPeakThreshold == 0)
    test.check(depth_control.lrAgreeThreshold  == 146)
    test.check(depth_control.minusDecrement == 20)
    test.check(depth_control.scoreThreshB == 3043)

    print("RSM: \n", advnc_mode.get_rsm())
    rsm = advnc_mode.get_rsm()
    test.check(rsm.removeThresh == 115)
    test.check(rsm.rsmBypass == 1)

    print("SLO Penalty Control: \n", advnc_mode.get_slo_penalty_control())
    spc = advnc_mode.get_slo_penalty_control()
    test.check(spc.sloK1PenaltyMod1 == 102)
    test.check(spc.sloK1PenaltyMod2 == 71)
    test.check(spc.sloK2Penalty == 146)
    test.check(spc.sloK2PenaltyMod1 == 192)
    test.check(spc.sloK2PenaltyMod2 == 129)

    print("Depth exposure: \n ", depth_sensor.get_option(rs.option.exposure))
    test.check(depth_sensor.get_option(rs.option.exposure) == 10000)

    color_sensor = dev.first_color_sensor()
    print("Color exposure: \n", color_sensor.get_option(rs.option.exposure))
    print("Color gain: \n", color_sensor.get_option(rs.option.gain))
    print("Color gamma: \n", color_sensor.get_option(rs.option.gamma))
    print("Color power line frequency: \n", color_sensor.get_option(rs.option.power_line_frequency))
    test.check(color_sensor.get_option(rs.option.exposure) == 166)
    test.check(color_sensor.get_option(rs.option.gain) == 16)
    test.check(color_sensor.get_option(rs.option.gamma) == 100)
    test.check(color_sensor.get_option(rs.option.power_line_frequency) == 0)

tests_wrapper.stop_wrapper(dev)
test.finish()
test.print_results_and_exit()
