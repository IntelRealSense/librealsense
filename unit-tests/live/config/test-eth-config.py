# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 RealSense Inc. All Rights Reserved.

# Currently only D555 supports DDS configuration natively
# test:device each(D555)

import pyrealsense2 as rs
import pyrsutils as rsutils
from rspy import test


get_eth_config_opcode = 0xBB
set_eth_config_opcode = 0xBA

default_values_param = 0
current_values_param = 1


dev, ctx = test.find_first_device_or_exit()

def get_eth_config( get_default_config = False ):
    raw_command = rs.debug_protocol( dev ).build_command( get_eth_config_opcode, default_values_param if get_default_config else current_values_param )
    raw_result = rs.debug_protocol( dev ).send_and_receive_raw_data( raw_command )
    test.check( raw_result[0] == get_eth_config_opcode )
    return rsutils.eth_config( raw_result[4:] )
 
def set_eth_config( config ):
    raw_command = rs.debug_protocol( dev ).build_command( set_eth_config_opcode, 0, 0, 0, 0, config.build_command() )
    raw_result = rs.debug_protocol( dev ).send_and_receive_raw_data( raw_command )
    test.check( raw_result[0] == set_eth_config_opcode )


with test.closure("Test DDS support"):
    orig_config = get_eth_config()
    new_config = get_eth_config() # Get a new config object to keep orig_config intact

with test.closure("Test link timeout configuration"):
    new_config.link.timeout *= 2
    set_eth_config( new_config )
    updated_config = get_eth_config()
    test.check( updated_config.link.timeout == orig_config.link.timeout * 2 )

with test.closure("Test MTU configuration"):
    new_config.link.mtu = 4000
    if new_config.header.version == 3:
        try:
            set_eth_config( new_config )
        except ValueError as e:
            test.check_exception( e, ValueError, "Camera FW supports only MTU 9000." )
        else:
            test.unreachable()
    else:
        set_eth_config( new_config )
        updated_config = get_eth_config()
        test.check( updated_config.link.mtu == 4000 )
        
        new_config.link.mtu = 0
        try:
            set_eth_config( new_config )
        except ValueError as e:
            test.check_exception( e, ValueError, "MTU size should be 500-9000. Current 0" )
        else:
            test.unreachable()

        new_config.link.mtu = 1234
        try:
            set_eth_config( new_config )
        except ValueError as e:
            test.check_exception( e, ValueError, "MTU size must be divisible by 500. Current 1234" )
        else:
            test.unreachable()
    new_config.link.mtu = orig_config.link.mtu # Restore field that might fail other tests, depending header version.


with test.closure("Test transmission delay configuration"):
    new_config.transmission_delay = 21
    if new_config.header.version == 3:
        try:
            set_eth_config( new_config )
        except ValueError as e:
            test.check_exception( e, ValueError, "Camera FW does not support transmission delay." )
        else:
            test.unreachable()
    else:
        set_eth_config( new_config )
        updated_config = get_eth_config()
        test.check( updated_config.transmission_delay == 21 )

        new_config.transmission_delay = 222
        try:
            set_eth_config( new_config )
        except ValueError as e:
            test.check_exception( e, ValueError, "Transmission delay should be 0-144. Current 222" )
        else:
            test.unreachable()
        
        new_config.transmission_delay = 100
        try:
            set_eth_config( new_config )
        except ValueError as e:
            test.check_exception( e, ValueError, "Transmission delay must be divisible by 3. Current 100" )
        else:
            test.unreachable()
    new_config.transmission_delay = orig_config.transmission_delay # Restore field that might fail other tests, depending header version.

with test.closure("Test configuration failures"): # Failures depending on version tested separately
    new_config.header.version = 2
    try:
        set_eth_config( new_config )
    except ValueError as e:
        test.check_exception( e, ValueError, "Unrecognized Eth config table version 2" )
    else:
        test.unreachable()
    new_config.header.version = orig_config.header.version
    
    new_config.dhcp.timeout = -1
    try:
        set_eth_config( new_config )
    except ValueError as e:
        test.check_exception( e, ValueError, "DHCP timeout cannot be negative. Current -1" )
    else:
        test.unreachable()
    new_config.dhcp.timeout = orig_config.dhcp.timeout
    
    # Don't set valid domain_id, it might cause DDS devices to loose connection.
    new_config.dds.domain_id = -1
    try:
        set_eth_config( new_config )
    except ValueError as e:
        test.check_exception( e, ValueError, "Domain ID should be in 0-232 range. Current -1" )
    else:
        test.unreachable()
    new_config.dds.domain_id = 233
    try:
        set_eth_config( new_config )
    except ValueError as e:
        test.check_exception( e, ValueError, "Domain ID should be in 0-232 range. Current 233" )
    else:
        test.unreachable()
    new_config.dds.domain_id = orig_config.dds.domain_id

with test.closure("Restore configuration"):
    set_eth_config( orig_config )        
    
test.print_results_and_exit()
