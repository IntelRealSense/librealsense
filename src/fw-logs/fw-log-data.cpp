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
        fw_log_data::fw_log_data(void)
        {
            magic_number = 0;
            severity = 0;
            file_id = 0;
            group_id = 0;
            event_id = 0;
            line = 0;
            sequence = 0;
            p1 = 0;
            p2 = 0;
            p3 = 0;
            timestamp = 0;
            delta = 0;
            message = "";
            file_name = "";
        }


        fw_log_data::~fw_log_data(void)
        {
        }


        string fw_log_data::to_string()
        {
            stringstream fmt;

            fmt << SET_WIDTH_AND_FILL(6, sequence)
                << SET_WIDTH_AND_FILL(30, file_name)
                << SET_WIDTH_AND_FILL(6, group_id)
                << SET_WIDTH_AND_FILL(15, thread_name)
                << SET_WIDTH_AND_FILL(6, severity)
                << SET_WIDTH_AND_FILL(6, line)
                << SET_WIDTH_AND_FILL(15, timestamp)
                << SET_WIDTH_AND_FILL(15, delta)
                << SET_WIDTH_AND_FILL(30, message);

            auto str = fmt.str();
            return str;
        }

        rs2_log_severity fw_logs_binary_data::get_severity() const
        {
            const fw_log_binary* log_binary = reinterpret_cast<const fw_log_binary*>(logs_buffer.data());
            return fw_logs_severity_to_log_severity(static_cast<int32_t>(log_binary->dword1.bits.severity));
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
