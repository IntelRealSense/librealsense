# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as dds
from rspy import log, test

# Test dds.option mechanism and logic

with test.closure( 'read-only options' ):
    test.check( dds.option.from_json( ['1', 0, 'desc'] ).is_read_only() )
    test.check_false( dds.option.from_json( ['1', 0, 0, 'desc'] ).is_read_only() )  # default value -> not read-only
    test.check( dds.option.from_json(
        ['Asic Temperature',None,-40.0,125.0,0.0,0.0,'Current Asic Temperature (degree celsius)',['optional','read-only']] ).is_read_only() )
    test.check_equal_lists( dds.option.from_json(  # default==min==max, step==0 -> read-only!
        ['Stereo Baseline',50.20994567871094,50.20994567871094,50.20994567871094,0.0,50.20994567871094,'...',['read-only']] ).to_json(),
        ['Stereo Baseline',50.20994567871094,'...'] )
    test.check_equal(  # 1.1 => 1.10000002 as float, => 1.1000000000000001 as double
        dds.option.from_json( ['a', 1.1, 'desc'] ).get_value(),
        1.1 )

with test.closure( 'boolean' ):
    test.check_equal( dds.option.from_json( ['b', True, 'bool'] ).value_type(), 'boolean' )
    test.check_equal( dds.option.from_json( ['b', False, 'bool'] ).value_type(), 'boolean' )
    test.check_equal( dds.option.from_json( ['b', 0, 'bool', ['boolean']] ).value_type(), 'boolean' )
    test.check_equal( dds.option.from_json( ['b', 0, 'bool', ['boolean']] ).get_value(), False )
    test.check_equal( dds.option.from_json( ['b', 1, 'bool', ['boolean']] ).get_value(), True )
    test.check_equal_lists(
        dds.option.from_json( ['b', 1, 'bool', ['boolean']] ).to_json(),
        ['b', True, 'bool'] )
    test.check_throws( lambda:
        dds.option.from_json( ['b', 1., 'bool', ['boolean']] ),
        RuntimeError, 'not convertible to a boolean: 1.0' )
    test.check_throws( lambda:
        dds.option.from_json( ['b', 2, 'bool', ['boolean']] ),
        RuntimeError, 'not convertible to a boolean: 2' )
    test.check_equal( dds.option.from_json( ['b', False, True, 'bool'] ).value_type(), 'boolean' )
    test.check_equal( dds.option.from_json( ['b', False, None, 'bool', ['optional']] ).value_type(), 'boolean' )

with test.closure( 'enum' ):
    test.check_equal( dds.option.from_json( ['e1', 'a', ['a','b','c'], 'c', 'enum'] ).value_type(), 'enum' )
    test.check_equal( dds.option.from_json( ['e1', 'a', ['a','a','c'], 'c', 'enum'] ).value_type(), 'enum' )
    test.check_throws( lambda:
        dds.option.from_json( ['e1', 'd', [], 'c', 'enum'] ),
        RuntimeError, 'invalid enum value: "c"' )
    test.check_throws( lambda:
        dds.option.from_json( ['e1', 'a', None, 'c', 'enum'] ),
        RuntimeError, 'enum option requires a choices array' )
    test.check_throws( lambda:
        dds.option.from_json( ['e1', 'd', ['a','b','c'], 'c', 'enum'] ),
        RuntimeError, 'invalid enum value: "d"' )
    test.check_throws( lambda:
        dds.option.from_json( ['e1', 'a', ['a','b','c'], 'd', 'enum'] ),
        RuntimeError, 'invalid enum value: "d"' )
    test.check_throws( lambda:
        dds.option.from_json( ['e1', 'a', [None,'b','c'], 'd', 'enum'] ),
        RuntimeError, 'enum choices must be strings' )
    test.check_throws( lambda:
        dds.option.from_json( ['e1', 1, [1,2,3], 3, 'enum'] ),
        RuntimeError, 'non-string enum values' )
    test.check_throws( lambda:
        dds.option.from_json( ['e1', None, ['a','b','c'], 'c', 'enum'] ),
        RuntimeError, 'value is not optional' )
    test.check_equal( dds.option.from_json( ['e1', None, ['a','b','c'], 'c', 'enum', ['optional']] ).value_type(), 'enum' )
    test.check_equal_lists(
        dds.option.from_json( ['e1', None, ['a','b','c'], 'c', 'enum', ['optional']] ).to_json(),
        ['e1', None, ['a','b','c'], 'c', 'enum', ['optional']] )

