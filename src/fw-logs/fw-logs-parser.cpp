// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "fw-logs-parser.h"
#include "fw-string-formatter.h"
#include <rsutils/string/from.h>

#include <regex>
#include <sstream>
#include <stdint.h>

using namespace std;

namespace librealsense
{
    namespace fw_logs
    {
        fw_logs_parser::fw_logs_parser( const string & xml_content )
            : _fw_logs_formating_options( xml_content )
        {
            _fw_logs_formating_options.initialize_from_xml();
        }

        fw_log_data fw_logs_parser::parse_fw_log( const fw_logs_binary_data * fw_log_msg ) 
        {
            fw_log_data log_data = fw_log_data();

            if (!fw_log_msg || fw_log_msg->logs_buffer.size() == 0)
                return log_data;

            structured_binary_data structured = structure_common_data( fw_log_msg );

            fw_log_event log_event_data;
            _fw_logs_formating_options.get_event_data( structured.event_id, &log_event_data );

            structure_timestamp( fw_log_msg, &structured );
            structure_params( fw_log_msg, log_event_data.num_of_params, &structured );

            fw_string_formatter reg_exp( _fw_logs_formating_options.get_enums() );
            reg_exp.generate_message( log_event_data.line,
                                      structured.params_info,
                                      structured.params_blob,
                                      &log_data._message );
            
            log_data._severity = fw_log_msg->get_severity(); // TODO - get severity according to underlying struct
            log_data._line = structured.line;
            log_data._sequence = structured.sequence;
            log_data._timestamp = structured.timestamp;

            _fw_logs_formating_options.get_file_name( structured.file_id, &log_data._file_name );
            _fw_logs_formating_options.get_thread_name( structured.source_id, &log_data._source_name );
            //TODO - get module name

            return log_data;
        }

        fw_logs_parser::structured_binary_data
        fw_logs_parser::structure_common_data( const fw_logs_binary_data * fw_log_msg ) const
        {
            structured_binary_data log_data;

            auto * log_binary = reinterpret_cast< const fw_log_binary * >( fw_log_msg->logs_buffer.data() );

            log_data.severity = static_cast< uint32_t >( log_binary->severity );
            log_data.source_id = static_cast< uint32_t >( log_binary->source_id );
            log_data.file_id = static_cast< uint32_t >( log_binary->file_id );
            log_data.module_id = static_cast< uint32_t >( log_binary->module_id );
            log_data.event_id = static_cast< uint32_t >( log_binary->event_id );
            log_data.line = static_cast< uint32_t >( log_binary->line_id );
            log_data.sequence = static_cast< uint32_t >( log_binary->seq_id );

            return log_data;
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

        legacy_fw_logs_parser::legacy_fw_logs_parser( const std::string & xml_content )
            : fw_logs_parser( xml_content )
        {
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
    }
}
