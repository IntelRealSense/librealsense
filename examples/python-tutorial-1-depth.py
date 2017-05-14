## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#####################################################
## librealsense tutorial #1 - Accessing depth data ##
#####################################################

# First import the library
import pyrealsense2 as rs

try:
    # Create a context object. This object owns the handles to all connected realsense devices
    ctx = rs.context()
    devices = ctx.query_devices()
    print("There are %d connected RealSense devices" % len(devices))
    if len(devices) is 0: exit(1)

    # This tutorial will access only a single device, but it is trivial to extend to multiple devices
    dev = devices[0]
    print("\nUsing device 0, an %s" % dev.get_camera_info(rs.camera_info.device_name))
    print("    Serial number: %s" % dev.get_camera_info(rs.camera_info.device_serial_number))
    print("    Firmware version: %s" % dev.get_camera_info(rs.camera_info.camera_firmware_version))

    # Configure depth to run at VGA resolution at 30 frames per second
    dev.open(rs.stream_profile(rs.stream.depth, 640, 480, 30, rs.format.z16))

    # Configure frame queue of size one and start streaming into it
    queue = rs.frame_queue(1)
    dev.start(queue)

    # Determine depth value corresponding to one meter
    one_meter = int(1.0 / dev.get_depth_scale())

    while True:
        # This call waits until a new coherent set of frames is available on a device
        # Calls to get_frame_data(...) and get_frame_timestamp(...) on a device will return stable values until wait_for_frames(...) is called
        frame = queue.wait_for_frame()

        # Retrieve depth data, which was previously configured 640 x 480 image of 16-bit depth values
        frame_data = frame.get_data()
        depth_frame = [0]*(len(frame_data)/2)
        for i in xrange(len(frame_data)/2):
            depth_frame[i] = (frame_data[i*2] + frame_data[i*2+1]*256)

        # Print a simple text-based representation of the image, by breaking it into 10x20 pixel regions and approximating the coverage of pixels within one meter
        pixel = 0
        coverage = [0]*64
        for y in xrange(480):
            for x in xrange(640):
                depth = depth_frame[pixel]
                pixel += 1
                if 0 < depth and depth < one_meter:
                    coverage[x/10] += 1
            
            if y%20 is 19:
                line = ""
                for c in coverage:
                    line += " .:nhBXWW"[c/25]
                coverage = [0]*64
                print(line)
    exit(0)
#except rs.error as e:
#    # Method calls agaisnt librealsense objects may throw exceptions of type pylibrs.error
#    print("pylibrs.error was thrown when calling %s(%s):\n", % (e.get_failed_function(), e.get_failed_args()))
#    print("    %s\n", e.what())
#    exit(1)
except: print("oops")