with test.closure( 'r/o options are still settable' ):
    # NOTE: the DDS options do not enforce logic post initialization; they serve only to COMMUNICATE any state and limits
    test.check_equal( dds.option.from_json( ['1', 0, 'desc'] ).value_type(), 'int' )
    dds.option.from_json( ['1', 0, 'desc'] ).set_value( 20. )  # OK because 20.0 can be expressed as int
    test.check_throws( lambda:
        dds.option.from_json( ['1', 0, 'desc'] ).set_value( 20.5 ),
        RuntimeError, 'not convertible to a signed integer: 20.5' )

with test.closure( 'optional (default) value' ):
    test.check_false( dds.option.from_json( ['1', 0, 'desc'] ).is_optional() )
    test.check( dds.option.from_json( ['Asic Temperature',None,-40.0,125.0,0.0,0.0,'Current Asic Temperature (degree celsius)',['optional','read-only']] ).is_optional() )
    dds.option.from_json( ['4', 'string-value', None, 'desc', ['optional']] )
    dds.option.from_json( ['5', None, 'default-string-value', 'desc', ['optional']] )  # string type is deduced
    test.check_throws( lambda:
        dds.option.from_json( ['a', None, 'desc', ['optional']] ),
        RuntimeError, 'cannot deduce value type: ["a",null,"desc",["optional"]]' )
    test.check_equal_lists(
        dds.option.from_json( ['Integer Option', None, None, 'Something', ['optional', 'int']] ).to_json(),
        ['Integer Option', None, None, 'Something', ['optional', 'int']] )

with test.closure( 'mixed types' ):
    test.check_equal( dds.option.from_json( ['i', 0, 'desc'] ).value_type(), 'int' )
    test.check_equal( dds.option.from_json( ['f', 0, 'desc', ['float']] ).value_type(), 'float' )
    test.check_equal( dds.option.from_json( ['1', 0., 0, 1, 1, 0, 'desc'] ).value_type(), 'float' )
    test.check_equal( dds.option.from_json( ['2', 0, 0., 1, 1, 0, 'desc'] ).value_type(), 'float' )
    test.check_equal( dds.option.from_json( ['3', 0, 0, 1., 1, 0, 'desc'] ).value_type(), 'float' )
    test.check_equal( dds.option.from_json( ['4', 0, 0, 1, 1., 0, 'desc'] ).value_type(), 'float' )
    test.check_equal( dds.option.from_json( ['5', 0, 0, 1, 1, 0., 'desc'] ).value_type(), 'float' )
    test.check_equal( dds.option.from_json( ['-1', 0, -1, 4, 1, 0, 'desc'] ).value_type(), 'int' )
    test.check_equal(           # float, but forced to an int
        dds.option.from_json( ['3f', 3, 1, 5, 1, 2., '', ['int']] ).value_type(), 'int' )
    test.check_throws( lambda:  # same, but with an actual float
        dds.option.from_json( ['3f', 3, 1, 5, 1, 2.2, '', ['int']] ),
        RuntimeError, 'not convertible to a signed integer: 2.2' )

    dds.option.from_json( ['a', 9223372036854775807, 'desc'] )       # max int64
    test.check_throws( lambda:
        dds.option.from_json( ['a', 9223372036854775808, 'desc'] ),  # uint64
        RuntimeError, 'not convertible to a signed integer: 9223372036854775808' )
    dds.option.from_json( ['a', -9223372036854775808, 'desc'] )    # min int64
    # -9223372036854775809 cannot be converted to json

    test.check_equal( dds.option.from_json( ['Brightness',0,-64,64,1,0,'UVC image brightness'] ).value_type(), 'int' )

with test.closure( 'range' ):
    test.check_throws( lambda:
        dds.option.from_json( ['3x', 0, 0, -1, 1, 0, ''] ),
        RuntimeError, 'default value 0 > -1 maximum' )
    test.check_throws( lambda:
        dds.option.from_json( ['3x', 0, 2, 3, 1, 0, ''] ),
        RuntimeError, 'default value 0 < 2 minimum' )
    test.check_throws( lambda:
        dds.option.from_json( ['3y', -1, -2, 2, 1, 3, 'bad default'] ),
        RuntimeError, 'default value 3 > 2 maximum' )
    dds.option.from_json( ['3', -1, -2, 2, 1, 0, ''] )
    dds.option.from_json( ['Backlight Compensation', 0, 0, 1, 1, 0, 'Backlight custom description'] )
    dds.option.from_json( ['Custom Option', 5., -10, 10, 1, -5., 'Description'] )

