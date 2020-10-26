/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include "../types.h"       //for rs2_firmware_log_message

namespace librealsense
{
    namespace fw_logs
    {
        struct fw_logs_binary_data
        {
            std::vector<uint8_t> logs_buffer;
            rs2_log_severity get_severity() const;
            uint32_t get_timestamp() const;
        };

        rs2_log_severity fw_logs_severity_to_log_severity(int32_t severity);

        static const int BINARY_DATA_SIZE = 20;

        typedef union
        {
            uint32_t value;
            struct
            {
                uint32_t magic_number : 8;
                uint32_t severity : 5;
                uint32_t thread_id : 3;
                uint32_t file_id : 11;
                uint32_t group_id : 5;
            } bits;
        } fw_log_header_dword1;

        typedef union
        {
            uint32_t value;
            struct
            {
                uint32_t event_id : 16;
                uint32_t line_id : 12;
                uint32_t seq_id : 4;
            } bits;
        } fw_log_header_dword2;

        struct fw_log_header_dword3
        {
            uint16_t p1;
            uint16_t p2;
        };

        struct fw_log_header_dword4
        {
            uint32_t p3;
        };

        struct fw_log_header_dword5
        {
            uint32_t timestamp;
        };

        struct fw_log_binary
        {
            fw_log_header_dword1 dword1;
            fw_log_header_dword2 dword2;
            fw_log_header_dword3 dword3;
            fw_log_header_dword4 dword4;
            fw_log_header_dword5 dword5;
        };


        class fw_log_data
        {
        public:
            fw_log_data(void);
            ~fw_log_data(void);

            uint32_t _magic_number;
            uint32_t _severity;
            uint32_t _file_id;
            uint32_t _group_id;
            uint32_t _event_id;
            uint32_t _line;
            uint32_t _sequence;
            uint32_t _p1;
            uint32_t _p2;
            uint32_t _p3;
            uint64_t _timestamp;
            double _delta;

            uint32_t _thread_id;

            std::string _message;
            std::string _file_name;
            std::string _thread_name;

            rs2_log_severity get_severity() const;
            const std::string& get_message() const;
            const std::string& get_file_name() const;
            const std::string& get_thread_name() const;
            uint32_t get_line() const;
            uint32_t get_timestamp() const;
            uint32_t get_sequence_id() const;
        };
    }
}
