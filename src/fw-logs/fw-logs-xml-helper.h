/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2024 Intel Corporation. All Rights Reserved. */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>


namespace librealsense
{
    namespace fw_logs
    {
        class fw_logs_xml_helper
        {
        public:
            std::string get_source_parser_file_path( int source_id, const std::string & definitions_xml ) const;

            // Return a mapping of source module IDs to their requested verbosity level.
            std::unordered_map< int, int > get_source_module_verbosity( int source_id, const std::string & definitions_xml ) const;

            // Return a mapping of event IDs to their number of arguments and format pairs.
            std::unordered_map< int, std::pair< int, std::string > > get_events( const std::string & parser_contents ) const;

            // The following functions return a mapping of element IDs to their names
            std::unordered_map< int, std::string > get_files( const std::string & parser_contents ) const;
            std::unordered_map< int, std::string > get_modules( const std::string & parser_contents ) const;
            std::unordered_map< int, std::string > get_threads( const std::string & parser_contents ) const;

            // Return a mapping of enum names to the enum litterals value and meaning
            std::unordered_map< std::string, std::vector< std::pair< int, std::string > > >
            get_enums( const std::string & parser_contents ) const;
        };
    }
}
