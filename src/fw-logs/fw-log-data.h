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

        rs2_log_severity extended_fw_logs_severity_to_rs2_log_severity( int32_t severity );
        rs2_log_severity fw_logs_severity_to_rs2_log_severity( int32_t severity );

#pragma pack( push, 1 )
        struct fw_log_binary_common
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

        struct fw_log_binary : public fw_log_binary_common
        {
            uint16_t p1;
            uint16_t p2;
            uint32_t p3;
            uint32_t timestamp;
        };

        struct extended_fw_log_binary : public fw_log_binary_common
        {
            uint16_t number_of_params;         // Max 32 params
            uint16_t total_params_size_bytes;  // Max 848 bytes, number of bytes after hkr_timestamp field
            uint64_t soc_timestamp;
            uint64_t hkr_timestamp;

            // Also additinal parameters data, but size differs based on number and type of params so not defined here.
        };
#pragma pack( pop )

        struct fw_log_data
        {
            rs2_log_severity severity = RS2_LOG_SEVERITY_NONE;
            uint32_t line = 0;
            uint32_t sequence = 0;
            uint64_t timestamp = 0;

            std::string message = "";
            std::string source_name = "";
            std::string file_name = "";
            std::string module_name = "";
        };

        constexpr const size_t max_sources = 3;
        constexpr const size_t max_modules = 32;

        struct extended_log_request
        {
            uint32_t opcode = 0;
            uint32_t update = 0; // Indication if FW log settings need to be updated. Ignore the following fields if 0.
            uint32_t module_filter[max_sources] = {};  // Bits mapped to source modules. 1 should log, 0 should not.
            uint8_t severity_level[max_sources][max_modules] = {}; // Maps to modules. Zero disables module logging.
        };
    }
}
