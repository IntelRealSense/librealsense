# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

# test:device D405


import pyrealsense2 as rs
from rspy import test

ctx = rs.context()
dev = ctx.query_devices()[0]
pid = dev.get_info(rs.camera_info.product_id)
print(dev.get_info(rs.camera_info.name) + " found")

#############################################################################################
test.start("explicit configuration")
try:
    pipeline = rs.pipeline()
    config = rs.config()
    config.enable_stream(rs.stream.infrared, 1, 1288, 808, rs.format.y16, 15)
    config.enable_stream(rs.stream.infrared, 2, 1288, 808, rs.format.y16, 15)
    config.enable_stream(rs.stream.color, 1280, 720, rs.format.rgb8, 15)
    pipeline.start(config)

    iteration = 0
    while True:
        iteration = iteration + 1
        if iteration > 10:
            break
        frames = pipeline.wait_for_frames()
        test.check(frames.size() == 3)
        ir_1_stream_found = False
        ir_2_stream_found = False
        color_stream_found = False
        for f in frames:
            profile = f.get_profile()
            if profile.stream_type() == rs.stream.infrared:
                if profile.stream_index() == 1:
                    ir_1_stream_found = True
                elif profile.stream_index() == 2:
                    ir_2_stream_found = True
            elif profile.stream_type() == rs.stream.color:
                color_stream_found = True
        test.check(ir_1_stream_found and ir_2_stream_found and color_stream_found)
    pass

except Exception as e:
    print(e)
    pass

test.finish()

#############################################################################################
test.start("implicit configuration")
try:
    pipeline = rs.pipeline()
    config = rs.config()
    config.enable_stream(rs.stream.infrared, rs.format.y16, 15)
    config.enable_stream(rs.stream.color)
    pipeline.start(config)

    iteration = 0
    while True:
        iteration = iteration + 1
        if iteration > 10:
            break
        frames = pipeline.wait_for_frames()
        test.check(frames.size() == 2)
        ir_1_stream_found = False
        color_stream_found = False
        for f in frames:
            profile = f.get_profile()
            if profile.stream_type() == rs.stream.infrared:
                if profile.stream_index() == 1:
                    ir_1_stream_found = True
            elif profile.stream_type() == rs.stream.color:
                color_stream_found = True
        test.check(ir_1_stream_found and color_stream_found)
    pass

except Exception as e:
    print(e)
    pass

test.finish()


test.print_results_and_exit()
