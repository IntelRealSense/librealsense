// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <iostream>             // for cout

#include <../src/hw-monitor.h>
using librealsense::to_string;


rs2::device find_first_l515( rs2::context const& ctx )
{
    auto devices = ctx.query_devices();
    for( auto&& dev : devices )
    {
        bool dev_is_l515 = dev.supports( RS2_CAMERA_INFO_NAME ) ?
            std::string( dev.get_info( RS2_CAMERA_INFO_NAME ) ).find( "L515" ) != std::string::npos : false;
        if( dev_is_l515 )
            return dev;
    }
    return {};
}


std::ostream&
dump( std::ostream& os, std::vector< byte > const& bytes, std::string const& prefix = "..." )
{
    auto it = bytes.begin();
    while( it < bytes.end() )
    {
        os << std::string( to_string() << std::setfill( '0' ) << std::setw( 6 ) << std::hex
            << (it - bytes.begin()) );
        if( !prefix.empty() )
            os << ' ' << prefix;
        for( size_t offset = 0; offset < 16 && it < bytes.end(); ++offset, ++it )
        {
            os << ' ';
            os << (to_string()
                << std::setfill( '0' ) << std::setw( 2 ) << std::hex << +*it)
                .
                operator std::string();
        }
        os << std::endl;
    }
    return os;
}

void check( std::vector< byte >& bytes, uint32_t op_code_sent )
{
    if( bytes.size() < 4 )
        throw std::runtime_error( "unexpected size returned" );

    auto op_code_returned = librealsense::pack( bytes[3], bytes[2], bytes[1], bytes[0] );
    if( op_code_returned != op_code_sent )
    {
        auto response = static_cast<librealsense::hwmon_response>(op_code_returned);

        auto str = librealsense::hwmon_error2str( response );
        throw std::runtime_error( to_string()
            << "hwmon command 0x" << std::hex << unsigned( op_code_sent )
            << " failed (response " << response << "= "
            << (str.empty() ? "unknown" : str) << ")" );
    }

    bytes.erase( bytes.begin(), bytes.begin() + sizeof( op_code_returned ) );
    //std::cout << "Success" << std::endl;
}


std::vector< byte > mrd( rs2::debug_protocol& fw,
    unsigned start,
    unsigned end )  // not inclusive!
{
    /*
      <Command Name="MRD" CmdPipe="WinUsb" CmdInterface="UvcAndI2c" Opcode="0x01" I2cRegOffset="0x3008" I2cCmdType="ReadAfterWrite" CmdPermission="Public" ReadFormat="Doubles" IsReadCommand="true" Description="Read Tensilica memory ( 32bit ). Output : 32bit dump">
        <Parameter1 Name="Offset" />
        <Parameter2 Name="End Address" />
      </Command>
    */
    std::cout << "\nMRD:\n";
    std::array< byte, librealsense::HW_MONITOR_BUFFER_SIZE > cmd;
    int cb;
    librealsense::hw_monitor::fill_usb_buffer( 0x01, start, end, 0, 0, nullptr, 0, cmd.data(), cb );
    auto bytes = fw.send_and_receive_raw_data( { cmd.begin(), cmd.end() } );
    check( bytes, 0x01 );
    dump( std::cout, bytes );
    return bytes;
}

void mwd( rs2::debug_protocol& fw,
    unsigned start,
    unsigned end,  // not inclusive!
    std::vector< byte > const& bytes )
{
    /*
      <Command Name="MWD" CmdPipe="WinUsb" CmdInterface="UvcAndI2c" Opcode="0x02" I2cRegOffset="0x3014" I2cCmdType="WriteOnly" CmdPermission="Private" Description="Write Tensilica memory ( 32bit )">
        <Parameter1 Name="Start Address" />
        <Parameter2 Name="End Address" />
        <Data Name="Data" IsReverseBytes="true" FormatLength="8" />
      </Command>
    */
    std::cout << "\nMWD\n";
    dump( std::cout, bytes );
    std::array< byte, librealsense::HW_MONITOR_BUFFER_SIZE > cmd;
    int cb;
    librealsense::hw_monitor::fill_usb_buffer( 0x02, start, end, 0, 0, bytes.data(), (int)bytes.size(), cmd.data(), cb );
    auto rbytes = fw.send_and_receive_raw_data( { cmd.begin(), cmd.end() } );
    check( rbytes, 0x02 );
    dump( std::cout, rbytes );
}


int main(int argc, char * argv[]) try
{
    rs2::context ctx;
    auto device = find_first_l515( ctx );
    if( ! device )
    {
        std::cerr << "No L515 device found!" << std::endl;
        return 1;
    }
    auto fw = device.as< rs2::debug_protocol >();

    std::vector< byte > bytes = mrd( fw, 0x90000040, 0x90000044 );

    bytes[1] += 1;
    mwd( fw, 0x90000040, 0x90000044, bytes );
    // This specific memory location will get changed only while streaming color!
    // So, if you see the same value in the next MRD, that's why...

    mrd( fw, 0x90000040, 0x90000044 );

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
