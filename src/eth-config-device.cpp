// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 RealSense, Inc. All Rights Reserved.

#include "eth-config-device.h"

#include <rsutils/string/from.h>
#include <rsutils/type/eth-config.h>
#include <librealsense2/h/rs_eth_config.h>

namespace librealsense
{
    // Command opcodes from rs-dds-config tool
    constexpr uint32_t GET_ETH_CONFIG = 0xBB;
    constexpr uint32_t SET_ETH_CONFIG = 0xBA;
    constexpr uint32_t ETH_CONFIG_DEFAULT = 0;  // For factory reset
    constexpr uint32_t ETH_CONFIG_CURRENT = 1;  // For current values

    void eth_config_device::init( debug_interface * debug )
    {
        if( ! debug )
            throw std::runtime_error( "eth_config_device::init called with null parameter" );

        _debug = debug;
        if( supports_ethernet_configuration() )
        {
            _supported = true;
            _curr_config = read_eth_config_from_device();
        }
    }

    bool eth_config_device::supports_ethernet_configuration()
    {
        try
        {
            read_eth_config_from_device(); // Test if we can successfully get current configuration
            return true;
        }
        catch( ... )
        {
            return false;
        }
    }

    rsutils::type::eth_config eth_config_device::read_eth_config_from_device( bool default_values )
    {
        if( ! _debug )
            throw std::runtime_error( "debug_interface not set for eth_config_device" );

        uint32_t type = default_values ? ETH_CONFIG_DEFAULT : ETH_CONFIG_CURRENT;
        auto cmd = _debug->build_command( GET_ETH_CONFIG, type );
        auto data = _debug->send_receive_raw_data( cmd );
        
        if( data.size() < sizeof( int32_t ) )
            throw std::runtime_error( "Invalid response size for Ethernet configuration" );

        uint32_t const & code = *reinterpret_cast< uint32_t const * >( data.data() );
        if( code != GET_ETH_CONFIG )
            throw std::runtime_error( rsutils::string::from() << "Error code response for Ethernet configuration: " << code );

        // Remove the response code from data
        data.erase( data.begin(), data.begin() + sizeof( code ) );

        return rsutils::type::eth_config( data );
    }

    void eth_config_device::save_eth_config_to_device( const rsutils::type::eth_config & config, bool default_values )
    {
        if( ! _debug )
            throw std::runtime_error( "debug_interface not set for eth_config_device" );

        auto config_buffer = config.build_command();
        uint32_t type = default_values ? ETH_CONFIG_DEFAULT : ETH_CONFIG_CURRENT;
        auto cmd = _debug->build_command( SET_ETH_CONFIG, type, 0, 0, 0, config_buffer.data(), config_buffer.size() );
        auto data = _debug->send_receive_raw_data( cmd );

        if( data.size() != sizeof( int32_t ) )
            throw std::runtime_error( "Invalid response size for setting Ethernet configuration" );

        int32_t const & code = *reinterpret_cast< int32_t const * >( data.data() );
        if( code != SET_ETH_CONFIG )
            throw std::runtime_error( rsutils::string::from() << "Failed to set Ethernet configuration, response code: " << code );
    }

    void eth_config_device::validate_support( bool for_write ) const
    {
        if( ! _supported )
            throw std::runtime_error( "Ethernet configuration is not supported for this device" );
        if( for_write && _curr_config.header.version > 5 )
            throw std::invalid_argument( rsutils::string::from()
                                        << "FW eth-config v" << _curr_config.header.version
                                        << " is newer than SDK, please update SDK" );
    }

    template< class Update > void eth_config_device::update_eth_config( Update update )
    {
        validate_support();
        auto tmp_config = _curr_config;
        update( tmp_config );
        tmp_config.validate();  // Reject invalid input before issuing a firmware GET.
        tmp_config = read_eth_config_from_device();
        update( tmp_config );
        save_eth_config_to_device( tmp_config );
        _curr_config = tmp_config;
    }

    // Link configuration
    uint32_t eth_config_device::get_ethernet_link_speed() const
    {
        validate_support();
        return _curr_config.link.speed;
    }

    rsutils::type::link_priority eth_config_device::get_link_priority() const
    {
        validate_support();
        return _curr_config.link.priority;
    }

