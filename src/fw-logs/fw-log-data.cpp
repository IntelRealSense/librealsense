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

        rs2_firmware_log_message fw_log_data::to_rs2_firmware_log_message()
        {
            rs2_firmware_log_message msg;
            msg._magic_number = this->magic_number;
            msg._severity = this->severity;
            msg._file_id = this->file_id;
            msg._group_id = this->group_id;
            msg._event_id = this->event_id;
            msg._line = this->line;
            msg._sequence = this->sequence;
            msg._p1 = this->p1;
            msg._p2 = this->p2;
            msg._p3 = this->p3;
            msg._timestamp = this->timestamp;
            msg._thread_id = this->thread_id;


            //TODO - REMI - check these pointers are deleted in rs2_firmware_log_message destructor
            /*msg._message = new char[this->message.size()];
            strcpy(msg._message, this->message.c_str());

            msg._file_name = new char[this->file_name.size()];
            strcpy(msg._file_name, this->file_name.c_str());

            msg._thread_name = new char[this->thread_name.size()];
            strcpy(msg._thread_name, this->thread_name.c_str());*/

            return msg;
        }
    }
}
