# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import log, test
import sw


# The timestamp jumps are closely correlated to the FPS passed to the video streams:
# syncer expects frames to arrive every 1000/FPS milliseconds!
sw.fps_c = sw.fps_d = 30
sw.init( syncer_matcher = rs.matchers.dic_c )
sw.start()

#############################################################################################
#
test.start( "Init" )

# It can take a few frames for the syncer to actually produce a matched frameset (it doesn't
# know what to match to in the beginning)

# D  C  @timestamp  comment
# -- -- ----------- ----------------
# 0     @0          so next expected frame timestamp is at 0+16.67
#    0  @0
#
sw.generate_depth_and_color( frame_number = 0, timestamp = 0 )
sw.expect( depth_frame = 0 )                          # syncer doesn't know about C yet, so releases right away
sw.expect( color_frame = 0, nothing_else = True )     # no hope for a match: D@0 is already out, so it's released
#
# The syncer now knows about both streams, and is empty -- that was what we wanted

test.finish()
#
#############################################################################################
#
test.start( "Go past Color's Next Expected; get a lone Depth frame" )

# 1     @7952 -> NE=7985; it's released because WAY past C.NE
#
sw.generate_depth_frame( 1, 7952 )
sw.expect( depth_frame = 1, nothing_else = True )

test.finish()
#
#############################################################################################
#
test.start( "Generate a Color frame which will wait for Depth" )

#    2  @7978 will wait, as it's ~= D.NE
#
sw.generate_color_frame( 2, 7978 )
sw.expect_nothing()

test.finish()
#
#############################################################################################
#
test.start( "Generate Depth for release BEFORE the waiting Color" )

# 3     @7952 -> needs to be released BEFORE C2!!
#
# NOTE: the timestamp is the SAME AS BEFORE! Imagine that, instead of a Depth frame, this was
# an Infrared: the matcher would be (TS: (TS: Depth Infra Confidence) Color). But we have no
# Infra or Confidence mechanism (in sw) so we just generate another D -- it should have the
# same effect:
#
# NOTE: this used to crash (see LRS-289)!
#
sw.generate_depth_frame( 3, 7952 )
sw.expect( depth_frame = 3 )

test.finish()
#
#############################################################################################
#
test.start( "And only then get the Color when we generate a matching Depth" )

sw.expect_nothing()  # C is still waiting for D.NE!

# 4     @7986
#
sw.generate_depth_frame( 4, 7986 )
sw.expect( depth_frame = 4, color_frame = 2, nothing_else = True )

test.finish()
#
#############################################################################################
test.print_results_and_exit()
