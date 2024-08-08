# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

from rspy import log, test
from pyrsutils import version


#############################################################################################
#
with test.closure( "String constructor" ):
    test.check_false( version() )

    test.check_false( version( "" ))
    test.check_false( version( "1" ))
    test.check_false( version( "1." ))
    test.check_false( version( "1.2" ))
    test.check_false( version( "1.2." ))
    test.check  (     version( "1.2.3" ))
    test.check_false( version( "1.2.3." ))
    test.check  (     version( "1.2.3.4" ))
    test.check_false( version( "1.2.3.4." ))
    test.check_false( version( "1 . 2.3.4" ));
    test.check_false( version( ".1.2.3.4" ))
    test.check_false( version( "0.0.0.0" ))
    test.check  (     version( "1.0.0.0" ))
    test.check  (     version( "0.1.0.0" ))
    test.check  (     version( "0.0.1.0" ))
    test.check  (     version( "0.0.0.1" ))
    test.check_false( version( ".2.3.4" ))
    test.check_false( version( "1..2.4" ))
    test.check_false( version( "1.2..4" ))

    test.check  (     version( "65535.2.3.4" ))
    test.check_false( version( "65536.2.3.4" ))
    test.check  (     version( "1.65535.3.4" ))
    test.check_false( version( "1.65536.3.4" ))
    test.check  (     version( "1.2.65535.4" ))
    test.check_false( version( "1.2.65536.4" ))
    test.check  (     version( "1.2.3.65535" ))
    test.check_false( version( "1.2.3.65536" ))

    test.check_false( version( "xxxxxxxxxxx" ))
#
#############################################################################################
#
with test.closure( "Number constructor" ):
    test.check( not version.from_number(0) )
    test.check( version( 1, 2, 3 ))
    test.check( version( 1, 2, 3, 4 ))
    test.check_throws( lambda: version( -1, 2, 3, 4 ), TypeError )
    test.check_throws( lambda: version( 1, -2, 3, 4 ), TypeError )
    test.check_throws( lambda: version( 1, 2, -3, 4 ), TypeError )
    test.check_throws( lambda: version( 1, 2, 3, -4 ), TypeError )
    test.check( version( 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF ))
    test.check_throws( lambda: version( 0x10000, 0xFFFF, 0xFFFF, 0xFFFF ), TypeError )
    test.check_throws( lambda: version( 0xFFFF, 0x10000, 0xFFFF, 0xFFFF ), TypeError )
    test.check_throws( lambda: version( 0xFFFF, 0xFFFF, 0x10000, 0xFFFF ), TypeError )
    test.check_throws( lambda: version( 0xFFFF, 0xFFFF, 0xFFFF, 0x10000 ), TypeError )

    v1234 = version.from_number( version( 1, 2, 3, 1234 ).number )
    test.check_equal( v1234.major(), 1 );
    test.check_equal( v1234.minor(), 2 );
    test.check_equal( v1234.patch(), 3 );
    test.check_equal( v1234.build(), 1234 );
#
#############################################################################################
#
test.start( "Comparisons" )
try:
    v0 = version()
    vN = version.from_number( 281483566843090 )
    v1233 = version( "1.2.3.1233" )
    v1234 = version( "1.2.3.1234" )
    v1235 = version( "1.2.3.1235" )

    test.check_equal( v0, v0 );
    test.check_equal( v0, version() );
    test.check_equal( v0, version('') );
    test.check_equal( v0, version.from_number(0) );
    test.check_equal( v0, version( "0.0.0.0" ) )
    test.check_equal( v0, version( "123" ) )
    test.check      ( vN != v0 );

    test.check_equal( vN, v1234 );
    test.check      ( v1234 == v1234 );
    test.check_false( v1234 != v1234 );
    test.check_false( v1234 == v1235 );
    test.check      ( v1234 != v1235 );

    test.check_equal( version( 1, 2, 3 ), version( "1.2.3" ) )
    test.check( version( "1.2.3.1234" ) != version( "1.2.3" ) )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Leading zeroes" )
