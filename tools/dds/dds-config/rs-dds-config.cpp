// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "eth-config.h"

#include <common/cli.h>

#include <rsutils/os/special-folder.h>
#include <rsutils/json.h>
#include <rsutils/json-config.h>
#include <rsutils/string/hexdump.h>
#include <rsutils/string/from.h>

#include <iostream>
#include <fstream>
#include <thread>
#include <set>

#include <librealsense2/rs.hpp>


using rsutils::json;
using rsutils::type::ip_address;


uint32_t const GET_ETH_CONFIG = 0xBB;
uint32_t const SET_ETH_CONFIG = 0xBA;
char const * const HWM_FMT = "{4} {04}({4i},{4i},{4i},{4i}) {repeat:}{1}{:}";


#define LOG_DEBUG( ... )                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        std::ostringstream os__;                                                                                       \
        os__ << __VA_ARGS__;                                                                                           \
        rs2_log( RS2_LOG_SEVERITY_DEBUG, os__.str().c_str(), nullptr );                                                \
    }                                                                                                                  \
    while( false )
bool g_quiet = false;
#define INFO( ... )                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if( ! g_quiet )                                                                                                \
            std::cout << "-I- " << __VA_ARGS__ << std::endl;                                                           \
    }                                                                                                                  \
    while( false )


struct indent
{
    int _spaces;
    indent( int spaces ) : _spaces( spaces ) {}
};
std::ostream & operator<<( std::ostream & os, indent const & ind )
{
    for( int i = ind._spaces; i; --i )
        os << ' ';
    return os;
}


template< class T >
struct _output_field
{
    char const * const _name;
    T const & _current;
    T const & _requested;
};
template< class T >
std::ostream & operator<<( std::ostream & os, _output_field< T > const & f )
{
    os << f._name << ": " << f._current;
    if( f._requested != f._current )
        os << " --> " << f._requested;
    return os;
}
template< class T >
_output_field< T > setting( const char * name, T const & current )
{
    return { name, current, current };
}
template< class T >
_output_field< T > setting( const char * name, T const & current, T const & requested )
{
    return { name, current, requested };
}


eth_config get_eth_config( rs2::debug_protocol hwm, bool golden )
{
    if( ! hwm )
        throw std::runtime_error( "no debug_protocol available" );
    auto cmd = hwm.build_command( GET_ETH_CONFIG, golden ? 0 : 1 );  // 0=golden; 1=actual
    LOG_DEBUG( "cmd: " << rsutils::string::hexdump( cmd.data(), cmd.size() ).format( HWM_FMT ) );
    auto data = hwm.send_and_receive_raw_data( cmd );
    int32_t const & code = *reinterpret_cast<int32_t const *>(data.data());
    if( data.size() < sizeof( code ) )
        throw std::runtime_error( rsutils::string::from()
                                  << "bad response " << rsutils::string::hexdump( data.data(), data.size() ) );
    if( code != GET_ETH_CONFIG )
        throw std::runtime_error( rsutils::string::from() << "bad response " << code );
    LOG_DEBUG( "response: " << rsutils::string::hexdump( data.data(), data.size() ).format( "{4} {repeat:}{1}{:}" ) );
    data.erase( data.begin(), data.begin() + sizeof( code ) );

    return eth_config( data );
}


bool find_device( rs2::context const & ctx,
                  rs2::device & device,
                  eth_config & config,
                  rs2::cli::value< std::string > & sn_arg,
                  bool const golden,
                  std::set< std::string > & devices_looked_at )
{
    auto device_list = ctx.query_devices();
    auto const n_devices = device_list.size();
    for( uint32_t i = 0; i < n_devices; ++i )
    {
        auto possible_device = device_list[i];
        try
        {
            std::string sn = possible_device.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
            if( sn_arg.isSet() && sn != sn_arg.getValue() )
                continue;
            if( ! devices_looked_at.insert( sn ).second )
                continue;  // insert failed: device was already looked at
            if( possible_device.supports( RS2_CAMERA_INFO_FIRMWARE_VERSION ) )
                LOG_DEBUG( "trying " << possible_device.get_description() << ", FW version "
                           << possible_device.get_info( RS2_CAMERA_INFO_FIRMWARE_VERSION ) );
            else
                LOG_DEBUG( "trying " << possible_device.get_description() );
            config = get_eth_config( possible_device, golden );
            if( device )
                throw std::runtime_error( "More than one device is available; please use --serial-number" );
            device = possible_device;
        }
        catch( std::exception const & e )
        {
            LOG_DEBUG( "failed! " << e.what() );
        }
    }
    return device;
}


