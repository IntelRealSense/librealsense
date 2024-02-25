// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <src/fw-logs/fw-logs-parser.h>
#include <src/fw-logs/fw-string-formatter.h>
#include <src/fw-logs/fw-logs-xml-helper.h>

#include <rsutils/string/from.h>

#include <regex>
#include <fstream>
#include <iterator>


namespace librealsense
{
    namespace fw_logs
    {
        fw_logs_parser::fw_logs_parser( const std::string & definitions_xml )
            : _source_to_formatting_options()
        {
            // The definitions XML should contain entries for all log sources.
            // For each source it lists parser options file path and (optional) module verbosity level.
            fw_logs_xml_helper helper;
            _source_id_to_name = helper.get_listed_sources( definitions_xml );
            for( const auto & source : _source_id_to_name )
            {
                std::string path = helper.get_source_parser_file_path( source.first, definitions_xml );
                std::ifstream f( path.c_str() );
                if( f.good() )
                {
                    std::string xml_contents;
                    xml_contents.append( std::istreambuf_iterator< char >( f ), std::istreambuf_iterator< char >() );
                    fw_logs_formatting_options format_options( std::move( xml_contents ) );
                    _source_to_formatting_options[source.first] = format_options;
                }
                else
                    throw librealsense::invalid_value_exception( rsutils::string::from() << "Can't open file " << path );
            }
        }

        fw_log_data fw_logs_parser::parse_fw_log( const fw_logs_binary_data * fw_log_msg ) 
        {
            fw_log_data parsed_data = fw_log_data();

            if( ! fw_log_msg || fw_log_msg->logs_buffer.size() == 0 )
                return parsed_data;

            // Struct data in a common format
            structured_binary_data structured = structure_common_data( fw_log_msg );
            structure_timestamp( fw_log_msg, &structured );

            const fw_logs_formatting_options & formatting = get_format_options_ref( structured.source_id );

            kvp event_data = formatting.get_event_data( structured.event_id );
            structure_params( fw_log_msg, event_data.first, &structured );

            // Parse data
            fw_string_formatter reg_exp( formatting.get_enums() );
            parsed_data.message = reg_exp.generate_message( event_data.second,
                                                            structured.params_info,
                                                            structured.params_blob );
            
            parsed_data.line = structured.line;
            parsed_data.sequence = structured.sequence;
            parsed_data.timestamp = structured.timestamp;
            parsed_data.severity = parse_severity( structured.severity );
            parsed_data.source_name = get_source_name( structured.source_id );
            parsed_data.file_name = formatting.get_file_name( structured.file_id );
            parsed_data.module_name = formatting.get_module_name( structured.module_id );

            return parsed_data;
        }

        size_t fw_logs_parser::get_log_size( const uint8_t * log ) const
        {
            const auto extended = reinterpret_cast< const extended_fw_log_binary * >( log );
            // If there are parameters total_params_size_bytes includes the information
            return sizeof( extended_fw_log_binary ) - sizeof( extended->info ) + extended->total_params_size_bytes;
        }

        fw_logs_parser::structured_binary_data
        fw_logs_parser::structure_common_data( const fw_logs_binary_data * fw_log_msg ) const
        {
            structured_binary_data structured_data;

            auto * log_binary = reinterpret_cast< const fw_log_binary * >( fw_log_msg->logs_buffer.data() );

            structured_data.severity = static_cast< uint32_t >( log_binary->severity );
            structured_data.source_id = static_cast< uint32_t >( log_binary->source_id );
            structured_data.file_id = static_cast< uint32_t >( log_binary->file_id );
            structured_data.module_id = static_cast< uint32_t >( log_binary->module_id );
            structured_data.event_id = static_cast< uint32_t >( log_binary->event_id );
            structured_data.line = static_cast< uint32_t >( log_binary->line_id );
            structured_data.sequence = static_cast< uint32_t >( log_binary->seq_id );

            return structured_data;
        }

        void fw_logs_parser::structure_timestamp( const fw_logs_binary_data * raw,
                                                  structured_binary_data * structured ) const
        {
            const auto actual_struct = reinterpret_cast< const extended_fw_log_binary * >( raw->logs_buffer.data() );
            structured->timestamp = actual_struct->soc_timestamp;
        }

