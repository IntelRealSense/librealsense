// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.

#include "fw-log-data.h"

#include <rsutils/string/from.h>

#include <algorithm>

namespace librealsense
{
    namespace fw_logs
    {
        constexpr const size_t min_binary_size = std::min( sizeof( fw_log_binary ), sizeof( extended_fw_log_binary ) );
        constexpr const uint8_t fw_logs_magic_number = 0xA0;
        constexpr const uint8_t extended_fw_logs_magic_number = 0xA5;

        rs2_log_severity fw_logs_binary_data::get_severity() const
        {
            if( logs_buffer.size() < min_binary_size )
                throw librealsense::invalid_value_exception( rsutils::string::from()
                                                             << "FW log data size is too small "
                                                             << logs_buffer.size() );

            const auto log_binary = reinterpret_cast< const fw_log_binary_common * >( logs_buffer.data() );
            if( log_binary->magic_number == extended_fw_logs_magic_number )
                return extended_fw_logs_severity_to_rs2_log_severity( log_binary->severity );
            if( log_binary->magic_number == fw_logs_magic_number )
                return fw_logs_severity_to_rs2_log_severity( log_binary->severity );

            throw librealsense::invalid_value_exception( rsutils::string::from()
                                                         << "Received unfamiliar FW log 'magic number' "
                                                         << log_binary->magic_number );
        }

        uint32_t fw_logs_binary_data::get_timestamp() const
        {
            if( logs_buffer.size() < min_binary_size )
                throw librealsense::invalid_value_exception( rsutils::string::from()
                                                             << "FW log data size is too small "
                                                             << logs_buffer.size() );

            const auto log_binary = reinterpret_cast< const fw_log_binary_common * >( logs_buffer.data() );
            if( log_binary->magic_number == extended_fw_logs_magic_number )
            {
                auto timestamp = reinterpret_cast< const extended_fw_log_binary * >( this )->soc_timestamp;
                return static_cast< uint32_t >( timestamp );
            }
            if( log_binary->magic_number == fw_logs_magic_number )
                return reinterpret_cast< const fw_log_binary * >( this )->timestamp;

            throw librealsense::invalid_value_exception( rsutils::string::from()
                                                         << "Received unfamiliar FW log 'magic number' "
                                                         << log_binary->magic_number );
        }

        rs2_log_severity extended_fw_logs_severity_to_rs2_log_severity( int32_t severity )
        {
            rs2_log_severity result = RS2_LOG_SEVERITY_NONE;

            switch( severity )
            {
            case 1: // Verbose level. Fall through, debug is LibRS most verbose level
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
            case 0: // No logging. Fall through to keep level None set above.
            default:
                break;
            }

            return result;
        }

        rs2_log_severity fw_logs_severity_to_rs2_log_severity(int32_t severity)
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
