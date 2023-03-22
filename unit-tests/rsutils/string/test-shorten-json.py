# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

from rspy import log, test
from pyrsutils import shorten_json_string as short


# This tests shorten_json_string, but indirectly also json conversion from python objects


#############################################################################################
#
test.start( "shorten_json_string (string)" )
#
# strings are not enforced as valid json objects
test.check_equal( short( '' ), '' )
test.check_equal( short( '{' ), '{' )
test.check_equal( short( '{}' ), '{}' )
test.check_equal( short( '[' ), '[' )
test.check_equal( short( '[]' ), '[]' )
test.check_equal( short( '[{}]' ), '[{}]' )
test.check_equal( short( '{[{}]}' ), '{[{}]}' )  # note: invalid dict/json, but OK as a string!
test.check_equal( short( '[1,2,3]' ), '[1,2,3]' )
test.check_equal( short( '{"some":"one"}' ), '{"some":"one"}' )
#
test.finish()
#
#############################################################################################
#
test.start( "shorten_json_string (string, max-length)" )
#
# max-length must be reasonably long (at least enough characters to encompass '{ ... }' or 7
# characters) or the original string will be returned:
#
s = '{"some":"one"}'
test.check_equal( short( s, 0 ), s )  # invalid
test.check_equal( short( s, 1 ), s )  # invalid
test.check_equal( short( s, 2 ), s )  # invalid
test.check_equal( short( s, 3 ), s )  # invalid
test.check_equal( short( s, 4 ), s )  # invalid
test.check_equal( short( s, 5 ), s )  # invalid
test.check_equal( short( s, 6 ), s )  # invalid
test.check_equal( short( s, 7 ), '{ ... }' )
test.check_equal( short( s, 10 ), '{"so ... }' )
#
s = '[1,2,3,4,5,6,7,8]'
test.check_equal( short( s, 6 ), s )
test.check_equal( short( s, 7 ), '[ ... ]' )
test.check_equal( short( s, 8 ), '[1 ... ]' )
test.check_equal( short( s, 16 ), '[1,2,3,4,5 ... ]' )
test.check_equal( short( s, 17 ), s )
test.check_equal( short( s, 18 ), s )
#
test.finish()
#
#############################################################################################
#
test.start( "shorten_json_string with ellipsis deep inside" )
#
s = '{"a[1]":1,"b[2":3,"d":[1,2,{3,4,5,6,7,8,9},6,7,8]}'  # more complicated
test.check_equal( short( s ), s )
test.check_equal( short( s, len(s) ), s )
test.check_equal( short( s, len(s)-1 ), '{"a[1]":1,"b[2":3,"d":[1,2,{3,4,5,6 ... },6,7,8]}' )
test.check_equal( short( s, len(s)-2 ), '{"a[1]":1,"b[2":3,"d":[1,2,{3,4,5, ... },6,7,8]}' )
test.check_equal( short( s, len(s)-8 ), '{"a[1]":1,"b[2":3,"d":[1,2,{ ... },6,7,8]}' )
test.check_equal( short( s, len(s)-9 ), '{"a[1]":1,"b[2":3,"d":[1,2,{3,4,5, ... ]}' )
#
test.finish()
#
#############################################################################################
#
test.start( "shorten_json_string (None)" )
#
# None translates to json 'null', so is entirely valid
test.check_equal( short( None ), 'null' )
#
test.finish()
#
#############################################################################################
#
test.start( "shorten_json_string (tuples)" )
#
test.check_equal( short( (1,2,'string') ), '[1,2,"string"]' )
test.check_equal( short( (1,2,'string'), 11 ), '[1,2, ... ]' )
test.check_equal( short( '(1,2,"string")' ), '(1,2,"string")' )
#
test.finish()
#
#############################################################################################
#
test.start( "shorten_json_string (json)" )
#
# Just like string input is not enforced as valid json, neither is the output expected to be:
#
test.check_equal( short(
    {'some':'one','two':[1,2, 3, 'dkjsahdkjashdkjhsakjdhada',213123123,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9]} ),
    '{"some":"one","two":[1,2,3,"dkjsahdkjashdkjhsakjdhada",213123123,1,2,3,4,5,6,7,8,9,0,1,2, ... ]}' )
test.check_equal( short(
    {'some':'one','two':"1,2, 3, 'dkjsahdkjashdkjhsakjdhada',213123123,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9"} ),
    '{"some":"one","two":"1,2, 3, \'dkjsahdkjashdkjhsakjdhada\',213123123,1,2,3,4,5,6,7,8,9,0,1,2 ... }' )
#
s = {"a[1]":1,"b[2":3,"d":[1,2,[3,4,5,6,7,8,9],6,7,8]}
ss = '{"a[1]":1,"b[2":3,"d":[1,2,[3,4,5,6,7,8,9],6,7,8]}'
test.check_equal( short( s ), ss )
test.check_equal( short( s, len(ss)-1 ), '{"a[1]":1,"b[2":3,"d":[1,2,[3,4,5,6 ... ],6,7,8]}' )
#
test.finish()
#
#############################################################################################
test.print_results_and_exit()
