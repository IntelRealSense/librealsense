# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

from rspy import log, test
import pyrealdds as dds


with test.closure( 'default ctor' ):
    test.check_equal( dds.time().seconds, 0 )
    test.check_equal( dds.time().nanosec, 0 )

with test.closure( 'seconds, nanoseconds' ):
    test.check_equal( dds.time( 1, 2 ).seconds, 1 )
    test.check_equal( dds.time( 1, 2 ).nanosec, 2 )

with test.closure( 'limits' ):
    test.check_throws( lambda: dds.time( 0xffffffff, 0 ), TypeError )
    test.check_throws( lambda: dds.time( 0, 0x100000000 ), TypeError )
    test.check_throws( lambda: repr( dds.time( 0, 0xffffffff )), ValueError )  # nsec > 9 digits don't make sense...
    test.check_throws( lambda: repr( dds.time( 0, 1000000000 )), ValueError )
    dds.time( 0, 0xffffffff )  # invalid, but not checked on construction!
    dds.time( 0, 999999999 )  # valid
    # infinite time is 0x7fffffff, 0xffffffff, but is not represented yet
    test.check_throws( lambda: repr( dds.time( 0x7fffffff, 0xffffffff )), ValueError )

with test.closure( 'nanoseconds' ):
    test.check_equal( dds.time().to_ns(), 0 )
    test.check_equal( dds.time( 0 ).to_ns(), 0 )               # nsec
    test.check_equal( dds.time( 1 ).to_ns(), 1 )
    test.check_equal( dds.time( 1, 2 ).to_ns(),  1000000002 )  # sec, nsec

with test.closure( 'double is inexact!' ):
    test.check_equal( dds.time().to_double(), 0. )
    test.check_equal( dds.time( 1.1 ).to_double(), 1.1 )
    test.check_equal( dds.time( 1.1 ).to_ns(),   1100000000 )  # double is ~sec.nsec
    test.check_equal( dds.time( 1.001 ).to_ns(), 1000999999 )  # but inexact!
    test.check_equal( dds.time( 1682486031.2499189 ).to_ns(), 1682486031249918937 )    # !?
    test.check_equal( dds.time( 1682486031.2499189 ).to_double(), 1682486031.249919 )  # !?

with test.closure( 'negatives do not work!' ):
    # NOTE:
    # RTPS, and therefore DDS, time is using the NTP representation, according to RTPS 9.3.2.2.
    # This means it's not really meant to convey negative values: in fact, RTPS defines Time_t
    # as unsigned; it's Duration_t that's signed. Even then, only the seconds can be negative,
    # so -1.1 seconds should really be encoded as -2 seconds, plus 0.9, or -2.9!
    test.check_equal( dds.time( -1, 0 ).to_ns(), -1000000000 )  # -1 seconds is fairly simple
    test.check_equal( repr( dds.time( -1, 0 )), '-1.0' )
    test.check_throws( lambda: dds.time( 0, -1 ), TypeError )   # nanosec is unsigned
    test.check_equal( repr( dds.time( -1 )), '-1.999999999' )   # ?!
    test.check_equal( dds.time( -0, 1 ).to_ns(), 1 )            # doesn't work - this is 1ns!
    test.check_equal( dds.time( -1, 1 ).to_ns(), -999999999 )   # nope
    test.check_equal( dds.time( -1, 999999999 ).to_ns(), -1 )   # yep!
    test.check_equal( repr( dds.time( -1, 999999999 )), '-1.999999999' )  # Oh boy...

with test.closure( 'operators' ):
    test.check_equal( dds.time.from_double( 1.1 ), dds.time( 1.1 ))
    test.check_false( dds.time.from_double( 1.1 ) == 1.1 )
    test.check( dds.time.from_double( 1.1 ) != 1.1 )
    test.check_false( dds.time.from_double( 1.1 ) != dds.time( 1.1 ))
    test.check_throws( lambda: dds.time( 1 ) < dds.time( 2 ), TypeError )    # '<' not supported
    test.check_throws( lambda: dds.time( 2 ) > dds.time( 2 ), TypeError )

with test.closure( 'string repr' ):
    test.check_throws( lambda: dds.time( '0.0' ), TypeError )  # no string parsing
    test.check_equal( str( dds.time() ), '0.0' )               # repr is same as to_double()
    test.check_equal( repr( dds.time() ), '0.0' )
    test.check_equal( str( dds.time( 1.1 )), '1.1' )

test.print_results_and_exit()
