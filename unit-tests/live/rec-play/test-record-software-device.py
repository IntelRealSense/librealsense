# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import test
import numpy as np
import tempfile, os.path

################################################################################################
test.start("Record software-device")

W = 640
H = 480
BPP = 2

temp_dir = tempfile.mkdtemp()
filename = os.path.join(temp_dir, "recording.bag")

video_frame = rs.software_video_frame()
motion_frame = rs.software_motion_frame()

pixels = np.array([100 for i in range(W*H*BPP)], dtype=np.uint8)
motion_frame_data = rs.vector()
motion_frame_data.x = 1.0
motion_frame_data.y = 2.0
motion_frame_data.z = 3.0

sd = rs.software_device()
sensor = sd.add_sensor("Synthetic")

depth_intrinsics = rs.intrinsics()
depth_intrinsics.width = W
depth_intrinsics.height = H
depth_intrinsics.ppx = float(W/2)
depth_intrinsics.ppy = H/2
depth_intrinsics.fx = float(W)
depth_intrinsics.fy = float(H)
depth_intrinsics.model = rs.distortion.brown_conrady
depth_intrinsics.coeffs = [0, 0, 0, 0, 0]

vs = rs.video_stream()
vs.type = rs.stream.depth
vs.index = 0
vs.uid = 0
vs.width = W
vs.height = H
vs.fps = 60
vs.bpp = BPP
vs.fmt = rs.format.z16
vs.intrinsics = depth_intrinsics
depth_stream_profile = sensor.add_video_stream(vs).as_video_stream_profile()

motion_intrinsics = rs.motion_device_intrinsic()
motion_intrinsics.data = [[1.0] * 4] * 3
motion_intrinsics.noise_variances = [2, 2, 2]
motion_intrinsics.bias_variances = [3, 3, 3]

motion_stream = rs.motion_stream()
motion_stream.type = rs.stream.accel
motion_stream.index = 0
motion_stream.uid = 1
motion_stream.fps = 200
motion_stream.fmt = rs.format.motion_raw
motion_stream.intrinsics = motion_intrinsics
motion_stream_profile = sensor.add_motion_stream(motion_stream)

sync = rs.syncer()
stream_profiles = []
stream_profiles.append(depth_stream_profile)
stream_profiles.append(motion_stream_profile)

video_frame.pixels = pixels
video_frame.bpp = BPP
video_frame.stride = W*BPP
video_frame.timestamp = 10000
video_frame.domain = rs.timestamp_domain.hardware_clock
video_frame.frame_number = 0
video_frame.profile = depth_stream_profile.as_video_stream_profile()

motion_frame.data = motion_frame_data
motion_frame.timestamp = 20000
motion_frame.domain = rs.timestamp_domain.hardware_clock
motion_frame.frame_number = 0
motion_frame.profile = motion_stream_profile.as_motion_stream_profile()

recorder = rs.recorder(filename, sd)
sensor.open(stream_profiles)
sensor.start(sync)

sensor.on_video_frame(video_frame)
sensor.on_motion_frame(motion_frame)

sensor.stop()
sensor.close()

recorder.pause()
recorder = None

ctx = rs.context()
player_dev = ctx.load_device(filename)
player_dev.set_real_time(False)
player_sync = rs.syncer()
s = player_dev.query_sensors()[0]
s.open(s.get_stream_profiles())
s.start(player_sync)

fset = rs.frame().as_frameset()
recorded_depth = rs.frame()
recorded_accel = rs.frame()

success, fset = player_sync.try_wait_for_frames()
while success:
    if fset.first_or_default(rs.stream.depth):
        recorded_depth = fset.first_or_default(rs.stream.depth)
    if fset.first_or_default(rs.stream.accel):
        recorded_accel = fset.first_or_default(rs.stream.accel)
    success, fset = player_sync.try_wait_for_frames()

recorded_depth_data = np.hstack(np.asarray(recorded_depth.as_depth_frame().get_data())).view(dtype=np.uint8)

for (i, pixel) in enumerate(pixels):
    test.check_equal(pixel, recorded_depth_data[i])

test.check_equal(video_frame.frame_number, recorded_depth.get_frame_number())
test.check_equal(video_frame.domain, recorded_depth.get_frame_timestamp_domain())
test.check_equal(video_frame.timestamp, recorded_depth.get_timestamp())


recorded_accel_data = recorded_accel.as_motion_frame().get_motion_data()
test.check_equal(motion_frame_data.x, recorded_accel_data.x)
test.check_equal(motion_frame_data.y, recorded_accel_data.y)
test.check_equal(motion_frame_data.z, recorded_accel_data.z)
test.check_equal(motion_frame.frame_number, recorded_accel.get_frame_number())
test.check_equal(motion_frame.domain, recorded_accel.get_frame_timestamp_domain())
test.check_equal(motion_frame.timestamp, recorded_accel.get_timestamp())

s.stop()
s.close()


test.finish()
################################################################################################
test.print_results_and_exit()