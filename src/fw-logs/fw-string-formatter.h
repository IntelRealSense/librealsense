/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2024 Intel Corporation. All Rights Reserved. */

#pragma once

#include <src/fw-logs/fw-log-data.h>

#include <string>
#include <map>
#include <stdint.h>
#include <vector>
#include <unordered_map>

namespace librealsense
{
    namespace fw_logs
    {
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
            uint16_t offset;  // Offset in the params blob. Note - original raw message offset is from start of message.
            param_type type;  // Built in type, enumerated
            uint8_t size;
        };
        
        class fw_string_formatter
        {
        public:
            fw_string_formatter( std::unordered_map< std::string, std::vector< std::pair< int, std::string > > > enums );

            std::string generate_message( const std::string & source,
                                          const std::vector< param_info > & params_info,
                                          const std::vector< uint8_t > & params_blob );

        private:
            std::string replace_params( const std::string & source,
                                        const std::map< std::string, std::string > & exp_replace_map,
                                        const std::map< std::string, int > & enum_replace_map );
            std::string convert_param_to_string( const param_info & info, const uint8_t * param_start ) const;
            bool is_integral( const param_info & info ) const;

            std::unordered_map<std::string, std::vector<std::pair<int, std::string>>> _enums;
        };
    }
}
