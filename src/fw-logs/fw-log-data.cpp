// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "fw-log-data.h"
#include <sstream>
#include <iomanip>
#include <locale>
#include <string>

using namespace std;

# define SET_WIDTH_AND_FILL(num, element) \
setfill(' ') << setw(num) << left << element \

namespace librealsense
{
    namespace fw_logs
    {
        rs2_log_severity fw_log_data::get_severity() const
        {
            return fw_logs_severity_to_log_severity(_severity);
        }

        const std::string& fw_log_data::get_message() const
        {
            return _message;
        }

        const std::string& fw_log_data::get_file_name() const
        {
            return _file_name;
        }

        const std::string& fw_log_data::get_thread_name() const
        {
            return _thread_name;
        }

        uint32_t fw_log_data::get_line() const
        {
            return _line;
        }

        uint32_t fw_log_data::get_timestamp() const
        {
            return (uint32_t)_timestamp;
        }

        uint32_t fw_log_data::get_sequence_id() const
        {
            return _sequence;
        }

        rs2_log_severity fw_logs_binary_data::get_severity() const
        {
            const fw_log_binary* log_binary = reinterpret_cast<const fw_log_binary*>(logs_buffer.data());
            return fw_logs_severity_to_log_severity(static_cast<int32_t>(log_binary->dword1.bits.severity));
        }

        uint32_t fw_logs_binary_data::get_timestamp() const
        {
            const fw_log_binary* log_binary = reinterpret_cast<const fw_log_binary*>(logs_buffer.data());
            return static_cast<uint32_t>(log_binary->dword5.timestamp);
        }

        rs2_log_severity fw_logs_severity_to_log_severity(int32_t severity)
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
    }
}
