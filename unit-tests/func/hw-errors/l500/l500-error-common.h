// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <concurrency.h>

using namespace rs2;

const uint8_t l500_trigger_error_opcode = 0x5E;

// these have to match the l500_notifications_types in l500-private.h
const std::map< uint8_t, std::pair< std::string, rs2_log_severity > > l500_error_report = {
    { 1,
      { "Fatal error occur and device is unable \nto run depth stream", RS2_LOG_SEVERITY_ERROR } },
    { 2, { "Overflow occur on infrared stream", RS2_LOG_SEVERITY_ERROR } },
    { 3, { "Overflow occur on depth stream", RS2_LOG_SEVERITY_ERROR } },
    { 4, { "Overflow occur on confidence stream", RS2_LOG_SEVERITY_ERROR } },
    { 5, { "Receiver light saturation, stream stopped for 1 sec", RS2_LOG_SEVERITY_ERROR } },
    { 6,
      { "Error that may be overcome in few sec. \nStream stoped. May be recoverable",
        RS2_LOG_SEVERITY_ERROR } },
    { 7, { "Warning, temperature close to critical", RS2_LOG_SEVERITY_ERROR } },
    { 8, { "Critical temperature reached", RS2_LOG_SEVERITY_ERROR } },
    { 9, { "DFU error", RS2_LOG_SEVERITY_ERROR } },
    { 10, { "L500 HW report - unresolved type 10", RS2_LOG_SEVERITY_WARN } },
    { 11, { "L500 HW report - unresolved type 11", RS2_LOG_SEVERITY_WARN } },
    { 12, { "Fall detected stream stopped", RS2_LOG_SEVERITY_ERROR } },
    { 13, { "L500 HW report - unresolved type 13", RS2_LOG_SEVERITY_WARN } },
    { 14, { "Fatal error occurred (14)", RS2_LOG_SEVERITY_ERROR } },
    { 15, { "Fatal error occurred (15)", RS2_LOG_SEVERITY_ERROR } },
    { 16, { "Fatal error occurred (16)", RS2_LOG_SEVERITY_ERROR } },
    { 17, { "Fatal error occurred (17)", RS2_LOG_SEVERITY_ERROR } },
    { 18, { "Fatal error occurred (18)", RS2_LOG_SEVERITY_ERROR } },
    { 19, { "Fatal error occurred (19)", RS2_LOG_SEVERITY_ERROR } },
    { 20, { "Fatal error occurred (20)", RS2_LOG_SEVERITY_ERROR } },
    { 21, { "Fatal error occurred (21)", RS2_LOG_SEVERITY_ERROR } },
    { 22, { "Fatal error occurred (22)", RS2_LOG_SEVERITY_ERROR } },
    { 23, { "Fatal error occurred (23)", RS2_LOG_SEVERITY_ERROR } },
    { 24, { "Fatal error occurred (24)", RS2_LOG_SEVERITY_ERROR } } };

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
        catch( ... )
        {
            std::cout << "This device doesn't support trigger_error" << std::endl;
            exit( 0 );
        }
    }
}