with test.closure( 'IP address' ):
    dds.option.from_json( ['ip', None, 'desc', ['IPv4', 'optional']] )
    test.check_throws( lambda:
        dds.option.from_json( ['ip', '', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: ""' )
    test.check_throws( lambda:  # 0.0.0.0 is used to denote an "invalid" IP
        dds.option.from_json( ['ip', '0.0.0.0', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: "0.0.0.0"' )
    dds.option.from_json( ['ip', '0.0.0.1', 'desc', ['IPv4']] )
    dds.option.from_json( ['ip', '255.255.255.255', 'desc', ['IPv4']] )
    test.check_throws( lambda:
        dds.option.from_json( ['ip', '255.255.255.256', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: "255.255.255.256"' )
    test.check_throws( lambda:
        dds.option.from_json( ['ip', '255.255.256.255', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: "255.255.256.255"' )
    test.check_throws( lambda:
        dds.option.from_json( ['ip', '255.256.255.255', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: "255.256.255.255"' )
    test.check_throws( lambda:
        dds.option.from_json( ['ip', '256.255.255.255', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: "256.255.255.255"' )
    test.check_throws( lambda:
        dds.option.from_json( ['ip', '1.2.3.4a', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: "1.2.3.4a"' )
    test.check_throws( lambda:
        dds.option.from_json( ['ip', '1.2.3.4.', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: "1.2.3.4."' )
    test.check_throws( lambda:
        dds.option.from_json( ['ip', '1.2..4', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: "1.2..4"' )
    test.check_throws( lambda:
        dds.option.from_json( ['ip', '1.2.3.', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: "1.2.3."' )
    test.check_throws( lambda:
        dds.option.from_json( ['ip', '1.2.3', 'desc', ['IPv4']] ),
        RuntimeError, 'not an IP address: "1.2.3"' )

with test.closure( 'rect' ):
    test.check_equal( dds.option.from_json( ['r', [0,0,1,1], 'r/o'] ).value_type(), 'rect' )
    test.check_equal( dds.option.from_json( ['r', None, 'r/o', ['rect','optional']] ).value_type(), 'rect' )
    test.check_equal( dds.option.from_json( ['r', [0,1,2,3], 'r/o'] ).get_value(), [0,1,2,3] )
    test.check_equal( dds.option.from_json( ['r', [0,1,2,3], [1,2,3,4], 'r/w'] ).value_type(), 'rect' )
    test.check_equal( dds.option.from_json( ['r', [0,1,2,3], [1,2,3,4], 'r/w'] ).get_default_value(), [1,2,3,4] )
    test.check_throws( lambda:  # non-integer inside the default value
        dds.option.from_json( ['r', [0,1,2,3], [1,2,3.,4], 'r/w'] ),
        RuntimeError, 'cannot deduce value type: ["r",[0,1,2,3],[1,2,3.0,4],"r/w"]' )
    test.check_throws( lambda:  # no range for rect
        dds.option.from_json( ['r', [0,1,2,3], 0, 2, 1, [1,2,3,4], 'r/w'] ),
        RuntimeError, 'not [x1,y1,x2,y2]: 0' )
    test.check_throws( lambda:  # with 5 args, it looks like an enum
        dds.option.from_json( ['r', [0,1,2,3], [1,2,3,4], [1,2,3,4], 'r/w'] ),
        RuntimeError, 'non-string enum values' )
    # With a range supplied, operator<() for JSON takes effect
    test.check_equal( dds.option.from_json( ['r', [1,2,3,4], [0,1,2,3], [2,3,4,5], [3,4,5,6], [2,3,4,5], 'r/w'] ).value_type(), 'rect' )
    test.check_throws( lambda:
        dds.option.from_json( ['r', [1,2,3,4], [0,1,2,3.], [2,3,4,5], [3,4,5,6], [2,3,4,5], 'r/w'] ),
        RuntimeError, 'non-integers found: [0,1,2,3.0]' )

#############################################################################################
test.print_results()
