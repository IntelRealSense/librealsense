## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

###############################################
## pybackend example #1 - A general overview ##
###############################################

# First import the library
import pybackend2 as rs
import time

def on_frame(profile, f):
    print ("Received %d bytes" % f.frame_size)

    # Accessing image pixels
    p = f.pixels
    print ("First 10 bytes are: ")
    for i in range(10):
        print (hex(p[i]))
    print

try:
    # Building a device
    backend = rs.create_backend()
    infos = backend.query_uvc_devices()
    print("There are %d connected UVC devices" % len(infos))
    if len(infos) is 0: exit(1)
    info = infos[2]
    dev = backend.create_uvc_device(info)

    print ("VID=%d, PID=%d, MI=%d, UID=%s" % (info.vid, info.pid, info.mi, info.unique_id))

    # Turn on power
    print ("Move device to D0 power state...")
    dev.set_power_state(rs.D0)

    # Configure streaming
    print ("Print list of UVC profiles supported by the device...")
    profiles = dev.get_profiles()
    for p in profiles:
        print (p)
        # save IR VGA setting for later
        if p.width == 640 and p.height == 480 and p.fps == 30 and p.format == 1196574041:
            vga = p
    first = profiles[0]

    print ("Negotiate Probe-Commit for first profile")
    dev.probe_and_commit(first, on_frame)

    # XU controls
    xu = rs.extension_unit(0, 3, 2, rs.guid("C9606CCB-594C-4D25-af47-ccc496435995")) # Define the Depth XU
    dev.init_xu(xu) # initialize depth XU
    ae = dev.get_xu(xu, 0xB, 1) # get XU value. args: XU, control #, # of bytes
    print ("Auto Exposure option is:", ae)
    print ("Setting Auto Exposure option to new value")
    dev.set_xu(xu, 0x0B, [0x00]) # set XU value. args: XU, control #, list of bytes
    ae = dev.get_xu(xu, 0xB, 1)
    print ("New Auto Exposure setting is:", ae)

    # PU controls
    gain = dev.get_pu(rs.option.gain)
    print ("Gain = %d" % gain)
    print ("Setting gain to new value")
    dev.set_pu(rs.option.gain, 32)
    gain = dev.get_pu(rs.option.gain)
    print ("New gain = %d" % gain)

    # Start streaming
    print ("Start listening for callbacks (for all pins)...")
    dev.start_callbacks()

    print ("Start streaming (from all pins)...")
    dev.stream_on()

    print ("Wait for 5 seconds while frames are expected:")
    time.sleep(5)

    # Close device
    print ("Stop listening for new callbacks...")
    dev.stop_callbacks()

    print ("Close the specific pin...")
    dev.close(first)

    # saving frames to disk
    def save_frame(profile, f):
        f.save_png("pybackend_example_1_general_depth_frame.png", 640, 480, f.frame_size / (640*480))

    print ("Saving an IR VGA frame using profile:", vga)
    dev.probe_and_commit(vga, save_frame)

    dev.set_xu(xu, 0x0B, [0x01]) # turn autoexposure back on
    dev.start_callbacks()
    dev.stream_on()
    time.sleep(1)
    dev.close(vga)

    print ("Move device to D3")
    dev.set_power_state(rs.D3)
    pass
except Exception as e:
    print (e)
    pass