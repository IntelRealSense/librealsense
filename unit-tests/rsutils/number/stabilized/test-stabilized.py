# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
from pyrsutils import stabilized_value


#############################################################################################
#
test.start( "get 60% stability" )
#
# Verify if history is filled with a stable value at the required percentage, the user
# will get it when asked for even if other inputs exist in history
#
try:
    sv = stabilized_value( 10 )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    test.check_equal( sv.get( 0.6 ), 55. );
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "change stabilization percent" )
#
# Verify that on the same values different 'get' calls with different stabilization percent
# give the expected value
#
try:
    sv = stabilized_value( 5 )
    sv.add( 1. )
    sv.add( 1. )
    sv.add( 1. )
    sv.add( 1. )
    sv.add( 1. )

    sv.add( 2. )
    sv.add( 2. )
    sv.add( 2. )
    sv.add( 2. )

    test.check_equal( sv.get( 1.  ), 1. )
    test.check_equal( sv.get( 0.9 ), 1. )
    test.check_equal( sv.get( 0.8 ), 2. )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "empty test" )
#
# Verify that empty function works as expected
#
try:
    sv = stabilized_value( 5 )
    test.check( sv.empty() )
    test.check( not sv )
    sv.add( 1. )
    test.check_false( sv.empty() )
    sv.clear()
    test.check( sv.empty() )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "update stable value" )
#
# Verify a stabilized value once the inserted value is flickering between 2 values
#
try:
    sv = stabilized_value( 10 )
    # Verify flickering value always report the stable value
    for i in range(100):
        sv.add( 55. )
        sv.add( 60. )
        test.check_equal( sv.get( 0.7 ), 55. )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Illegal input - percentage value too high" )
#
# Verify stabilized_value percentage input is at range (0-100] % (zero not included)
#
try:
    sv = stabilized_value( 5 )
    sv.add( 55. )
    test.check_throws( lambda: sv.get( 1.1 ), RuntimeError, "illegal stabilization percentage 1.100000" )
    test.check_throws( lambda: sv.get( -1.1 ), RuntimeError, "illegal stabilization percentage -1.100000" )
    test.check_throws( lambda: sv.get( 0.0 ), RuntimeError, "illegal stabilization percentage 0.000000" )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "not full history" )
#
# Verify if history is not the logic works as expected and the percentage is calculated
# from the history current size
#
try:
    sv = stabilized_value( 30 )
    sv.add( 76. )
    sv.add( 76. )
    sv.add( 76. )
    sv.add( 76. )
    test.check_equal( sv.get( 0.6 ), 76. )

    sv.add( 45. )
    sv.add( 45. )
    sv.add( 45. )
    sv.add( 45. )
    sv.add( 45. )

    test.check_equal( sv.get( 0.6 ), 76. )
    sv.add( 45. )  # The stable value should change now (4 * 76.0 + 6 * 45.0 (total 10 values))
    test.check_equal( sv.get( 0.6 ), 45. );
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "stable value sanity" )
#
# Verify if history is full with a specific value, the stabilized value is always the same
# no matter what percentage is required
#
try:
    sv = stabilized_value( 5 )
    sv.add( 1. )
    sv.add( 1. )
    sv.add( 1. )
    sv.add( 1. )
    sv.add( 1. )

    test.check_equal( sv.get( 1.   ), 1. )
    test.check_equal( sv.get( 0.4  ), 1. )
    test.check_equal( sv.get( 0.25 ), 1. )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "stay with last value" )
#
# Verify if history is filled with less stable percantage than required the last stable
# value is returned
#
try:
    sv = stabilized_value( 10 )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )

    test.check_equal( sv.get( 0.6 ), 55. )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
# Verify if history is filled with a stable value and then filled with required percentage
# of new val, new val is returned as stable value.
#
test.start( "update stable value - nominal" )
#
try:
    sv = stabilized_value( 10 )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 55. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )

    test.check_equal( sv.get( 0.6 ), 60. )
    sv.add( 35. )
    sv.add( 35. )
    sv.add( 35. )
    sv.add( 35. )
    sv.add( 35. )
    test.check_equal( sv.get( 0.6 ), 60. )

    sv.add( 35. )
    test.check_equal( sv.get( 0.6 ), 35. )
except:
    test.unexpected_exception()
test.finish()
#
test.start( "update stable value - last stable not in history" )
try:
    sv = stabilized_value( 10 )
    sv.add( 55. )
    test.check_equal( sv.get( 1. ), 55. );

    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    sv.add( 60. )
    test.check_equal( sv.get( 1.  ), 55. )
    test.check_equal( sv.get( 0.9 ), 60. )
    sv.add( 70. )
    test.check_equal( sv.get( 1.  ), 60. )
except:
    test.unexpected_exception()
test.finish()
#
test.start( "update stable value - last stable is in history" )
try:
    sv = stabilized_value( 10 )
    sv.add( 55. )
    sv.add( 60. )
    sv.add( 60. )
    test.check_equal( sv.get( 0.8 ), 55. )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
test.print_results_and_exit()
