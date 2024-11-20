// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#include "hw-monitor.h"
#include "types.h"
#include <rsutils/string/from.h>
#include <iomanip>
#include <limits>
#include <sstream>


static inline uint32_t pack( uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3 )
{
    return ( c0 << 24 ) | ( c1 << 16 ) | ( c2 << 8 ) | c3;
}


namespace librealsense
{

    std::string hw_monitor::get_module_serial_string(const std::vector<uint8_t>& buff, size_t index, size_t length)
    {
        std::stringstream formattedBuffer;
        for (auto i = 0; i < length; i++)
            formattedBuffer << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(buff[index + i]);

        return formattedBuffer.str();
    }

    void hw_monitor::fill_usb_buffer(int opCodeNumber, int p1, int p2, int p3, int p4,
        uint8_t const * data, int dataLength, uint8_t* bufferToSend, int& length)
    {
        auto preHeaderData = IVCAM_MONITOR_MAGIC_NUMBER;

        uint8_t* writePtr = bufferToSend;
        auto header_size = 4;

        auto cur_index = 2;
        memcpy(writePtr + cur_index, &preHeaderData, sizeof(uint16_t));
        cur_index += sizeof(uint16_t);
        memcpy(writePtr + cur_index, &opCodeNumber, sizeof(uint32_t));
        cur_index += sizeof(uint32_t);
        memcpy(writePtr + cur_index, &p1, sizeof(uint32_t));
        cur_index += sizeof(uint32_t);
        memcpy(writePtr + cur_index, &p2, sizeof(uint32_t));
        cur_index += sizeof(uint32_t);
        memcpy(writePtr + cur_index, &p3, sizeof(uint32_t));
        cur_index += sizeof(uint32_t);
        memcpy(writePtr + cur_index, &p4, sizeof(uint32_t));
        cur_index += sizeof(uint32_t);

        if (dataLength)
        {
            std::memcpy( writePtr + cur_index, data, dataLength );
            cur_index += dataLength;
        }

        length = cur_index;
        uint16_t tmp_size = length - header_size;
        memcpy(bufferToSend, &tmp_size, sizeof(uint16_t)); // Length doesn't include header
    }

    command hw_monitor::build_command_from_data(const std::vector<uint8_t> data)
    {
        uint32_t offset = 4; // skipping over first 4 bytes
        uint8_t opcode = data[offset];
        // despite the fact that the opcode is only uint8, we need to step over 4 bytes, 
        // as done in hw_monitor::fill_usb_buffer - the method that constructs the data for usb buffer
        offset += sizeof(uint32_t); 

        uint32_t param1 = *reinterpret_cast<const uint32_t*>(data.data() + offset);
        offset += sizeof(uint32_t);

        uint32_t param2 = *reinterpret_cast<const uint32_t*>(data.data() + offset);
        offset += sizeof(uint32_t);

        uint32_t param3 = *reinterpret_cast<const uint32_t*>(data.data() + offset);
        offset += sizeof(uint32_t);

        uint32_t param4 = *reinterpret_cast<const uint32_t*>(data.data() + offset);
        offset += sizeof(uint32_t);

        command cmd { opcode, param1, param2, param3, param4 };
        if (data.size() > size_of_command_without_data)
            cmd.data.insert(cmd.data.begin(), data.begin() + offset, data.end());

        return cmd;
    }

    void hw_monitor::execute_usb_command(uint8_t const *out, size_t outSize, uint32_t & op, uint8_t * in, 
        size_t & inSize, bool require_response) const
    {
        auto res = _locked_transfer->send_receive( out, outSize, 5000, require_response );

        // read
        if (require_response && in && inSize)
        {
            if (res.size() < static_cast<int>(sizeof(uint32_t)))
                throw invalid_value_exception("Incomplete bulk usb transfer!");

            //if (res.size() > IVCAM_MONITOR_MAX_BUFFER_SIZE)
             //   throw invalid_value_exception("Out buffer is greater than max buffer size!");

            op = *reinterpret_cast<uint32_t *>(res.data());
            //D457 uses an ad-hoc logic to transmit the data outside
            //if (res.size() > static_cast<int>(inSize))
            //    throw invalid_value_exception("bulk transfer failed - user buffer too small");

            inSize = std::min(res.size(),inSize); // For D457 only
            std::memcpy( in, res.data(), inSize );
        }
    }

    void hw_monitor::update_cmd_details(hwmon_cmd_details& details, size_t receivedCmdLen, unsigned char* outputBuffer)
    {
        details.receivedCommandDataLength = receivedCmdLen;

        if (!details.require_response) return;

        if (details.receivedCommandDataLength < 4)
            throw invalid_value_exception("received incomplete response to usb command");

        details.receivedCommandDataLength -= 4;
        std::memcpy( details.receivedOpcode.data(), outputBuffer, 4 );

        if (details.receivedCommandDataLength > 0)
            std::memcpy( details.receivedCommandData.data(), outputBuffer + 4, details.receivedCommandDataLength );
    }

