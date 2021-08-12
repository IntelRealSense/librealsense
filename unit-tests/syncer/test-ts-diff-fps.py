# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import log, test


# The timestamp jumps are closely correlated to the FPS passed to the video streams:
# syncer expects frames to arrive every 1000/FPS milliseconds!
fps_d = 100
fps_c =  10
gap_d = 1000 / fps_d
gap_c = 1000 / fps_c
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

def expect_nothing():
    expect( nothing_else = True )


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
depth_stream.fps = fps_d

depth_profile = rs.video_stream_profile( depth_sensor.add_video_stream( depth_stream ))

color_stream = rs.video_stream()
color_stream.type = rs.stream.color
color_stream.uid = 1
color_stream.width = 640
color_stream.height = 480
color_stream.bpp = 2
color_stream.fmt = rs.format.rgba8
color_stream.fps = fps_c

color_profile = rs.video_stream_profile( color_sensor.add_video_stream( color_stream ))

device.create_matcher( rs.matchers.default )  # Compare all streams according to timestamp

syncer = rs.syncer( 100 )  # We don't want to lose any frames so uses a big queue size (default is 1)

depth_sensor.open( depth_profile )
color_sensor.open( color_profile )
depth_sensor.start( syncer )
color_sensor.start( syncer )

domain = rs.timestamp_domain.hardware_clock
pixels = bytearray( b'\x00' * ( 640 * 480 * 2 ))


#############################################################################################
#
test.start( "Wait for framesets" )

# It can take a few frames for the syncer to actually produce a matched frameset (it doesn't
# know what to match to in the beginning)

# D  C  @timestamp
# -- -- -----------
# 0     @0
# 1     @10
#    0  @0
#
generate_depth_frame( frame_number = 0, timestamp = gap_d * 0 )
generate_depth_frame( 1, gap_d * 1 )  # @10
generate_color_frame( 0, gap_c * 0 )  # @0 -- small latency
expect( depth_frame = 0 )                          # syncer doesn't know about color yet
expect( depth_frame = 1 )
#expect( color_frame = 0, nothing_else = True )
# We'd expect C0 to not wait for another frame, but it does: @0 is comparable to @20 (D.NE)
# because it's using C.fps (10 fps -> 100 gap / 2 = 50ms error, so 0~=20)!
expect_nothing()

# 2     @20
generate_depth_frame( 2, gap_d * 2 )
expect( depth_frame = 2, color_frame = 0, nothing_else = True )

test.finish()
#
#############################################################################################
#
test.start( "Depth waits for next Color" )

# 3     @ 30
# 4     @ 40
# 5     @ 50
# 6     @ 60
# 7     @ 70
# 8     @ 80
# 9     @ 90
# 10    @100 -> wait
# 11    @110
#
generate_depth_frame(  3, gap_d *  3 ); expect( depth_frame =  3 )
generate_depth_frame(  4, gap_d *  4 ); expect( depth_frame =  4 )
generate_depth_frame(  5, gap_d *  5 ); expect( depth_frame =  5 )
generate_depth_frame(  6, gap_d *  6 ); expect( depth_frame =  6 )
generate_depth_frame(  7, gap_d *  7 ); expect( depth_frame =  7 )
generate_depth_frame(  8, gap_d *  8 ); expect( depth_frame =  8 )
generate_depth_frame(  9, gap_d *  9 ); expect( depth_frame =  9 )
generate_depth_frame( 10, gap_d * 10 ); #expect( depth_frame = 10 )
# C.NE is @100, so it should wait...
expect_nothing()
generate_depth_frame( 11, gap_d * 11 );
expect_nothing()

#    1  @100 -> release (D10,C1) and (D11)
generate_color_frame(  1, gap_c *  1 );  # @100 -- small latency
expect( depth_frame = 10, color_frame = 1 )
expect( depth_frame = 11 )
expect_nothing()

# 12    @120 doesn't wait
generate_depth_frame( 12, gap_d * 12 );
expect( depth_frame = 12 )
expect_nothing()

test.finish()
#
#############################################################################################
#
test.start( "Color is early" )

# 13    @130
# 14    @140
# 15    @150
# 16    @160
# 17    @170
#    2  @200 -> wait
# 18    @180 -> (D18,C2) ?! why not wait for (D20,C2) ?!
#
generate_depth_frame( 13, gap_d * 13 ); expect( depth_frame = 13 )
generate_depth_frame( 14, gap_d * 14 ); expect( depth_frame = 14 )
generate_depth_frame( 15, gap_d * 15 ); expect( depth_frame = 15 )
generate_depth_frame( 16, gap_d * 16 ); expect( depth_frame = 16 )
generate_depth_frame( 17, gap_d * 17 ); expect( depth_frame = 17 )

generate_color_frame( 2, gap_c * 2 ); expect_nothing()
# We're waiting for D.NE @180, because it's comparable (using min fps of the two)

generate_depth_frame( 18, gap_d * 18 )
# Now we get both back:
expect( depth_frame = 18, color_frame = 2, nothing_else = True )

# But wait... why match C@200 to D@180 and not wait for D@200??
# If we used the faster FPS of the two, 180!=200: we'd get D18, D19, then (D20,C20)
#expect( depth_frame = 18, nothing_else = True )
#generate_depth_frame( 19, gap_d * 19 )
#expect( depth_frame = 19, nothing_else = True )
#generate_depth_frame( 20, gap_d * 20 )
#expect( depth_frame = 20, color_frame = 2, nothing_else = True )

test.finish()
#
#############################################################################################
#
test.start( "Stop depth" )

generate_color_frame( 3, gap_c * 3 )  # @300

# D.NE is @190, plus 7*gap_d gives a cutout of @260, so the frame shouldn't wait for depth
# to arrive, but it does:
expect_nothing()

# The reason is that it's not using gap_d: it's using gap_c, so cutout is @890:
generate_color_frame( 4, gap_c * 8 )
expect_nothing()
generate_color_frame( 5, gap_c * 9 )
expect( color_frame = 3 )
expect( color_frame = 4 )
expect( color_frame = 5 )
expect_nothing()

test.finish()
#
#############################################################################################
test.print_results_and_exit()
