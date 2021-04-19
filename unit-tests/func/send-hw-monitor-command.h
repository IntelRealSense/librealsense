// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../test.h"
#include <librealsense2/rs.hpp>
#include <hw-monitor.h>

#pragma once
using namespace rs2;

struct hw_monitor_command
{
    hw_monitor_command(
        unsigned int in_cmd, int in_p1 = 0, int in_p2 = 0, int in_p3 = 0, int in_p4 = 0 )
        : cmd( in_cmd )
        , p1( in_p1 )
        , p2( in_p2 )
        , p3( in_p3 )
        , p4( in_p4 )
    {
    }

    unsigned int cmd;
    int p1;
    int p2;
    int p3;
    int p4;
};

inline std::vector< uint8_t > send_command_and_check( rs2::debug_protocol dp,
                                               hw_monitor_command command,
                                               uint32_t expected_size_return = 0 )
{
    const int MAX_HW_MONITOR_BUFFER_SIZE = 1024;

    std::vector< uint8_t > res( MAX_HW_MONITOR_BUFFER_SIZE, 0 );
    int size = 0;

    librealsense::hw_monitor::fill_usb_buffer( command.cmd,
                                               command.p1,
                                               command.p2,
                                               command.p3,
                                               command.p4,
                                               nullptr,
                                               0,
                                               res.data(),
                                               size );

    res = dp.send_and_receive_raw_data( res );
    REQUIRE( res.size() == sizeof( unsigned int ) * ( expected_size_return + 1 ) );  // opcode

    auto vals = reinterpret_cast< int32_t * >( (void *)res.data() );
    REQUIRE( vals[0] == command.cmd );
    res.erase( res.begin(), res.begin() + sizeof( unsigned int ) );  // remove opcode

    return res;
}