int main( int argc, char * argv[] )
try
{
    using cli = rs2::cli_no_dds;

    cli::flag quiet_arg( "quiet", "Suppress regular informational (-I-) messages" );
    cli::flag reset_arg( "reset", "Hardware reset without making any changes" );
    cli::flag disable_arg( "disable", "Disable DDS in all librealsense examples & tools by default and exit" );
    cli::flag no_reset_arg( "no-reset", "Do not hardware reset after changes are made" );
    cli::flag golden_arg( "golden", "Show R/O golden values vs. current; mutually exclusive with any changes" );
    cli::flag factory_reset_arg( "factory-reset", "Reset settings back to the --golden values" );
    cli::value< std::string > sn_arg( "serial-number", "S/N", "", "Device serial-number to use, if more than one device is available" );
    cli::value< std::string > ip_arg( "ip", "address", "", "Device static IP address to use when DHCP is off" );
    cli::value< std::string > mask_arg( "mask", "1.2.3.4", "", "Device static IP network mask to use when DHCP is off" );
    cli::value< std::string > gateway_arg( "gateway", "1.2.3.4", "", "Device static IP network mask to use when DHCP is off" );
    cli::value< std::string > dhcp_arg( "dhcp", "on/off", "on", "DHCP dynamic IP discovery 'on' or 'off'" );
    cli::value< uint32_t > dhcp_timeout_arg( "dhcp-timeout", "seconds", 30, "Seconds before DHCP times out and falls back to a static IP" );
    cli::value< uint32_t > link_timeout_arg( "link-timeout", "milliseconds", 4000, "Milliseconds before --eth-first link times out and falls back to USB" );
    cli::value< int > domain_id_arg( "domain-id", "0-232", 0, "DDS Domain ID to use (default is 0)" );
    cli::flag usb_first_arg( "usb-first", "Prioritize USB before Ethernet" );
    cli::flag eth_first_arg( "eth-first", "Prioritize Ethernet and fall back to USB after link timeout" );
    cli::flag dynamic_priority_arg( "dynamic-priority", "Dynamically prioritize the last-working connection method (the default)" );

    json settings = cli( "rs-dds-config Ethernet device configuration tool" )
        .arg( quiet_arg )
        .arg( sn_arg )
        .arg( reset_arg )
        .arg( disable_arg )
        .arg( golden_arg )
        .arg( factory_reset_arg )
        .arg( usb_first_arg )
        .arg( eth_first_arg )
        .arg( dynamic_priority_arg )
        .arg( link_timeout_arg )
        .arg( dhcp_arg )
        .arg( dhcp_timeout_arg )
        .arg( ip_arg )
        .arg( mask_arg )
        .arg( gateway_arg )
        .arg( domain_id_arg )
        .arg( no_reset_arg )
        .process( argc, argv );

    g_quiet = quiet_arg.isSet();

    if( disable_arg.isSet() )
    {
        auto const filename = rsutils::os::get_special_folder( rsutils::os::special_folder::app_data ) + RS2_CONFIG_FILENAME;
        auto config = rsutils::json_config::load_from_file( filename );
        
        if( config.nested( "context", "dds", "enabled" ).default_value( false ) )
        {
            config["context"]["dds"].erase("enabled");
            std::ofstream out( filename );
            out << std::setw( 2 ) << config;
            out.close();
            INFO( "DDS has been disabled by default" );
        }
        else
        {
            INFO( "DDS is already disabled; no changes made" );
        }
        return EXIT_SUCCESS;
    }

    bool const golden = golden_arg.isSet();

    // Create a RealSense context and look for a device
    settings["dds"]["enabled"] = true;
    rs2::context ctx( settings.dump() );

    rs2::device device;
    eth_config current;
    std::set< std::string > devices_looked_at;
    if( ! find_device( ctx, device, current, sn_arg, golden, devices_looked_at ) )
    {
        if( sn_arg.isSet() )
            throw std::runtime_error( "Device not found or does not support Eth" );

        throw std::runtime_error( "No device found supporting Eth" );
    }
    if( device.supports( RS2_CAMERA_INFO_FIRMWARE_VERSION ) )
        INFO( "Device: " << device.get_description() << ", FW version "
                         << device.get_info( RS2_CAMERA_INFO_FIRMWARE_VERSION ) );
    else
        INFO( "Device: " << device.get_description() );

    eth_config requested( current );
    if( golden || factory_reset_arg.isSet() || reset_arg.isSet() )
    {
        if( ip_arg.isSet() || mask_arg.isSet() || usb_first_arg.isSet() || eth_first_arg.isSet()
            || dynamic_priority_arg.isSet() || link_timeout_arg.isSet() || dhcp_arg.isSet() || dhcp_timeout_arg.isSet() )
        {
            throw std::runtime_error( "Cannot change any settings with --golden, --factory-reset, or --reset" );
        }
        if( golden + factory_reset_arg.isSet() + reset_arg.isSet() > 1 )
        {
            throw std::runtime_error( "Mutually exclusive: --golden, --factory-reset, and --reset" );
        }
        if( reset_arg.isSet() )
        {
            INFO( "Resetting..." );
            device.hardware_reset();
            return EXIT_SUCCESS;
        }
        if( factory_reset_arg.isSet() )
        {
            LOG_DEBUG( "getting golden:" );
            requested = get_eth_config( device, true );  // golden
        }
        else
        {
            // Current is the golden values; show them vs the actual values in requested
            // No actual changes will be made
            LOG_DEBUG( "getting current:" );
            requested = get_eth_config( device, false );  // not golden
        }
    }
    else
    {
        if( ip_arg.isSet() )
            requested.configured.ip = ip_address( ip_arg.getValue(), rsutils::throw_if_not_valid );
        if( mask_arg.isSet() )
            requested.configured.netmask = ip_address( mask_arg.getValue(), rsutils::throw_if_not_valid );
        if( gateway_arg.isSet() )
            requested.configured.gateway = ip_address( gateway_arg.getValue(), rsutils::throw_if_not_valid );
        if( usb_first_arg.isSet() + eth_first_arg.isSet() + dynamic_priority_arg.isSet() > 1 )
            throw std::invalid_argument( "--usb-first, --eth-first, and --dynamic-priority are mutually exclusive" );
        if( usb_first_arg.isSet() )
            requested.link.priority = link_priority::usb_first;
        else if( eth_first_arg.isSet() )
            requested.link.priority = link_priority::eth_first;
        else if( dynamic_priority_arg.isSet() )
            requested.link.priority  // Enable eth-first if we have a link
                = current.link.speed ? link_priority::dynamic_eth_first : link_priority::dynamic_usb_first;
        if( link_timeout_arg.isSet() )
            requested.link.timeout = link_timeout_arg.getValue();
        if( dhcp_arg.isSet() )
        {
            if( dhcp_arg.getValue() == "on" )
                requested.dhcp.on = true;
            else if( dhcp_arg.getValue() == "off" )
                requested.dhcp.on = false;
            else
                throw std::invalid_argument( "--dhcp can be 'on' or 'off'; got " + dhcp_arg.getValue() );
        }
        if( dhcp_timeout_arg.isSet() )
            requested.dhcp.timeout = dhcp_timeout_arg.getValue();
        if( domain_id_arg.isSet() )
        {
            if( domain_id_arg.getValue() < 0 || domain_id_arg.getValue() > 232 )
                throw std::invalid_argument( "--domain-id must be 0-232" );
            requested.dds.domain_id = domain_id_arg.getValue();
        }
    }

    if( ! g_quiet )
    {
        INFO( indent( 4 ) << setting( "MAC address", current.mac_address ) );
        INFO( indent( 4 ) << setting( "configured", current.configured, requested.configured ) );
        if( ! golden && current.actual && current.actual != current.configured )
            INFO( indent( 4 ) << setting( "actual    ", current.actual ) );

        {
            INFO( indent( 4 ) << "DDS:" );
            INFO( indent( 8 ) << setting( "domain ID", current.dds.domain_id, requested.dds.domain_id ) );
        }
        {
            if( golden )
            {
                INFO( indent( 4 ) << "link:" );
            }
            else
            {
                if( current.link.speed )
                    INFO( indent( 4 ) << setting( "link", current.link.speed ) << " Mbps" );
                else
                    INFO( indent( 4 ) << setting( "link", "OFF" ) );
            }
            INFO( indent( 8 ) << setting( "MTU, bytes", current.link.mtu ) );
            INFO( indent( 8 ) << setting( "timeout, ms", current.link.timeout, requested.link.timeout ) );
            INFO( indent( 8 ) << setting( "priority", current.link.priority, requested.link.priority ) );
        }
        {
            std::string current_dhcp( current.dhcp.on ? "ON" : "OFF" );
            std::string requested_dhcp( requested.dhcp.on ? "ON" : "OFF" );
            INFO( indent( 4 ) << setting( "DHCP", current_dhcp, requested_dhcp ) );
            INFO( indent( 8 ) << setting( "timeout, sec", current.dhcp.timeout, requested.dhcp.timeout ) );
        }
    }

    if( golden )
    {
        LOG_DEBUG( "no changes requested with --golden" );
    }
    else if( requested == current )
    {
        LOG_DEBUG( "nothing to do" );

        // When running with no arguments, and DDS is disabled, and we actually find a DDS device, we automatically
        // enable DDS in future runs:
        if( device.get_type() == "DDS" )
        {
            auto const filename
                = rsutils::os::get_special_folder( rsutils::os::special_folder::app_data ) + RS2_CONFIG_FILENAME;
            auto config = rsutils::json_config::load_from_file( filename );

            bool enabled;
            if( ! config.nested( "context", "dds", "enabled" ).get_ex( enabled ) )
            {
                config["context"]["dds"]["enabled"] = true;
                try
                {
                    std::ofstream out( filename );
                    out << std::setw( 2 ) << config;
                    out.close();
                    INFO( "DDS is now enabled by default in " RS2_CONFIG_FILENAME );
                }
                catch( std::exception const & e )
                {
                    std::cout << "-W- " << "FAILED to enable DDS by default: " << e.what() << std::endl;
                }
            }
            else
            {
                LOG_DEBUG( "'dds/enabled' was pre-set in configuration file (to " << enabled << "); no changes made" );
            }
        }
    }
    else
    {
        rs2::debug_protocol hwm( device );
        auto cmd = hwm.build_command( SET_ETH_CONFIG, 0, 0, 0, 0, requested.build_command() );
        LOG_DEBUG( "cmd: " << rsutils::string::hexdump( cmd.data(), cmd.size() ).format( HWM_FMT ) );
        auto data = hwm.send_and_receive_raw_data( cmd );
        int32_t const & code = *reinterpret_cast< int32_t const * >( data.data() );
        if( data.size() != sizeof( code ) )
            throw std::runtime_error( rsutils::string::from()
                                      << "Failed to change: bad response size " << data.size() << ' '
                                      << rsutils::string::hexdump( data.data(), data.size() ) );
        if( code != SET_ETH_CONFIG )
            throw std::runtime_error( rsutils::string::from() << "Failed to change: bad response " << code );
        INFO( "Successfully changed" );
        if( ! no_reset_arg.isSet() )
        {
            INFO( "Resetting device..." );
            device.hardware_reset();
        }
    }

    return EXIT_SUCCESS;
}
catch( const rs2::error & e )
{
    std::cerr << "-F- RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args()
              << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch( const std::exception & e )
{
    std::cerr << "-F- " << e.what() << std::endl;
    return EXIT_FAILURE;
}
