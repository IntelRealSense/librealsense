// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "fw-log-data.h"

#include <rsutils/string/from.h>


namespace librealsense
{
    namespace fw_logs
    {
        rs2_log_severity fw_logs_binary_data::get_severity() const
        {
            const fw_log_binary * log_binary = reinterpret_cast< const fw_log_binary * >( logs_buffer.data() );
            if( log_binary->magic_number == 0xA5)
                return fw_logs_severity_to_rs2_log_severity( log_binary->severity );
            if( log_binary->magic_number == 0xA0 )
                return legacy_fw_logs_severity_to_rs2_log_severity( log_binary->severity );

            throw librealsense::invalid_value_exception( rsutils::string::from()
                                                         << "Received unfamiliar FW log 'magic number' "
                                                         << log_binary->magic_number );
        }

        uint32_t fw_logs_binary_data::get_timestamp() const
        {
            const legacy_fw_log_binary * log_binary = reinterpret_cast< const legacy_fw_log_binary * >( logs_buffer.data() );
            return log_binary->timestamp;
        }

        rs2_log_severity fw_logs_severity_to_rs2_log_severity( int32_t severity )
        {
            rs2_log_severity result = RS2_LOG_SEVERITY_NONE;
            switch( severity )
            {
            case 1:
                // Verbose level. Fall through, debug is LibRS most verbose level
            case 2:
                result = RS2_LOG_SEVERITY_DEBUG;
                break;
            case 4:
                result = RS2_LOG_SEVERITY_INFO;
                break;
            case 8:
                result = RS2_LOG_SEVERITY_WARN;
                break;
            case 16:
                result = RS2_LOG_SEVERITY_ERROR;
                break;
            case 32:
                result = RS2_LOG_SEVERITY_FATAL;
                break;
            default:
                break;
            }
            return result;
        }

        rs2_log_severity legacy_fw_logs_severity_to_rs2_log_severity(int32_t severity)
        {
            rs2_log_severity result = RS2_LOG_SEVERITY_NONE;
            switch (severity)
            {
            case 1:
                result = RS2_LOG_SEVERITY_DEBUG;
                break;
            case 2:
                result = RS2_LOG_SEVERITY_WARN;
                break;
            case 3:
                result = RS2_LOG_SEVERITY_ERROR;
                break;
            case 4:
                result = RS2_LOG_SEVERITY_FATAL;
                break;
            default:
                break;
            }
            return result;
        }
    } // namespace fw_logs
} // namespace librealsense
