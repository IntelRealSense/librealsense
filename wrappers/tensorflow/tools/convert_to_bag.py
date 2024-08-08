import numpy as np
import cv2
import pyrealsense2 as rs
import time, sys, glob

focal = 0.0021
baseline = 0.08

sd = rs.software_device()
depth_sensor = sd.add_sensor("Depth")

intr = rs.intrinsics()
intr.width = 848
intr.height = 480
intr.ppx = 637.951293945312
intr.ppy = 360.783233642578
intr.fx = 638.864135742188
intr.fy = 638.864135742188

vs = rs.video_stream()
vs.type = rs.stream.infrared
vs.fmt = rs.format.y8
vs.index = 1
vs.uid = 1
vs.width = intr.width
vs.height = intr.height
vs.fps = 30
vs.bpp = 1
vs.intrinsics = intr
depth_sensor.add_video_stream(vs)

vs.type = rs.stream.depth
vs.fmt = rs.format.z16
vs.index = 1
vs.uid = 3
vs.bpp = 2
depth_sensor.add_video_stream(vs)

vs.type = rs.stream.depth
vs.fmt = rs.format.z16
vs.index = 2
vs.uid = 4
vs.bpp = 2
depth_sensor.add_video_stream(vs)

vs.type = rs.stream.depth
vs.fmt = rs.format.z16
vs.index = 3
vs.uid = 5
vs.bpp = 2
depth_sensor.add_video_stream(vs)

depth_sensor.add_read_only_option(rs.option.depth_units, 0.001)
name = "virtual camera"
sd.register_info(rs.camera_info.name, name)

ctx = rs.context()
sd.add_to(ctx)

dev = ctx.query_devices()[0]
for d in ctx.query_devices():
    if d.get_info(rs.camera_info.name) == name:
        dev = d

images_path = "."
if (len(sys.argv) > 1):
    images_path = str(sys.argv[1])

rec = rs.recorder(images_path + "/1.bag", dev)
sensor = rec.query_sensors()[0]

q = rs.frame_queue()
sensor.open(sensor.get_stream_profiles())
sensor.start(q)

files = glob.glob1(images_path, "gt*")
index = []
for f in files:
    idx = (f.split('-')[1]).split('.')[0]
    index.append(int(idx))

for i in index:
    left_name = images_path + "/left-" + str(i) + ".png"
    depth_name = images_path + "/gt-" + str(i) + ".png"
    result_name = images_path + "/res-" + str(i) + ".png"
    denoised_name = images_path + "/res_denoised-" + str(i) + ".png"

    img = cv2.imread(left_name)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    f = rs.software_video_frame()
    f.stride = intr.width
    f.bpp = 1
    f.pixels = np.asarray(img, dtype="byte")
    f.timestamp = i * 0.01
    f.frame_number = i
    f.profile = sensor.get_stream_profiles()[0].as_video_stream_profile()
    depth_sensor.on_video_frame(f)

    time.sleep(0.01)

    f3 = rs.software_video_frame()
    img = cv2.imread(result_name, cv2.IMREAD_ANYDEPTH)
    f3.stride = 2 * intr.width
    f3.bpp = 2
    px = np.asarray(img, dtype="ushort")
    f3.pixels = px
    f3.timestamp = i * 0.01
    f3.frame_number = i
    f3.profile = sensor.get_stream_profiles()[1].as_video_stream_profile()
    depth_sensor.on_video_frame(f3)

    time.sleep(0.01)

    f4 = rs.software_video_frame()
    img = cv2.imread(depth_name, cv2.IMREAD_ANYDEPTH)
    f4.stride = 2 * intr.width
    f4.bpp = 2
    px = np.asarray(img, dtype="ushort")
    f4.pixels = px
    f4.timestamp = i * 0.01
    f4.frame_number = i
    f4.profile = sensor.get_stream_profiles()[2].as_video_stream_profile()
    depth_sensor.on_video_frame(f4)
    time.sleep(0.01)

    f5 = rs.software_video_frame()
    img = cv2.imread(denoised_name, cv2.IMREAD_ANYDEPTH)
    f5.stride = 2 * intr.width
    f5.bpp = 2
    px = np.asarray(img, dtype="ushort")
    f5.pixels = px
    f5.timestamp = i * 0.01
    f5.frame_number = i
    f5.profile = sensor.get_stream_profiles()[3].as_video_stream_profile()
    depth_sensor.on_video_frame(f5)
    time.sleep(0.01)

    time.sleep(1)

print("a")
f = q.wait_for_frame()
print("b")

time.sleep(1)

sensor.stop()
sensor.close()
