/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once
#include <stdint.h>
#include <string>
#include <vector>

namespace fw_logger
{
    struct fw_logs_binary_data
    {
        std::vector<uint8_t> logs_buffer;
    };

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

        uint32_t magic_number;
        uint32_t severity;
        uint32_t file_id;
        uint32_t group_id;
        uint32_t event_id;
        uint32_t line;
        uint32_t sequence;
        uint32_t p1;
        uint32_t p2;
        uint32_t p3;
        uint64_t timestamp;
        double delta;

        std::string message;
        std::string file_name;
        std::string thread_name;

        std::string to_string();
    };
}