try:
    test.check( str(version( "01.02.03.04" )) == "1.2.3.4" );
    test.check(     version( "01.0002.00000000000000003.0000000000000004" ))
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Operator >" )
try:
    test.check( version( "1.2.3.4" ) > version( "0.2.3.4" ))
    test.check( version( "1.2.3.4" ) > version( "1.1.3.4" ))
    test.check( version( "1.2.3.4" ) > version( "1.2.2.4" ))
    test.check( version( "1.2.3.4" ) > version( "1.2.3.3" ))

    test.check_false( version( "1.2.3.4" ) > version( "2.2.3.4" ))
    test.check_false( version( "1.2.3.4" ) > version( "1.3.3.4" ))
    test.check_false( version( "1.2.3.4" ) > version( "1.2.4.4" ))
    test.check_false( version( "1.2.3.4" ) > version( "1.2.3.5" ))
    test.check_false( version( "1.2.3.4" ) > version( "1.2.3.4" ))
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Operator <" )
try:
    test.check( version( "1.2.3.4" ) < version( "2.2.3.4" ))
    test.check( version( "1.2.3.4" ) < version( "1.3.3.4" ))
    test.check( version( "1.2.3.4" ) < version( "1.2.4.4" ))
    test.check( version( "1.2.3.4" ) < version( "1.2.3.5" ))

    test.check_false( version( "1.2.3.4" ) < version( "0.2.3.4" ))
    test.check_false( version( "1.2.3.4" ) < version( "1.1.3.4" ))
    test.check_false( version( "1.2.3.4" ) < version( "1.2.2.4" ))
    test.check_false( version( "1.2.3.4" ) < version( "1.2.3.3" ))

    test.check_false( version( "1.2.3.4" ) < version( "1.2.3.4" ))
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Operator >=" )
try:
    test.check( version( "1.2.3.4" ) >= version( "1.2.3.4" ))

    test.check( version( "2.2.3.4" ) >= version( "1.2.3.4" ))
    test.check( version( "1.3.3.4" ) >= version( "1.2.3.4" ))
    test.check( version( "1.2.4.4" ) >= version( "1.2.3.4" ))
    test.check( version( "1.2.3.5" ) >= version( "1.2.3.4" ))

    test.check_false( version( "1.2.3.4" ) >= version( "2.2.3.4" ))
    test.check_false( version( "1.2.3.4" ) >= version( "1.3.3.4" ))
    test.check_false( version( "1.2.3.4" ) >= version( "1.2.4.4" ))
    test.check_false( version( "1.2.3.4" ) >= version( "1.2.3.5" ))
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Operator <=" )
try:
    test.check( version( "1.2.3.4" ) <= version( "1.2.3.4" ))

    test.check( version( "1.2.3.4" ) <= version( "2.2.3.4" ))
    test.check( version( "1.2.3.4" ) <= version( "1.3.3.4" ))
    test.check( version( "1.2.3.4" ) <= version( "1.2.4.4" ))
    test.check( version( "1.2.3.4" ) <= version( "1.2.3.5" ))

    test.check_false( version( "2.2.3.4" ) <= version( "1.2.3.4" ))
    test.check_false( version( "1.3.3.4" ) <= version( "1.2.3.4" ))
    test.check_false( version( "1.2.4.4" ) <= version( "1.2.3.4" ))
    test.check_false( version( "1.2.3.5" ) <= version( "1.2.3.4" ))
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Is between" )
try:
    v1 = version( "01.02.03.04" )
    v2 = version( "01.03.03.04" )
    v3 = version( "02.01.03.04" )

    test.check( v2.is_between( v1, v3 ))
    test.check( v3.is_between( v1, v3 ))
    test.check( v2.is_between( v2, v3 ))
    test.check( v2.is_between( v2, v2 ))
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Without build" )
try:
    test.check_equal( version(1,2,3,1234).without_build(), version(1,2,3))
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "String conversion" )
try:
    test.check_equal( str(version(1,2,3,4)), "1.2.3.4" );
    test.check_equal( str(version(1,2,3,0)), "1.2.3" );
    test.check_equal( str(version(1,2,0,0)), "1.2.0" );
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
test.print_results_and_exit()