    void hw_monitor::send_hw_monitor_command(hwmon_cmd_details& details) const
    {
        unsigned char outputBuffer[HW_MONITOR_BUFFER_SIZE];

        uint32_t op{};
        size_t receivedCmdLen = HW_MONITOR_BUFFER_SIZE;

        execute_usb_command(details.sendCommandData.data(), details.sizeOfSendCommandData, op,
            outputBuffer, receivedCmdLen, details.require_response);
        update_cmd_details(details, receivedCmdLen, outputBuffer);
    }

    std::vector< uint8_t > hw_monitor::send( std::vector< uint8_t > const & data ) const
    {
        return _locked_transfer->send_receive( data.data(), data.size() );
    }

    std::vector< uint8_t >
    hw_monitor::send( command const & cmd, hwmon_response_type * p_response, bool locked_transfer ) const
    {
        uint32_t const opCodeXmit = cmd.cmd;

        hwmon_cmd_details details;
        details.require_response = cmd.require_response;
        details.timeOut = cmd.timeout_ms;

        fill_usb_buffer( opCodeXmit,
                         cmd.param1,
                         cmd.param2,
                         cmd.param3,
                         cmd.param4,
                         cmd.data.data(),  // memcpy
                         std::min( (uint16_t)cmd.data.size(), HW_MONITOR_BUFFER_SIZE ),
                         details.sendCommandData.data(),
                         details.sizeOfSendCommandData );

        if (locked_transfer)
        {
            return _locked_transfer->send_receive( details.sendCommandData.data(), details.sendCommandData.size() );
        }

        send_hw_monitor_command(details);

        // Error/exit conditions
        if (p_response)
            *p_response = _hwmon_response->success_value();
        if( ! cmd.require_response )
            return {};

        // endian?
        auto opCodeAsUint32 = pack(details.receivedOpcode[3], details.receivedOpcode[2],
            details.receivedOpcode[1], details.receivedOpcode[0]);
        if (opCodeAsUint32 != opCodeXmit)
        {
            auto err_type = static_cast<hwmon_response_type>(opCodeAsUint32);
            //LOG_DEBUG(err);  // too intrusive; may be an expected error
            if( p_response )
            {
                *p_response = err_type;
                return {};
            }
            std::string err = hwmon_error_string( cmd, err_type );
            throw invalid_value_exception(err);
        }

        auto const pb = details.receivedCommandData.data();
        return std::vector<uint8_t>( pb, pb + details.receivedCommandDataLength );
    }

    /*static*/ std::vector<uint8_t> hw_monitor::build_command(uint32_t opcode,
        uint32_t param1,
        uint32_t param2,
        uint32_t param3,
        uint32_t param4,
        uint8_t const * data,
        size_t dataLength)
    {
        int length;
        std::vector<uint8_t> result;
        size_t length_of_command_with_data = dataLength + size_of_command_without_data;
        auto init_size = (length_of_command_with_data > IVCAM_MONITOR_MAX_BUFFER_SIZE) ? length_of_command_with_data : IVCAM_MONITOR_MAX_BUFFER_SIZE;
        result.resize(init_size);
        fill_usb_buffer(opcode, param1, param2, param3, param4, data, static_cast<int>(dataLength), result.data(), length);
        result.resize(length);
        return result;
    }

    std::string hw_monitor::hwmon_error_string( command const & cmd, hwmon_response_type response_code ) const
    {
        auto str = _hwmon_response->hwmon_error2str(response_code);
        std::ostringstream err;
        err << "hwmon command 0x" << std::hex << unsigned(cmd.cmd) << '(';
        err << ' ' << cmd.param1;
        err << ' ' << cmd.param2;
        err << ' ' << cmd.param3;
        err << ' ' << cmd.param4 << std::dec;
        err << " ) failed (response " << response_code << "= " << ( str.empty() ? "unknown" : str ) << ")";
        return err.str();
    }


    void hw_monitor::get_gvd( size_t sz,
                              unsigned char * gvd,
                              uint8_t gvd_cmd,
                              const std::set< int32_t > * retry_error_codes ) const
    {
        command command(gvd_cmd);
        hwmon_response_type response;
        auto data = send( command, &response );
        // If we get an error code that match to the error code defined as require retry,
        // we will retry the command until it succeed or we reach a timeout
        bool should_retry = retry_error_codes && retry_error_codes->find( response ) != retry_error_codes->end();
        if( should_retry )
        {
            constexpr size_t RETRIES = 50;
            for( int i = 0; i < RETRIES; ++i )
            {
                LOG_WARNING( "GVD not ready - retrying GET_GVD command" );
                std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
                data = send( command, &response );
                if( response == _hwmon_response->success_value() )
                    break;
                // If we failed after 'RETRIES' retries or it is less `RETRIES` and the error
                // code is not in the retry list than , raise an exception
                if( i >= ( RETRIES - 1 ) || retry_error_codes->find( response ) == retry_error_codes->end() )
                    throw io_exception( rsutils::string::from()
                                        << "error in querying GVD, error:"
                                        << _hwmon_response->hwmon_error2str( response ) );
                
            }
        }
        auto minSize = std::min(sz, data.size());
        std::memcpy( gvd, data.data(), minSize );
    }
}
