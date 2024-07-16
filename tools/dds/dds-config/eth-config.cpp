// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "eth-config.h"
#include "eth-config-v3.h"

#include <rsutils/number/crc32.h>
#include <rsutils/string/hexdump.h>
#include <rsutils/string/from.h>


std::ostream & operator<<( std::ostream & os, link_priority p )
{
    if( static_cast< uint8_t >( p ) & static_cast< uint8_t >( link_priority::_dynamic_bit ) )
    {
        os << "dynamic-";
        p = static_cast< link_priority >( static_cast< uint8_t >( p )
                                          & ~static_cast< uint8_t >( link_priority::_dynamic_bit ) );
    }
    switch( p )
    {
    case link_priority::usb_only:
        os << "usb-only";
        break;
    case link_priority::usb_first:
        os << "usb-first";
        break;
    case link_priority::eth_only:
        os << "eth-only";
        break;
    case link_priority::eth_first:
        os << "eth-first";
        break;
    default:
        os << "UNKNOWN-" << (int) p;
        break;
    }
    return os;
}


std::ostream & operator<<( std::ostream & os, ip_3 const & ip3 )
{
    os << ip3.ip;
    if( ip3.netmask.is_valid() )
        os << " & " << ip3.netmask;
    if( ip3.gateway.is_valid() )
        os << " / " << ip3.gateway;
    return os;
}


eth_config::eth_config( eth_config_v3 const & v3 )
    : header( v3.header )
    , mac_address( rsutils::string::from( rsutils::string::hexdump( v3.mac_address, sizeof( v3.mac_address ) )
                                              .format( "{01}:{01}:{01}:{01}:{01}:{01}" ) ) )
    , configured{ v3.config_ip, v3.config_netmask, v3.config_gateway }
    , actual{ v3.actual_ip, v3.actual_netmask, v3.actual_gateway }
    , dds{ v3.domain_id }
    , link{ v3.mtu, v3.link_speed, v3.link_check_timeout, link_priority( v3.link_priority ) }
    , dhcp{ v3.dhcp_on != 0, v3.dhcp_timeout }
{
}


eth_config::eth_config( std::vector< uint8_t > const & hwm_response )
{
    auto header = reinterpret_cast< eth_config_header const * >( hwm_response.data() );
    if( hwm_response.size() < sizeof( *header ) )
        throw std::runtime_error( rsutils::string::from()
                                  << "HWM response size (" << hwm_response.size() << ") does not fit header (size "
                                  << sizeof( *header ) << ")" );

    if( hwm_response.size() != sizeof( *header ) + header->size )
        throw std::runtime_error( rsutils::string::from()
                                  << "HWM response size (" << hwm_response.size() << ") does not fit header ("
                                  << sizeof( *header ) << ") + header size (" << header->size << ")" );

    auto const crc = rsutils::number::calc_crc32( hwm_response.data() + sizeof( *header ), header->size );
    if( header->crc != crc )
        throw std::runtime_error( rsutils::string::from()
                                  << "Eth config table CRC (" << header->crc << ") does not match response " << crc );

    switch( header->version )
    {
    case 3: {
        if( header->size != sizeof( eth_config_v3 ) - sizeof( *header ) )
            throw std::runtime_error( rsutils::string::from()
                                      << "invalid Eth config table v3 size (" << header->size << "); expecting "
                                      << sizeof( eth_config_v3 ) << "-" << sizeof( *header ) );
        auto config = reinterpret_cast< eth_config_v3 const * >( hwm_response.data() );
        *this = *config;
        break;
    }

    default:
        throw std::runtime_error( rsutils::string::from()
                                  << "unrecognized Eth config table version " << header->version );
    }
}


bool eth_config::operator==( eth_config const & other ) const noexcept
{
    // Only compare those items that are configurable
    return configured.ip == other.configured.ip && configured.netmask == other.configured.netmask
        && configured.gateway == other.configured.gateway && dds.domain_id == other.dds.domain_id
        && dhcp.on == other.dhcp.on && link.priority == other.link.priority && link.timeout == other.link.timeout;
}


bool eth_config::operator!=( eth_config const & other ) const noexcept
{
    // Only compare those items that are configurable
    return configured.ip != other.configured.ip || configured.netmask != other.configured.netmask
        || configured.gateway != other.configured.gateway || dds.domain_id != other.dds.domain_id
        || dhcp.on != other.dhcp.on || link.priority != other.link.priority || link.timeout != other.link.timeout;
}


std::vector< uint8_t > eth_config::build_command() const
{
    std::vector< uint8_t > data;
    data.resize( sizeof( eth_config_v3 ) );
    eth_config_v3 & cfg = *reinterpret_cast< eth_config_v3 * >( data.data() );
    configured.ip.get_components( cfg.config_ip );
    configured.netmask.get_components( cfg.config_netmask );
    configured.gateway.get_components( cfg.config_gateway );
    cfg.dhcp_on = dhcp.on;
    cfg.dhcp_timeout = dhcp.timeout;
    cfg.domain_id = dds.domain_id;
    cfg.link_check_timeout = link.timeout;
    cfg.link_priority = (uint8_t)link.priority;

    cfg.mtu = 9000;  // R/O, but must be set to this value

    cfg.header.version = 3;
    cfg.header.size = sizeof( cfg ) - sizeof( cfg.header );
    cfg.header.crc = rsutils::number::calc_crc32( data.data() + sizeof( cfg.header ), cfg.header.size );
    return data;
}
