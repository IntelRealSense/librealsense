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
    dds.option.from_json( ['5', None, 'default-string-value', 'desc', ['optional']] )

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

#############################################################################################
test.print_results()
