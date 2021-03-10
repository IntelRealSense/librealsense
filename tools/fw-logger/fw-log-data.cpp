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

namespace fw_logger
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
}