    void eth_config_device::set_link_priority( rsutils::type::link_priority priority )
    {
        update_eth_config( [&]( rsutils::type::eth_config & config ) {
            config.link.priority = priority;
        } );
    }

    uint32_t eth_config_device::get_link_timeout() const
    {
        validate_support();
        return _curr_config.link.timeout;
    }

    void eth_config_device::set_link_timeout( uint32_t timeout )
    {
        update_eth_config( [&]( rsutils::type::eth_config & config ) {
            config.link.timeout = timeout;
        } );
    }

    uint32_t eth_config_device::get_dds_domain_id() const
    {
        validate_support();
        return _curr_config.dds.domain_id;
    }

    void eth_config_device::set_dds_domain_id( uint32_t domain )
    {
        update_eth_config( [&]( rsutils::type::eth_config & config ) {
            config.dds.domain_id = domain;
        } );
    }

    void eth_config_device::get_ip_address( rsutils::type::ip_address & configured_ip, rsutils::type::ip_address & actual_ip ) const
    {
        validate_support();
        configured_ip = _curr_config.configured.ip;
        actual_ip = _curr_config.actual.ip;
    }

    void eth_config_device::set_ip_address( const rsutils::type::ip_address & ip )
    {
        update_eth_config( [&]( rsutils::type::eth_config & config ) {
            config.configured.ip = ip;
        } );
    }

    void eth_config_device::get_netmask( rsutils::type::ip_address & configured_netmask, rsutils::type::ip_address & actual_netmask ) const
    {
        validate_support();
        configured_netmask = _curr_config.configured.netmask;
        actual_netmask = _curr_config.actual.netmask;
    }

    void eth_config_device::set_netmask( const rsutils::type::ip_address & netmask )
    {
        update_eth_config( [&]( rsutils::type::eth_config & config ) {
            config.configured.netmask = netmask;
        } );
    }

    void eth_config_device::get_gateway( rsutils::type::ip_address & configured_gateway, rsutils::type::ip_address & actual_gateway ) const
    {
        validate_support();
        configured_gateway = _curr_config.configured.gateway;
        actual_gateway = _curr_config.actual.gateway;
    }

    void eth_config_device::set_gateway( const rsutils::type::ip_address & gateway )
    {
        update_eth_config( [&]( rsutils::type::eth_config & config ) {
            config.configured.gateway = gateway;
        } );
    }

    void eth_config_device::get_dhcp_config( bool & enabled, uint8_t & timeout ) const
    {
        validate_support();
        enabled = _curr_config.dhcp.on;
        timeout = _curr_config.dhcp.timeout;
    }

    void eth_config_device::set_dhcp_config( bool enabled, uint8_t timeout )
    {
        update_eth_config( [&]( rsutils::type::eth_config & config ) {
            config.dhcp.on = enabled;
            config.dhcp.timeout = timeout;
        } );
    }

    // Network configuration
    uint32_t eth_config_device::get_mtu() const
    {
        validate_support();
        return _curr_config.link.mtu;
    }

    void eth_config_device::set_mtu( uint32_t mtu )
    {
        update_eth_config( [&]( rsutils::type::eth_config & config ) {
            config.link.mtu = mtu;
        } );
    }

    uint32_t eth_config_device::get_transmission_delay() const
    {
        validate_support();
        return _curr_config.transmission_delay;
    }

    void eth_config_device::set_transmission_delay( uint32_t delay )
    {
        update_eth_config( [&]( rsutils::type::eth_config & config ) {
            config.transmission_delay = delay;
        } );
    }

    uint8_t eth_config_device::get_udp_ttl() const
    {
        validate_support();
        return _curr_config.udp_ttl;
    }

    void eth_config_device::set_udp_ttl( uint8_t ttl )
    {
        update_eth_config( [&]( rsutils::type::eth_config & config ) {
            config.udp_ttl = ttl;
        } );
    }

    // Factory reset
    void eth_config_device::restore_defaults()
    {
        validate_support( true );
        auto default_config = read_eth_config_from_device( true );
        save_eth_config_to_device( default_config ); // Reading default but writing to current
        _curr_config = default_config;
    }

} // namespace librealsense
