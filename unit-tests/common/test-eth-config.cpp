// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

//#cmake:add-file ../../src/eth-config-device.cpp

#include <unit-tests/test.h>
#include <src/eth-config-device.h>
#include <rsutils/type/eth-config-struct.h>
#include <rsutils/number/crc32.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iterator>

namespace {

using rsutils::type::eth_config;
using rsutils::type::eth_config_header;
using rsutils::type::eth_config_v3;
using rsutils::type::eth_config_v4;
using rsutils::type::eth_config_v5;
using rsutils::type::ip_address;
using rsutils::type::link_priority;
using librealsense::eth_config_device;
using bytes = std::vector< uint8_t >;

void update_header( bytes & data )
{
    auto & header = *reinterpret_cast< eth_config_header * >( data.data() );
    header.size = static_cast< uint16_t >( data.size() - sizeof( header ) );
    header.crc = rsutils::number::calc_crc32( data.data() + sizeof( header ), header.size );
}

template< class Config > bytes make_table( uint16_t version, size_t extra = 0 )
{
    Config config = {};
    config.header.version = version;
    config.link_check_timeout = 5000;
    config.config_ip[0] = 192;
    config.config_ip[1] = 168;
    config.config_ip[2] = 1;
    config.config_ip[3] = 10;
    std::fill_n( config.config_netmask, 3, 255 );
    config.actual_ip[0] = 10;
    config.actual_ip[3] = 20;
    config.link_speed = 1000;
    config.mtu = 9000;
    config.dhcp_on = 1;
    config.dhcp_timeout = 30;
    config.domain_id = 2;
    config.link_priority = 2;
    std::fill( std::begin( config.mac_address ), std::end( config.mac_address ), 0x42 );
    std::fill( std::begin( config.reserved ), std::end( config.reserved ), 0xa5 );
    bytes data( sizeof( config ) + extra, 0x5a );
    std::memcpy( data.data(), &config, sizeof( config ) );
    update_header( data );
    return data;
}

bytes make_table( uint16_t version, size_t extra = 0 )
{
    if( version == 3 )
        return make_table< eth_config_v3 >( version, extra );
    if( version == 4 )
        return make_table< eth_config_v4 >( version, extra );
    return make_table< eth_config_v5 >( version, extra );
}

class debug_device : public librealsense::debug_interface
{
public:
    bytes current = make_table( 5 );
    bytes defaults = make_table( 5, 8 );
    std::vector< uint32_t > commands;
    bool fail_read = false;
    bool fail_write = false;

    bytes build_command( uint32_t opcode, uint32_t param1, uint32_t, uint32_t, uint32_t,
                         uint8_t const * data, size_t size ) const override
    {
        bytes command( 2 * sizeof( uint32_t ) + size );
        std::memcpy( command.data(), &opcode, sizeof( opcode ) );
        std::memcpy( command.data() + sizeof( opcode ), &param1, sizeof( param1 ) );
        if( size )
            std::memcpy( command.data() + 2 * sizeof( uint32_t ), data, size );
        return command;
    }

    bytes send_receive_raw_data( bytes const & command ) override
    {
        uint32_t opcode, type;
        std::memcpy( &opcode, command.data(), sizeof( opcode ) );
        std::memcpy( &type, command.data() + sizeof( opcode ), sizeof( type ) );
        commands.push_back( opcode );
        bytes response( sizeof( opcode ) );
        std::memcpy( response.data(), &opcode, sizeof( opcode ) );
        if( opcode == 0xbb )
        {
            if( fail_read )
                throw std::runtime_error( "GET failed" );
            auto const & data = type ? current : defaults;
            response.resize( sizeof( opcode ) + data.size() );
            std::memcpy( response.data() + sizeof( opcode ), data.data(), data.size() );
        }
        else
        {
            REQUIRE( opcode == 0xba );
            REQUIRE( type == 1 );
            if( fail_write )
                throw std::runtime_error( "SET failed" );
            current.assign( command.begin() + 2 * sizeof( uint32_t ), command.end() );
        }
        return response;
    }

    std::string get_opcode_string( int ) const override { return "eth-config-test"; }
};

}  // namespace

TEST_CASE( "Ethernet configuration preserves all unmodified bytes", "[eth-config]" )
{
    for( uint16_t version : { 3, 4, 5 } )
        for( size_t extra : { 0, 12 } )
        {
            CAPTURE( version, extra );
            auto original = make_table( version, extra );
            eth_config config( original );
            CHECK( config.build_command() == original );

            // Copying the parsed object must retain the opaque firmware fields.
            auto changed = config;
            changed.link.timeout = 6000;
            auto expected = original;
            reinterpret_cast< eth_config_v3 * >( expected.data() )->link_check_timeout = 6000;
            update_header( expected );
            CHECK( changed.build_command() == expected );
            CHECK( config.build_command() == original );
        }
}

