# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import log, test
import sw


# The timestamp jumps are closely correlated to the FPS passed to the video streams:
# syncer expects frames to arrive every 1000/FPS milliseconds!
sw.fps_d = 30
sw.fps_c = 30
sw.init()
sw.start()

#############################################################################################
with test.closure( "Init" ):
    # It can take a few frames for the syncer to actually produce a matched frameset (it doesn't
    # know what to match to in the beginning)

    # (sync.cpp:396) ... missing Color/66, next expected 63231.348758
    sw.generate_color_frame( frame_number=1730, timestamp=63198.01542, next_expected=63231.348758 )
    sw.expect( color_frame=1730 )   # syncer doesn't know about D yet, so releases right away
    sw.generate_depth_frame( frame_number=1742, timestamp=63214.73 )
    sw.expect_nothing()  # should be waiting for next color

#############################################################################################
with test.closure( "Sync D1742 & C1731" ):
    sw.generate_depth_frame( frame_number=1743, timestamp=63248.07 )
    sw.expect_nothing()  # still waiting for next color with 1742
    sw.generate_color_frame( frame_number=1731, timestamp=63230.22, next_expected=63264.698758 )
    sw.expect( depth_frame=1742, color_frame=1731, nothing_else=True )     # no hope for a match: D1743 is already out, so it's released

#############################################################################################
with test.closure( "Sync D1743 & C1732" ):
    sw.generate_depth_frame( frame_number=1744, timestamp=63281.41 )
    sw.expect_nothing()  # should be waiting for next color
    sw.generate_color_frame( frame_number=1732, timestamp=63263.57, next_expected=63298.049758 )
    sw.expect( depth_frame=1743, color_frame=1732, nothing_else=True )

#############################################################################################
with test.closure( "Sync D1744 & C1733" ):
    sw.generate_depth_frame( frame_number=1745, timestamp=63314.76 )
    sw.expect_nothing()  # should be waiting for next color
    sw.generate_color_frame( frame_number=1733, timestamp=63296.92, next_expected=63331.399758 )
    sw.expect( depth_frame=1744, color_frame=1733, nothing_else=True )

#############################################################################################
with test.closure( "Sync D1745 & C1734" ):
    sw.generate_depth_frame( frame_number=1746, timestamp=63348.10 )
    sw.expect_nothing()  # should be waiting for next color
    sw.generate_color_frame( frame_number=1734, timestamp=63330.27, next_expected=63364.750758 )
    sw.expect( depth_frame=1745, color_frame=1734, nothing_else=True )

#############################################################################################
with test.closure( "Sync D1746 & C1735" ):
    sw.generate_depth_frame( frame_number=1747, timestamp=63381.45 )
    sw.expect_nothing()  # should be waiting for next color
    sw.generate_color_frame( frame_number=1735, timestamp=63363.62, next_expected=63398.100758 )
    sw.expect( depth_frame=1746, color_frame=1735, nothing_else=True )

#############################################################################################
with test.closure( "Sync D1747 & C1736" ):
    sw.generate_depth_frame( frame_number=1748, timestamp=63414.79 )
    sw.expect_nothing()  # should be waiting for next color
    sw.generate_color_frame( frame_number=1736, timestamp=63396.97, next_expected=63431.451758 )
    sw.expect( depth_frame=1747, color_frame=1736, nothing_else=True )

#############################################################################################
with test.closure( "Sync D1748 & C1737" ):
    sw.generate_depth_frame( frame_number=1749, timestamp=63448.13 )
    sw.expect_nothing()  # should be waiting for next color
    sw.generate_color_frame( frame_number=1737, timestamp=63430.32, next_expected=63464.801758 )
    sw.expect( depth_frame=1748, color_frame=1737 )

#############################################################################################
with test.closure( "LONE D1749!" ):
    sw.expect( depth_frame=1749, nothing_else=True )


