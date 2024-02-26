// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "firmware_logger_device.h"

namespace librealsense
{
    firmware_logger_device::firmware_logger_device( std::shared_ptr< const device_info > const & dev_info,
                                                    std::shared_ptr< hw_monitor > hardware_monitor,
                                                    const command & fw_logs_command )
        : device( dev_info )
        , _fw_logs_command( fw_logs_command )
        , _hw_monitor( hardware_monitor )
        , _fw_logs()
        , _parser( nullptr )
    {
        if( ! _hw_monitor )
            throw librealsense::invalid_value_exception( "HW monitor is empty" );
    }

    void firmware_logger_device::start()
    {
        if( ! _parser )
            throw librealsense::wrong_api_call_sequence_exception( "FW log parser is not initialized" );

        command start_command = _parser->get_start_command();
        start_command.cmd = _fw_logs_command.cmd; // Opcode comes from the device, may be different between devices
        if( start_command.cmd != 0 )
            _hw_monitor->send( start_command );
    }

    void firmware_logger_device::stop()
    {
        if( ! _parser )
            throw librealsense::wrong_api_call_sequence_exception( "FW log parser is not initialized" );

        command stop_command = _parser->get_stop_command();
        stop_command.cmd = _fw_logs_command.cmd; // Opcode comes from the device, may be different between devices
        if( stop_command.cmd != 0 )
            _hw_monitor->send( stop_command );
    }

    bool firmware_logger_device::get_fw_log( fw_logs::fw_logs_binary_data & binary_data )
    {
        bool result = false;

        if( _fw_logs.empty() )
        {
            get_fw_logs_from_hw_monitor();
        }

        if( ! _fw_logs.empty() )
        {
            binary_data = std::move( _fw_logs.front() );
            _fw_logs.pop();
            result = true;
        }

        return result;
    }

    unsigned int firmware_logger_device::get_number_of_fw_logs() const
    {
        return (unsigned int)_fw_logs.size();
    }

    void firmware_logger_device::get_fw_logs_from_hw_monitor()
    {
        if( ! _parser )
            throw librealsense::wrong_api_call_sequence_exception( "FW log parser is not initialized" );

        auto res = _hw_monitor->send( _fw_logs_command );
        if( res.empty() )
        {
            return;
        }

        // Convert bytes to fw_logs_binary_data
        auto beginOfLogIterator = res.data();
        size_t size_left = res.size();
        while( size_left > 0 )
        {
            size_t log_size = _parser->get_log_size( beginOfLogIterator );
            if( log_size > size_left )
                throw librealsense::invalid_value_exception( "Received an incomplete FW log" );
            auto endOfLogIterator = beginOfLogIterator + log_size;
            fw_logs::fw_logs_binary_data binary_data;
            binary_data.logs_buffer.insert( binary_data.logs_buffer.begin(), beginOfLogIterator, endOfLogIterator );
            _fw_logs.push( std::move( binary_data ) );
            beginOfLogIterator = endOfLogIterator;
            size_left -= log_size;
        }
    }

    bool firmware_logger_device::init_parser( std::string xml_content )
    {
        _parser = std::make_unique< fw_logs::fw_logs_parser >( xml_content );

        return ( _parser != nullptr );
    }

    bool firmware_logger_device::parse_log( const fw_logs::fw_logs_binary_data * fw_log_msg,
                                            fw_logs::fw_log_data * parsed_msg )
    {
        bool result = false;
        if( _parser && parsed_msg && fw_log_msg )
        {
            *parsed_msg = _parser->parse_fw_log( fw_log_msg );
            result = true;
        }

        return result;
    }

    legacy_firmware_logger_device::legacy_firmware_logger_device( std::shared_ptr< const device_info > const & dev_info,
                                                                  std::shared_ptr< hw_monitor > hardware_monitor,
                                                                  const command & fw_logs_command,
                                                                  const command & flash_logs_command )
        : firmware_logger_device( dev_info, hardware_monitor, fw_logs_command )
        , _flash_logs_command( flash_logs_command )
        , _flash_logs()
        , _flash_logs_initialized( false )
    {
    }

    bool legacy_firmware_logger_device::init_parser( std::string xml_content )
    {
        _parser = std::make_unique< fw_logs::legacy_fw_logs_parser >( xml_content );

        return ( _parser != nullptr );
    }
    
    bool legacy_firmware_logger_device::get_flash_log( fw_logs::fw_logs_binary_data & binary_data )
    {
        bool result = false;

        if( ! _flash_logs_initialized )
        {
            get_flash_logs_from_hw_monitor();
        }

        if( ! _flash_logs.empty() )
        {
            binary_data = std::move( _flash_logs.front() );
            _flash_logs.pop();
            result = true;
        }

        return result;
    }
    
    void legacy_firmware_logger_device::get_flash_logs_from_hw_monitor()
    {
        auto res = _hw_monitor->send( _flash_logs_command );

        if( res.empty() )
        {
            LOG_INFO( "Getting Flash logs failed!" );
            return;
        }

        // Erasing header
        int size_of_flash_logs_header = 27;
        res.erase( res.begin(), res.begin() + size_of_flash_logs_header );

        // Convert bytes to flash_logs_binary_data. Flash logs are supported only for legacy devices.
        auto beginOfLogIterator = res.begin();
        for( int i = 0; i < res.size() / sizeof( fw_logs::legacy_fw_log_binary ) && *beginOfLogIterator == 160; ++i )
        {
            auto endOfLogIterator = beginOfLogIterator + sizeof( fw_logs::legacy_fw_log_binary );
            std::vector< uint8_t > resultsForOneLog;
            resultsForOneLog.insert( resultsForOneLog.begin(), beginOfLogIterator, endOfLogIterator );
            fw_logs::fw_logs_binary_data binary_data{ resultsForOneLog };
            _flash_logs.push( binary_data );
            beginOfLogIterator = endOfLogIterator;
        }

        _flash_logs_initialized = true;
    }
}
