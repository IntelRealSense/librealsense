/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2024 Intel Corporation. All Rights Reserved. */

#pragma once

#include <src/fw-logs/fw-logs-xml-helper.h>

#include <unordered_map>
#include <string>
#include <vector>

#ifdef ANDROID
#include "../../common/android_helpers.h"
#endif

#include <stdint.h>

namespace librealsense
{
    namespace fw_logs
    {
        typedef std::pair< int, std::string > kvp;  // XML key/value pair

        class fw_logs_formatting_options
        {
        public:
            fw_logs_formatting_options() = default;
            fw_logs_formatting_options( std::string && xml_content );

            kvp get_event_data( int id ) const;
            std::string get_file_name( int id ) const;
            std::string get_thread_name( uint32_t thread_id ) const;
            std::string get_module_name( uint32_t module_id ) const;
            std::unordered_map< std::string, std::vector< kvp > > get_enums() const;

        private:
            void initialize_from_xml();

            std::string _xml_content;

            std::unordered_map< int, kvp > _fw_logs_event_list;
            std::unordered_map< int, std::string > _fw_logs_file_names_list;
            std::unordered_map< int, std::string > _fw_logs_thread_names_list;
            std::unordered_map< int, std::string > _fw_logs_module_names_list;
            std::unordered_map< std::string, std::vector< std::pair< int, std::string > > > _fw_logs_enums_list;
        };
    }
}