#ing-block.cpp:55) --> syncing [Depth/63 #1750 @63481.48]
#<-- [Depth/63 #1750 @63481.48]  (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71)
#... have [Depth/63 #1750 @63481.48]
#... missing Color/66, next expected 63464.801758
#...     waiting for it
#ing-block.cpp:55) --> syncing [Color/66 #1738 @63463.67]
#<-- [Color/66 #1738 @63463.67]  (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71)
#... have [Color/66 #1738 @63463.67]
#... have [Depth/63 #1750 @63481.48]
#  - [Depth/63 #1750 @63481.48] is not in sync; won't be released
#<-- [[Color/66 #1738 @63463.67]]  (CI: (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71))
#ing-block.cpp:20) <-- queueing [[Color/66 #1738 @63463.67]]
#... have [Depth/63 #1750 @63481.48]
#... missing Color/66, next expected 63498.152758
#...     ignoring it
#<-- [[Depth/63 #1750 @63481.48]]  (CI: (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71))
#ing-block.cpp:20) <-- queueing [[Depth/63 #1750 @63481.48]]
#ing-block.cpp:76) --> frame ready: [[Color/66 #1738 @63463.67]]
#ing-block.cpp:76) --> frame ready: [[Depth/63 #1750 @63481.48]]

#############################################################################################
with test.closure( "No sync on D1750 & C1738" ):
    sw.generate_depth_frame( frame_number=1750, timestamp=63481.48 )
    sw.expect_nothing()  # should be waiting for next color
    sw.generate_color_frame( frame_number=1738, timestamp=63463.67, next_expected=63498.152758 )
    sw.expect( color_frame=1738 )
    sw.expect( depth_frame=1750 )


#ing-block.cpp:55) --> syncing [Depth/63 #1751 @63514.82]
#<-- [Depth/63 #1751 @63514.82]  (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71)
#... have [Depth/63 #1751 @63514.82]
#... missing Color/66, next expected 63498.152758
#...     waiting for it
#ing-block.cpp:55) --> syncing [Color/66 #1739 @63497.02]
#<-- [Color/66 #1739 @63497.02]  (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71)
#... have [Color/66 #1739 @63497.02]
#... have [Depth/63 #1751 @63514.82]
#  - [Depth/63 #1751 @63514.82] is not in sync; won't be released
#<-- [[Color/66 #1739 @63497.02]]  (CI: (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71))
#ing-block.cpp:20) <-- queueing [[Color/66 #1739 @63497.02]]
#... have [Depth/63 #1751 @63514.82]
#... missing Color/66, next expected 63531.502758
#...     ignoring it
#<-- [[Depth/63 #1751 @63514.82]]  (CI: (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71))
#ing-block.cpp:20) <-- queueing [[Depth/63 #1751 @63514.82]]
#ing-block.cpp:76) --> frame ready: [[Color/66 #1739 @63497.02]]
#ing-block.cpp:76) --> frame ready: [[Depth/63 #1751 @63514.82]]
#ing-block.cpp:55) --> syncing [Depth/63 #1752 @63548.17]
#<-- [Depth/63 #1752 @63548.17]  (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71)
#... have [Depth/63 #1752 @63548.17]
#... missing Color/66, next expected 63531.502758
#...     waiting for it
#ing-block.cpp:55) --> syncing [Color/66 #1740 @63530.37]
#<-- [Color/66 #1740 @63530.37]  (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71)
#... have [Color/66 #1740 @63530.37]
#... have [Depth/63 #1752 @63548.17]
#  - [Depth/63 #1752 @63548.17] is not in sync; won't be released
#<-- [[Color/66 #1740 @63530.37]]  (CI: (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71))
#ing-block.cpp:20) <-- queueing [[Color/66 #1740 @63530.37]]
#... have [Depth/63 #1752 @63548.17]
#... missing Color/66, next expected 63564.853758
#...     ignoring it
#<-- [[Depth/63 #1752 @63548.17]]  (CI: (TS: Depth/63 Infrared/64 Infrared/65 Color/66 Accel/70 Gyro/71))
#ing-block.cpp:20) <-- queueing [[Depth/63 #1752 @63548.17]]
#ing-block.cpp:76) --> frame ready: [[Color/66 #1740 @63530.37]]
#ing-block.cpp:76) --> frame ready: [[Depth/63 #1752 @63548.17]]






#############################################################################################
test.print_results_and_exit()