        void fw_logs_parser::structure_params( const fw_logs_binary_data * raw,
                                               size_t num_of_params,
                                               structured_binary_data * structured ) const
        {
            const auto actual_struct = reinterpret_cast< const extended_fw_log_binary * >( raw->logs_buffer.data() );
            if( num_of_params != actual_struct->number_of_params )
                throw librealsense::invalid_value_exception( rsutils::string::from() << "Expecting " << num_of_params
                                                             << " parameters, received " << actual_struct->number_of_params );

            const uint8_t * blob_start = reinterpret_cast< const uint8_t * >( actual_struct->info + actual_struct->number_of_params );
            structured->params_info.insert( structured->params_info.end(),
                                            actual_struct->info,
                                            actual_struct->info + actual_struct->number_of_params );
            structured->params_blob.insert( structured->params_blob.end(),
                                            blob_start,
                                            blob_start + actual_struct->total_params_size_bytes );
        }

        const fw_logs_formatting_options & fw_logs_parser::get_format_options_ref( int source_id ) const
        {
            auto iter = _source_to_formatting_options.find( source_id );
            if( iter != _source_to_formatting_options.end() )
                return iter->second;

            throw librealsense::invalid_value_exception( rsutils::string::from()
                                                         << "Invalid source ID received " << source_id );
        }

        std::string fw_logs_parser::get_source_name( int source_id ) const
        {
            auto iter = _source_id_to_name.find( source_id );
            if( iter != _source_id_to_name.end() )
                return iter->second;

            throw librealsense::invalid_value_exception( rsutils::string::from()
                                                         << "Invalid source ID received " << source_id );
        }

        rs2_log_severity fw_logs_parser::parse_severity( uint32_t severity ) const
        {
            return fw_logs::fw_logs_severity_to_rs2_log_severity( severity );
        }

        legacy_fw_logs_parser::legacy_fw_logs_parser( const std::string & xml_content )
            : fw_logs_parser( xml_content )
        {
            // Check expected size here, no need to check repeatedly in parse functions
            if( _source_to_formatting_options.size() != 1 )
                throw librealsense::invalid_value_exception( rsutils::string::from()
                                                             << "Legacy parser expect one formating options, have "
                                                             << _source_to_formatting_options.size() );
        }

        size_t legacy_fw_logs_parser::get_log_size( const uint8_t * ) const
        {
            return sizeof( legacy_fw_log_binary );
        }

        void legacy_fw_logs_parser::structure_timestamp( const fw_logs_binary_data * raw,
                                                         structured_binary_data * structured ) const
        {
            const auto actual_struct = reinterpret_cast< const legacy_fw_log_binary * >( raw->logs_buffer.data() );
            structured->timestamp = actual_struct->timestamp;
        }

        void legacy_fw_logs_parser::structure_params( const fw_logs_binary_data * raw,
                                                      size_t num_of_params,
                                                      structured_binary_data * structured ) const
        {
            const auto actual_struct = reinterpret_cast< const legacy_fw_log_binary * >( raw->logs_buffer.data() );
            uint16_t params_size_bytes = 0;
            if( num_of_params > 0 )
            {
                structured->params_info.push_back( { params_size_bytes, param_type::UINT16, 2 } );  // P1, uint16_t
                params_size_bytes += 2;
            }
            if( num_of_params > 1 )
            {
                structured->params_info.push_back( { params_size_bytes, param_type::UINT16, 2 } );  // P2, uint16_t
                params_size_bytes += 2;
            }
            if( num_of_params > 2 )
            {
                structured->params_info.push_back( { params_size_bytes, param_type::UINT32, 4 } );  // P3, uint32_t
                params_size_bytes += 4;
            }
            if( num_of_params > 3 )
                throw librealsense::invalid_value_exception( rsutils::string::from() <<
                                                             "Expecting max 3 parameters, received " << num_of_params );

            const uint8_t * blob_start = reinterpret_cast< const uint8_t * >( &actual_struct->p1 );
            structured->params_blob.insert( structured->params_blob.end(), blob_start, blob_start + params_size_bytes );
        }

        const fw_logs_formatting_options & legacy_fw_logs_parser::get_format_options_ref( int source_id ) const
        {
            return _source_to_formatting_options.begin()->second;
        }

        std::string legacy_fw_logs_parser::get_source_name( int source_id ) const
        {
            // Legacy FW logs had threads, not source
            return _source_to_formatting_options.begin()->second.get_thread_name( source_id );
        }

        rs2_log_severity legacy_fw_logs_parser::parse_severity( uint32_t severity ) const
        {
            return fw_logs::legacy_fw_logs_severity_to_rs2_log_severity( severity );
        }
    }
}
