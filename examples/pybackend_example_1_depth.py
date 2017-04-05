## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#################################################
## pybackend example #1 - Accessing depth data ##
#################################################

# First import the library
import pybackend2 as rs
import time

def on_frame(profile, f):
    print "Received %d bytes" % f.frame_size

backend = rs.create_backend()
infos = backend.query_uvc_devices()
print("There are %d connected UVC devices" % len(infos))
if len(infos) is 0: exit(1)
dev = backend.create_uvc_device(infos[0])

print "VID=%d, PID=%d, MI=%d, UID=%s" % (infos[0].vid, infos[0].pid, infos[0].mi, infos[0].unique_id)

print "Move device to D0 power state..."
dev.set_power_state(rs.D0)

print "Print list of UVC profiles supported by the device..."
profiles = dev.get_profiles()
for p in profiles:
    print p
first = profiles[0]

print "Negotiate Probe-Commit for first profile"
dev.probe_and_commit(first, on_frame)

gain = dev.get_pu(rs.option.gain)
print "Gain = %d" % gain
print "Setting gain to new value"
#dev.set_pu(rs.option.gain, 32) - broken on RS2 :/
gain = dev.get_pu(rs.option.gain)
print "New gain = %d" % gain

print "Start listening for callbacks (for all pins)..."
dev.start_callbacks()

print "Start streaming (from all pins)..."
dev.stream_on()

print "Wait for 5 seconds while frames are expected:"
time.sleep(5)

print "Stop listening for new callbacks..."
dev.stop_callbacks()

print "Close the specific pin..."
dev.close(first)

print "Move device to D3"
dev.set_power_state(rs.D3)