TEST_CASE( "Typed Ethernet configuration constructors preserve reserved bytes", "[eth-config]" )
{
    auto v3 = make_table( 3 );
    auto v4 = make_table( 4 );
    auto v5 = make_table( 5 );
    CHECK( eth_config( *reinterpret_cast< eth_config_v3 const * >( v3.data() ) ).build_command() == v3 );
    CHECK( eth_config( *reinterpret_cast< eth_config_v4 const * >( v4.data() ) ).build_command() == v4 );
    CHECK( eth_config( *reinterpret_cast< eth_config_v5 const * >( v5.data() ) ).build_command() == v5 );
}

TEST_CASE( "New Ethernet configurations use the correct version layout", "[eth-config]" )
{
    for( uint16_t version : { 3, 4, 5 } )
    {
        eth_config config;
        config.header.version = version;
        config.configured = { ip_address( "192.168.1.10" ), ip_address( "255.255.255.0" ), ip_address() };
        config.dds = { 2 };
        config.link = { 9000, 0, 5000, link_priority::eth_first };
        config.dhcp = { true, 30 };
        config.transmission_delay = 0;
        config.udp_ttl = 0;
        auto const data = config.build_command();
        CHECK( data.size() == make_table( version ).size() );
        CHECK_NOTHROW( eth_config{ data } );
    }
}

TEST_CASE( "Ethernet configuration rejects malformed responses", "[eth-config]" )
{
    CHECK_THROWS( eth_config( bytes( sizeof( eth_config_header ) - 1 ) ) );
    for( uint16_t version : { 3, 4, 5 } )
    {
        CAPTURE( version );
        auto data = make_table( version );
        data.back() ^= 1;
        CHECK_THROWS( eth_config{ data } );  // Bad CRC
        data = make_table( version );
        data.pop_back();
        CHECK_THROWS( eth_config{ data } );  // Header length mismatch
        update_header( data );
        CHECK_THROWS( eth_config{ data } );  // Too short for the known version, even with a valid CRC
    }
    auto data = make_table( 5 );
    reinterpret_cast< eth_config_header * >( data.data() )->version = 2;
    CHECK_THROWS( eth_config{ data } );
    data = make_table( 6 );
    data.pop_back();
    update_header( data );
    CHECK_THROWS( eth_config{ data } );  // A newer version still needs the entire v5 prefix.
    data = make_table( 6 );
    data.back() ^= 1;
    CHECK_THROWS( eth_config{ data } );  // Newer versions require a valid CRC too.
}

TEST_CASE( "Ethernet setters read current device configuration before writing", "[eth-config]" )
{
    struct setter
    {
        char const * name;
        std::function< void( eth_config_device & ) > set;
        std::function< void( eth_config_v5 & ) > change;
    };
    setter const setters[] = {
        { "priority", []( eth_config_device & d ) { d.set_link_priority( link_priority::usb_first ); },
          []( eth_config_v5 & c ) { c.link_priority = 3; } },
        { "timeout", []( eth_config_device & d ) { d.set_link_timeout( 6000 ); },
          []( eth_config_v5 & c ) { c.link_check_timeout = 6000; } },
        { "domain", []( eth_config_device & d ) { d.set_dds_domain_id( 40 ); },
          []( eth_config_v5 & c ) { c.domain_id = 40; } },
        { "ip", []( eth_config_device & d ) { d.set_ip_address( ip_address( "192.168.1.11" ) ); },
          []( eth_config_v5 & c ) { c.config_ip[3] = 11; } },
        { "netmask", []( eth_config_device & d ) { d.set_netmask( ip_address( "255.255.0.0" ) ); },
          []( eth_config_v5 & c ) { c.config_netmask[2] = 0; } },
        { "gateway", []( eth_config_device & d ) { d.set_gateway( ip_address( "192.168.1.1" ) ); },
          []( eth_config_v5 & c ) { uint8_t ip[] = {192,168,1,1}; std::copy_n( ip, 4, c.config_gateway ); } },
        { "dhcp", []( eth_config_device & d ) { d.set_dhcp_config( false, 20 ); },
          []( eth_config_v5 & c ) { c.dhcp_on = 0; c.dhcp_timeout = 20; } },
        { "mtu", []( eth_config_device & d ) { d.set_mtu( 7000 ); },
          []( eth_config_v5 & c ) { c.mtu = 7000; } },
        { "delay", []( eth_config_device & d ) { d.set_transmission_delay( 30 ); },
          []( eth_config_v5 & c ) { c.transmission_delay = 30; } },
        { "ttl", []( eth_config_device & d ) { d.set_udp_ttl( 64 ); },
          []( eth_config_v5 & c ) { c.udp_ttl = 64; } },
    };
    for( auto const & setter : setters )
    {
        CAPTURE( setter.name );
        debug_device debug;
        // Include an opaque tail as well as the version's existing reserved bytes.
        debug.current = make_table( 5, 12 );
        eth_config_device device;
        device.init( &debug );
        debug.commands.clear();

        // Simulate another client or firmware changing the table after SDK initialization.
        auto & current = *reinterpret_cast< eth_config_v5 * >( debug.current.data() );
        current.domain_id = 23;
        current.link_speed = 2500;
        current.reserved[0] = 0x37;
        debug.current.back() = 0x81;
        update_header( debug.current );
        auto expected = debug.current;
        setter.change( *reinterpret_cast< eth_config_v5 * >( expected.data() ) );
        update_header( expected );

        setter.set( device );
        CHECK( debug.commands == std::vector< uint32_t >{ 0xbb, 0xba } );
        CHECK( debug.current == expected );
        CHECK( device.get_ethernet_link_speed() == 2500 );
        CHECK( device.get_dds_domain_id() == reinterpret_cast< eth_config_v5 const * >( expected.data() )->domain_id );
    }
}

