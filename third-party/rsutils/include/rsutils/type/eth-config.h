// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include "eth-config-header.h"

#include <rsutils/type/ip-address.h>


enum class link_priority : uint8_t
{
    usb_only = 0,
    eth_only = 1,
    eth_first = 2,
    usb_first = 3,

    _dynamic_bit = 0x10,
    dynamic_eth_first = eth_first | _dynamic_bit,
    dynamic_usb_first = usb_first | _dynamic_bit
};
std::ostream & operator<<( std::ostream & os, link_priority );


struct ip_3
{
    rsutils::type::ip_address ip;
    rsutils::type::ip_address netmask;
    rsutils::type::ip_address gateway;

    operator bool() const { return ip.is_valid(); }

    bool operator==( ip_3 const & other ) const { return ip == other.ip && netmask == other.netmask && gateway == other.gateway; }
    bool operator!=( ip_3 const & other ) const { return ip != other.ip || netmask != other.netmask || gateway != other.gateway; }
};
std::ostream & operator<<( std::ostream &, ip_3 const & );


struct eth_config_v3;


struct eth_config
{
    eth_config_header header;
    std::string mac_address;
    ip_3 configured;
    ip_3 actual;
    struct
    {
        int domain_id;  // dds domain id
    } dds;
    struct
    {
        unsigned mtu;      // bytes per packet
        unsigned speed;    // Mbps read-only; 0 if link is off
        unsigned timeout;  // The threshold to wait eth link(ms)
        link_priority priority;
    } link;
    struct
    {
        bool on;
        int timeout;  // The threshold to wait valid ip when DHCP is on(s)
    } dhcp;

    eth_config() {}
    explicit eth_config( std::vector< uint8_t > const & hwm_response_without_code );
    eth_config( eth_config_v3 const & );

    bool operator==( eth_config const & ) const noexcept;
    bool operator!=( eth_config const & ) const noexcept;

    std::vector< uint8_t > build_command() const;
};
