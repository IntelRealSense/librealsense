/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2024 Intel Corporation. All Rights Reserved. */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>


namespace librealsense
{
    namespace fw_logs
    {
        // This class encapsulates
        // 1. The XML file structure
        // 2. The actual XML parsing implementation (currently uses rapidxml)
        class fw_logs_xml_helper
        {
        public:
            // Returns a list of all sources in the definitions file, mapped by id to name.
            static std::map< int, std::string > get_listed_sources( const std::string & definitions_xml );

            // Returns parser options file path of the requested source
            static std::string get_source_parser_file_path( int source_id, const std::string & definitions_xml );

            // Returns a mapping of source module IDs to their requested verbosity level.
            static std::map< int, int > get_source_module_verbosity( int source_id, const std::string & definitions_xml );

            // Returns a mapping of event IDs to their number of arguments and format pairs.
            static std::unordered_map< int, std::pair< int, std::string > > get_events( const std::string & parser_contents );

            // The following functions return a mapping of element IDs to their names
            static std::unordered_map< int, std::string > get_files( const std::string & parser_contents );
            static std::unordered_map< int, std::string > get_modules( const std::string & parser_contents );
            static std::unordered_map< int, std::string > get_threads( const std::string & parser_contents );

            // Returns a mapping of enum names to the enum litterals value and meaning
            static std::unordered_map< std::string, std::vector< std::pair< int, std::string > > >
            get_enums( const std::string & parser_contents );

            // Returns the XML file version.
            static std::string get_file_version( const std::string & xml_file_contents );
        };
    }
}
