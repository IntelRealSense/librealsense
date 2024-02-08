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

        rs2_log_severity d400_fw_logs_severity_to_log_severity(int32_t severity);

        static const int BINARY_DATA_SIZE = 20;

        enum class param_type : uint8_t
        {
            STRING = 1,
            UINT8 = 2,
            SINT8 = 3,
            UINT16 = 4,
            SINT16 = 5,
            SINT32 = 6,
            UINT32 = 7,
            SINT64 = 8,
            UINT64 = 9,
            FLOAT = 10,
            DOUBLE = 11
        };

        struct param_info
        {
            uint16_t offset;  // Offset in the params blob, starting from 0
            param_type type;  // Built in type, enumerated
            uint8_t size;
        };

        struct fw_log_binary
        {
            uint32_t magic_number : 8;
            uint32_t severity : 5;
            uint32_t source_id : 3;  // SoC ID in D500, thread ID in D400
            uint32_t file_id : 11;
            uint32_t module_id : 5;
            uint32_t event_id : 16;
            uint32_t line_id : 12;
            uint32_t seq_id : 4;  // Rolling counter 0-15 per module
        };

        struct legacy_fw_log_binary : public fw_log_binary
        {
            uint16_t p1;
            uint16_t p2;
            uint32_t p3;
            uint32_t timestamp;
        };

        struct extended_fw_log_binary : public fw_log_binary
        {
            uint16_t number_of_params;         // Max 32 params
            uint16_t total_params_size_bytes;  // Max 848 bytes
            uint64_t soc_timestamp;
            uint64_t hkr_timestamp;
            param_info * info;  // param_info array size namber_of_params
            // uint8_t * params_blob; // Concatenated blob with params data, zero padded.
        };

        class fw_logs_parser;

        class fw_log_data
        {
            friend class fw_logs_parser;

            rs2_log_severity _severity = RS2_LOG_SEVERITY_NONE;
            uint32_t _line = 0;
            uint32_t _sequence = 0;
            uint64_t _timestamp = 0;

            std::string _message = "";
            std::string _file_name = "";
            std::string _source_name = "";
            std::string _module_name = "";

        public:
            rs2_log_severity get_severity() const;
            uint32_t get_line() const;
            uint32_t get_sequence_id() const;
            uint64_t get_timestamp() const;
            const std::string & get_message() const;
            const std::string & get_file_name() const;
            const std::string & get_source_name() const;
            const std::string & get_module_name() const;
        };
    }
}
