// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <src/fw-logs/fw-logs-formatting-options.h>
#include <src/fw-logs/fw-logs-xml-helper.h>

#include <src/librealsense-exception.h>
#include <rsutils/string/from.h>

#include <sstream>
#include <stdint.h>

namespace librealsense
{
    namespace fw_logs
    {
        fw_logs_formatting_options::fw_logs_formatting_options( std::string && xml_content )
            : _xml_content( std::move( xml_content ) )
        {
            initialize_from_xml();
        }

        kvp fw_logs_formatting_options::get_event_data( int id ) const
        {
            auto event_it = _fw_logs_event_list.find( id );
            if( event_it != _fw_logs_event_list.end() )
            {
                return event_it->second;
            }

            throw librealsense::invalid_value_exception( rsutils::string::from() << "Unrecognized Log Id:  " << id );
        }

        std::string fw_logs_formatting_options::get_file_name( int id ) const
        {
            auto file_it = _fw_logs_file_names_list.find( id );
            if( file_it != _fw_logs_file_names_list.end() )
                return file_it->second;

            return "Unknown";
        }

        std::string fw_logs_formatting_options::get_thread_name( uint32_t thread_id ) const
        {
            auto file_it = _fw_logs_thread_names_list.find( thread_id );
            if( file_it != _fw_logs_thread_names_list.end() )
                return file_it->second;

            return "Unknown";
        }

        std::string fw_logs_formatting_options::get_module_name( uint32_t module_id ) const
        {
            auto file_it = _fw_logs_module_names_list.find( module_id );
            if( file_it != _fw_logs_module_names_list.end() )
                return file_it->second;

            return "Unknown";
        }

        std::unordered_map< std::string, std::vector< kvp > > fw_logs_formatting_options::get_enums() const
        {
            return _fw_logs_enums_list;
        }

        void fw_logs_formatting_options::initialize_from_xml()
        {
            if( _xml_content.empty() )
                throw librealsense::invalid_value_exception( "Trying to initialize from empty xml content" );

            _fw_logs_event_list = fw_logs_xml_helper::get_events( _xml_content );
            _fw_logs_file_names_list = fw_logs_xml_helper::get_files( _xml_content );
            _fw_logs_thread_names_list = fw_logs_xml_helper::get_threads( _xml_content );
            _fw_logs_module_names_list = fw_logs_xml_helper::get_modules( _xml_content );
            _fw_logs_enums_list = fw_logs_xml_helper::get_enums( _xml_content );
        }
    }  // namespace fw_logs
}  // namespace librealsense
