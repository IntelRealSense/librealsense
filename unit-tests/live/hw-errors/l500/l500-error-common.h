// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include <src/l500/l500-private.h>

const uint8_t l500_trigger_error_opcode = 0x5E;

std::map< uint8_t, std::pair< std::string, rs2_log_severity > > build_log_errors_map()
{
    using namespace librealsense::ivcam2;

    std::map< uint8_t, std::pair< std::string, rs2_log_severity > > res;
    l500_notifications_types last_index = success;
    for (auto & i : l500_fw_error_report)
    {
        if (i.first == success)
            continue;  // LRS ignore if he get success as error

        REQUIRE(i.first > last_index);

        if (i.first > last_index + 1)
        {
            for (auto j = last_index + 1; j < i.first; j++)
            {
                std::stringstream s;
                s << "L500 HW report - unresolved type " << j;
                res[j] = { s.str(), RS2_LOG_SEVERITY_WARN };
            }
        }
        res[i.first] = { i.second, RS2_LOG_SEVERITY_ERROR };
        last_index = (l500_notifications_types)i.first;
    }
    return res;
}

void trigger_error_or_exit( const rs2::device & dev, uint8_t num )
{
    std::vector< uint8_t > raw_data( 24, 0 );
    raw_data[0] = 0x14;
    raw_data[2] = 0xab;
    raw_data[3] = 0xcd;
    raw_data[4] = l500_trigger_error_opcode;
    raw_data[12] = num;

    if( auto debug = dev.as< debug_protocol >() )
    {
        try
        {
            debug.send_and_receive_raw_data( raw_data );
        }
        catch(std::exception const& e)
        {
            FAIL(e.what());
        }
    }
}