TEST_CASE( "Failed Ethernet updates preserve the SDK cache and device table", "[eth-config]" )
{
    debug_device debug;
    eth_config_device device;
    device.init( &debug );
    auto const original = debug.current;
    debug.commands.clear();

    SECTION( "Read failure prevents SET" )
    {
        debug.fail_read = true;
        CHECK_THROWS( device.set_link_timeout( 6000 ) );
        CHECK( debug.commands == std::vector< uint32_t >{ 0xbb } );
    }
    SECTION( "Write failure does not publish the requested configuration" )
    {
        debug.fail_write = true;
        CHECK_THROWS( device.set_link_timeout( 6000 ) );
        CHECK( debug.commands == std::vector< uint32_t >{ 0xbb, 0xba } );
    }
    SECTION( "Invalid field values prevent GET and SET" )
    {
        CHECK_THROWS_AS( device.set_link_timeout( 1 ), std::invalid_argument );
        CHECK_THROWS_AS( device.set_link_timeout( 2050 ), std::invalid_argument );
        CHECK_THROWS_AS( device.set_dds_domain_id( 233 ), std::invalid_argument );
        CHECK_THROWS_AS( device.set_ip_address( ip_address() ), std::invalid_argument );
        CHECK_THROWS_AS( device.set_netmask( ip_address() ), std::invalid_argument );
        CHECK_THROWS_AS( device.set_mtu( 501 ), std::invalid_argument );
        CHECK_THROWS_AS( device.set_transmission_delay( 145 ), std::invalid_argument );
        CHECK( debug.commands.empty() );
    }
    CHECK( debug.current == original );
    CHECK( device.get_link_timeout() == 5000 );
}

TEST_CASE( "Ethernet factory reset preserves the complete default table", "[eth-config]" )
{
    debug_device debug;
    SECTION( "Valid current settings" ) {}
    SECTION( "Factory reset can recover invalid current field values" )
    {
        reinterpret_cast< eth_config_v5 * >( debug.current.data() )->link_check_timeout = 1;
        update_header( debug.current );
    }
    eth_config_device device;
    device.init( &debug );
    debug.commands.clear();
    device.restore_defaults();
    CHECK( debug.commands == std::vector< uint32_t >{ 0xbb, 0xba } );
    CHECK( debug.current == debug.defaults );
}

TEST_CASE( "Parsed Ethernet configuration versions cannot be changed", "[eth-config]" )
{
    for( uint16_t version : { 3, 4, 5 } )
        for( size_t extra : { 0, 64 } )
        {
            CAPTURE( version, extra );
            eth_config config( make_table( version, extra ) );
            for( uint16_t changed_version : { 3, 4, 5 } )
            {
                if( changed_version == version )
                    continue;
                config.header.version = changed_version;
                CHECK_THROWS_AS( config.validate(), std::invalid_argument );
                CHECK_THROWS_AS( config.build_command(), std::invalid_argument );
            }
        }
}

TEST_CASE( "Ethernet configuration table size is bounded", "[eth-config]" )
{
    for( uint16_t version : { 3, 4, 5, 6 } )
    {
        CAPTURE( version );
        auto const base_size = make_table( version ).size();
        auto const at_limit = make_table( version, 512 - base_size );
        CHECK_NOTHROW( eth_config{ at_limit } );
        if( version <= 5 )
            CHECK( eth_config( at_limit ).build_command() == at_limit );
        auto const oversized = make_table( version, 513 - base_size );
        CHECK_THROWS_WITH( eth_config{ oversized }, "Eth config table size (513) exceeds limit 512" );
    }
}

