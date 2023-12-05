# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

from rspy import log, test
import pyrealdds as dds


with test.closure( 'default ctor' ):
    test.check_false( dds.guid() )

with test.closure( 'from string: eProsima format' ):
    test.check_equal( repr( dds.guid.from_string( '112233445566.5.100' )), '112233445566.5.100' )
    test.check_false( dds.guid.from_string( '1122334455.5.100' ) )
    test.check_false( dds.guid.from_string( '00112233445566778899001122.5.100' ) )
    test.check_equal( repr( dds.guid.from_string( 'aabbccddeeff.5.100' ) ), 'aabbccddeeff.5.100' )  # hex digits
    test.check_equal( repr( dds.guid.from_string( '112233445566.b.100' ) ), '112233445566.b.100' )
    test.check_equal( repr( dds.guid.from_string( '112233445566.5.10a' ) ), '112233445566.5.10a' )
    test.check_equal( repr( dds.guid.from_string( 'aaBBccDDeeFF.5.100' ) ), 'aabbccddeeff.5.100' )  # uppercase OK, but output is lowercase
    test.check_equal( repr( dds.guid.from_string( '112233445566.CDE.100' ) ), '112233445566.cde.100' )
    test.check_equal( repr( dds.guid.from_string( '112233445566.5.1A0' ) ), '112233445566.5.1a0' )
    test.check_false( dds.guid.from_string( '112233g45566.5.100' ) )  # invalid
    test.check_false( dds.guid.from_string( '112233445566.U.100' ) )
    test.check_false( dds.guid.from_string( '112233445566.5.1X0' ) )
    test.check_false( dds.guid.from_string( '112233445566.-1.100' ) )
    test.check_false( dds.guid.from_string( '112233445566.0.100-' ) )

with test.closure( 'from string: generic format' ):
    test.check_false( dds.guid.from_string( '' ) )
    test.check_equal( repr( dds.guid.from_string( '001122334455667788990011.100' ) ), '001122334455667788990011.100' )
    test.check_equal( repr( dds.guid.from_string( '010f22334455667788990000.100' ) ), '223344556677.9988.100' )  # eProsima vendor ID



test.print_results_and_exit()
