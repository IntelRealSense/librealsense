# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import log, test


# The timestamp jumps are closely correlated to the FPS passed to the video streams:
# syncer expects frames to arrive every 1000/FPS milliseconds!
fps = 60
gap = 16  # 1000 / fps
domain = rs.timestamp_domain.hardware_clock

def generate_depth_frame( frame_number, timestamp ):
    """
    """
    global depth_profile, domain, pixels, depth_sensor
    #
    depth_frame = rs.software_video_frame()
    depth_frame.pixels = pixels
    depth_frame.stride = 640 * 2  # W * bpp
    depth_frame.bpp = 2
    depth_frame.frame_number = frame_number
    depth_frame.timestamp = timestamp
    depth_frame.domain = domain
    depth_frame.profile = depth_profile
    #
    log.d( "-->", depth_frame )
    depth_sensor.on_video_frame( depth_frame )

def generate_color_frame( frame_number, timestamp ):
    """
    """
    global color_profile, domain, pixels, color_sensor
    #
    color_frame = rs.software_video_frame()
    color_frame.pixels = pixels
    color_frame.stride = 640 * 2  # W * bpp
    color_frame.bpp = 2
    color_frame.frame_number = frame_number
    color_frame.timestamp = timestamp
    color_frame.domain = domain
    color_frame.profile = color_profile
    #
    log.d( "-->", color_frame )
    color_sensor.on_video_frame( color_frame )

def generate_depth_and_color( frame_number, timestamp ):
    generate_depth_frame( frame_number, timestamp )
    generate_color_frame( frame_number, timestamp )

def expect( depth_frame = None, color_frame = None, nothing_else = False ):
    """
    Looks at the syncer queue and gets the next frame from it if available, checking its contents
    against the expected frame numbers.
    """
    global syncer
    fs = syncer.poll_for_frames()  # NOTE: will never be None
    if not fs:
        test.check( depth_frame is None, "expected a depth frame" )
        test.check( color_frame is None, "expected a color frame" )
        return False

    log.d( "Got", fs )

    depth = fs.get_depth_frame()
    test.info( "actual depth", depth )
    test.check_equal( depth_frame is None, not depth )
    if depth_frame is not None and depth:
        test.check_equal( depth.get_frame_number(), depth_frame )
    
    color = fs.get_color_frame()
    test.info( "actual color", color )
    test.check_equal( color_frame is None, not color )
    if color_frame is not None and color:
        test.check_equal( color.get_frame_number(), color_frame )

    if nothing_else:
        fs = syncer.poll_for_frames()
        test.info( "Expected nothing else; actual", fs )
        test.check( not fs )

    return True


###############################################################################
#
device = rs.software_device()
depth_sensor = device.add_sensor( "Depth" )
color_sensor = device.add_sensor( "Color" )

depth_stream = rs.video_stream()
depth_stream.type = rs.stream.depth
depth_stream.uid = 0
depth_stream.width = 640
depth_stream.height = 480
depth_stream.bpp = 2
depth_stream.fmt = rs.format.z16
depth_stream.fps = fps

depth_profile = rs.video_stream_profile( depth_sensor.add_video_stream( depth_stream ))

#    depth_sensor.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.001f);

color_stream = rs.video_stream()
color_stream.type = rs.stream.color
color_stream.uid = 1
color_stream.width = 640
color_stream.height = 480
color_stream.bpp = 2
color_stream.fmt = rs.format.rgba8
color_stream.fps = fps

color_profile = rs.video_stream_profile( color_sensor.add_video_stream( color_stream ))

device.create_matcher( rs.matchers.default )  # Compare all streams according to timestamp

syncer = rs.syncer( 100 )  # We don't want to lose any frames so uses a big queue size (default is 1)

depth_sensor.open( depth_profile )
color_sensor.open( color_profile )
depth_sensor.start( syncer )
color_sensor.start( syncer )

pixels = bytearray( b'\x00' * ( 640 * 480 * 2 ))


#############################################################################################
#
test.start( "Wait for framesets" )

# It can take a few frames for the syncer to actually produce a matched frameset (it doesn't
# know what to match to in the beginning)

generate_depth_and_color( frame_number = 0, timestamp = 0 )
expect( depth_frame = 0 )                          # syncer doesn't know about color yet
expect( color_frame = 0, nothing_else = True )     # less than next expected of D
#
# NOTE: if the syncer queue wasn't 100 (see above) then we'd only get the color frame!
#
generate_depth_and_color( 1, gap * 1 )
expect( depth_frame = 1, color_frame = 1, nothing_else = True )  # frameset 1

test.finish()
#
#############################################################################################
#
test.start( "Keep going" )

generate_depth_and_color( 2, gap * 2 )
expect( depth_frame = 2, color_frame = 2, nothing_else = True )  # frameset 2
generate_depth_and_color( 3, gap * 3 )
generate_depth_and_color( 4, gap * 4 )
expect( depth_frame = 3, color_frame = 3 )                       # frameset 3
expect( depth_frame = 4, color_frame = 4, nothing_else = True )  # frameset 4

test.finish()
#
#############################################################################################
#
test.start( "Stop giving color; wait_for_frames() should throw (after 5s)" )

generate_depth_frame( 5, gap * 5 )

# We expect the syncer to try and wait for a Color frame to fit the Depth we just gave it.
# The syncer relies on frame inputs to actually run -- and, if we wait for a frame, we're
# not feeding it frames so can't get back any, either.
try:
    fs = syncer.wait_for_frames()
except RuntimeError as e:
    test.check_exception( e, RuntimeError, "Frame did not arrive in time!" )
else:
    test.info( "Unexpected frameset", fs )
    test.unreachable()

test.finish()
#
#############################################################################################
#
test.start( "try_wait_for_frames() allows us to keep feeding frames; eventually we get one" )

# The syncer will not immediately release framesets because it's still waiting for a color frame to
# match Depth #10. But, if we advance the timestamp sufficiently, it should eventually release it
# as it deems Color "inactive".

# The last color frame we sent was at gap*4, so it's next expected at gap*5 plus a buffer of gap*7
# (see the code in the syncer)... so we expect depth frame 5 to be released only when we're past gap*12

generate_depth_frame( 6, gap * 6 )
expect( nothing_else = True )
generate_depth_frame( 7, gap * 12 )
expect( nothing_else = True )
generate_depth_frame( 8, gap * 13 )
#
# We'll get all frames in a burst. Again, if the syncer queue was 1, we'd only get the last!
#
expect( depth_frame = 5 )
expect( depth_frame = 6 )
expect( depth_frame = 7 )
expect( depth_frame = 8, nothing_else = True )

test.finish()
#
#############################################################################################
test.print_results_and_exit()
