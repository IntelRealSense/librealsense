# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

from rspy import log, test
from pyrsutils import split


#############################################################################################
#
test.start( "rsutils::string::split" )

test.check_equal( len( split( "" , '\n' )), 0 )
test.check_equal( len( split( "abc" , '\n' )), 1 )
test.check_equal( len( split( "abc\nabc" , '\n' )), 2 )

test.check_equal_lists( split( "a\nbc\nabc"   , '\n' ), ['a', 'bc', 'abc' ] )
test.check_equal_lists( split( "a\nbc\nabc\n" , '\n' ), ['a', 'bc', 'abc' ] )

test.check_equal_lists( split( "1-12-123-1234" , '-' ), ['1', '12', '123', '1234' ] )

test.finish()
#
#############################################################################################
test.print_results_and_exit()
