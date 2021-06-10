# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:device L500*
#test:device D400*

import pyrealsense2 as rs
from rspy import test

color_options = [rs.option.backlight_compensation,
                     rs.option.brightness,
                     rs.option.contrast,
                     rs.option.gamma,
                     rs.option.hue,
                     rs.option.saturation,
                     rs.option.sharpness,
                     rs.option.enable_auto_white_balance,
                     rs.option.white_balance]

color_metadata = [rs.frame_metadata_value.backlight_compensation,
                     rs.frame_metadata_value.brightness,
                     rs.frame_metadata_value.contrast,
                     rs.frame_metadata_value.gamma,
                     rs.frame_metadata_value.hue,
                     rs.frame_metadata_value.saturation,
                     rs.frame_metadata_value.sharpness,
                     rs.frame_metadata_value.auto_white_balance_temperature,
                     rs.frame_metadata_value.manual_white_balance]

def check_option_and_metadata_values(option, metadata, value_to_set, frame):
    changed = color_sensor.get_option(option)
    test.check_equal(changed, value_to_set)
    if frame.supports_frame_metadata(metadata):
        changed_md = frame.get_frame_metadata(metadata)
        test.check_equal(changed_md, value_to_set)
    else:
        print("metadata " + repr(metadata) + " not supported")

#############################################################################################
test.start("checking color options")
# test scenario:
# for each option, set value, wait 10 frames, check value with get_option and get_frame_metadata
# values set for each option are min, max, and default values
ctx = rs.context()
dev = ctx.query_devices()[0]

try:
    color_sensor = dev.first_color_sensor()
    # Using a profile common to both L500 and D400
    color_profile = next(p for p in color_sensor.profiles if p.fps() == 30
                         and p.stream_type() == rs.stream.color
                         and p.format() == rs.format.yuyv
                         and p.as_video_stream_profile().width() == 640
                         and p.as_video_stream_profile().height() == 480)
    color_sensor.open(color_profile)
    lrs_queue = rs.frame_queue(capacity=10, keep_frames=False)
    color_sensor.start(lrs_queue)

    iteration = 0
    option_index = -1  # running over options
    value_index = -1  # 0, 1, 2 for min, max, default
    # number of frames to wait between set_option and checking metadata
    # is set as 10 - the expected delay is ~120ms for Win and ~80-90ms for Linux
    num_of_frames_to_wait = 15
    while True:
        try:
            lrs_frame = lrs_queue.wait_for_frame(5000)
            if iteration == 0:
                option_index = option_index + 1
                if option_index == len(color_options):
                    break
                option = color_options[option_index]
                if not color_sensor.supports(option):
                    continue
                option_range = color_sensor.get_option_range(option)
                metadata = color_metadata[option_index]
                # the following if statement is needed because of some bug in FW - see DSO-17221
                # to be removed after this bug is solved
                if option == rs.option.white_balance:
                    color_sensor.set_option(rs.option.enable_auto_white_balance, 0)
                value_to_set = option_range.min
            elif iteration == (num_of_frames_to_wait + 1):
                value_to_set = option_range.max
            elif iteration == 2 * (num_of_frames_to_wait + 1):
                value_to_set = option_range.default
            if iteration % (num_of_frames_to_wait + 1) == 0:  # for iterations 0, 11, 22
                color_sensor.set_option(option, value_to_set)
            if (iteration + 1) % (num_of_frames_to_wait + 1) == 0:  # for iterations 10, 21, 32
                check_option_and_metadata_values(option, metadata, value_to_set, lrs_frame)
            iteration = (iteration + 1) % (3 * (num_of_frames_to_wait + 1))

        except:
            test.unexpected_exception()
except:
    print("The device found has no color sensor")

test.finish()
test.print_results_and_exit()