TEST_CASE( "Newer Ethernet configurations expose getters but reject writes", "[eth-config]" )
{
    for( uint16_t version : { 6, 65535 } )
        for( size_t extra : { 0, 12 } )
        {
            CAPTURE( version, extra );
            debug_device debug;
            debug.current = make_table( version, extra );
            auto & table = *reinterpret_cast< eth_config_v5 * >( debug.current.data() );
            table.transmission_delay = 30;
            table.udp_ttl = 64;
            update_header( debug.current );
            auto const original = debug.current;
            auto const message = "FW eth-config v" + std::to_string( version )
                + " is newer than SDK, please update SDK";
            eth_config config( original );
            CHECK( config.header.version == version );
            CHECK( config.header.size == original.size() - sizeof( eth_config_header ) );
            CHECK( config.mac_address == "42:42:42:42:42:42" );
            CHECK_THROWS_WITH( config.build_command(), message );
            auto copy = config;
            copy.header.version = 5;  // Changing the public header cannot bypass read-only protection.
            CHECK_THROWS_WITH( copy.build_command(), message );

            eth_config_device device;
            device.init( &debug );
            CHECK( device.supports_ethernet_configuration() );
            CHECK( device.get_link_timeout() == 5000 );
            CHECK( device.get_ethernet_link_speed() == 1000 );
            CHECK( device.get_link_priority() == link_priority::eth_first );
            CHECK( device.get_dds_domain_id() == 2 );
            CHECK( device.get_mtu() == 9000 );
            CHECK( device.get_transmission_delay() == 30 );
            CHECK( device.get_udp_ttl() == 64 );
            ip_address configured, actual;
            device.get_ip_address( configured, actual );
            CHECK( configured == ip_address( "192.168.1.10" ) );
            CHECK( actual == ip_address( "10.0.0.20" ) );
            device.get_netmask( configured, actual );
            CHECK( configured == ip_address( "255.255.255.0" ) );
            device.get_gateway( configured, actual );
            CHECK( configured == ip_address() );
            bool enabled;
            uint8_t timeout;
            device.get_dhcp_config( enabled, timeout );
            CHECK( enabled );
            CHECK( timeout == 30 );
            debug.commands.clear();
            CHECK_THROWS_WITH( device.set_link_priority( link_priority::usb_first ), message );
            CHECK_THROWS_WITH( device.set_link_timeout( 6000 ), message );
            CHECK_THROWS_WITH( device.set_dds_domain_id( 40 ), message );
            CHECK_THROWS_WITH( device.set_ip_address( ip_address( "192.168.1.11" ) ), message );
            CHECK_THROWS_WITH( device.set_netmask( ip_address( "255.255.0.0" ) ), message );
            CHECK_THROWS_WITH( device.set_gateway( ip_address( "192.168.1.1" ) ), message );
            CHECK_THROWS_WITH( device.set_dhcp_config( false, 20 ), message );
            CHECK_THROWS_WITH( device.set_mtu( 7000 ), message );
            CHECK_THROWS_WITH( device.set_transmission_delay( 60 ), message );
            CHECK_THROWS_WITH( device.set_udp_ttl( 32 ), message );
            CHECK_THROWS_WITH( device.restore_defaults(), message );
            CHECK( debug.commands.empty() );
            CHECK( debug.current == original );
        }
}

TEST_CASE( "Version 3 Ethernet setters preserve reserved transmission delay bytes", "[eth-config]" )
{
    debug_device debug;
    debug.current = make_table( 3, 12 );
    eth_config_device device;
    device.init( &debug );
    auto const original = debug.current;
    debug.commands.clear();
    device.set_transmission_delay( 0 );
    CHECK( debug.commands == std::vector< uint32_t >{ 0xbb, 0xba } );
    CHECK( debug.current == original );
    debug.commands.clear();
    CHECK_THROWS_AS( device.set_transmission_delay( 30 ), std::invalid_argument );
    CHECK( debug.commands.empty() );
    CHECK( debug.current == original );
}

TEST_CASE( "Ethernet writes recheck the firmware version after GET", "[eth-config]" )
{
    debug_device debug;
    eth_config_device device;
    device.init( &debug );
    debug.current = make_table( 6, 12 );
    auto const original = debug.current;
    debug.commands.clear();
    CHECK_THROWS_WITH( device.set_link_timeout( 6000 ),
                       "FW eth-config v6 is newer than SDK, please update SDK" );
    CHECK( debug.commands == std::vector< uint32_t >{ 0xbb } );
    CHECK( debug.current == original );
    CHECK( device.get_link_timeout() == 5000 );
}
