// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "options-registry.h"
#include "basics.h"
#include <src/librealsense-exception.h>
#include <vector>
#include <mutex>
#include <map>


namespace librealsense {


LRS_EXTENSION_API std::string const & get_string( rs2_option );


namespace by_name {


static std::map< std::string, rs2_option > _option_by_name = []()
{
    // Automatically add all our known options into the option names -- that way, no duplicate custom option
    // can be added...
    std::map< std::string, rs2_option > map;
    for( auto option = rs2_option( 0 ); option < RS2_OPTION_COUNT; option = rs2_option( option + 1 ) )
        map[get_string( option )] = option;
    return std::move( map );
}();
static std::vector< std::string > _name_by_index;
static std::mutex _mutex;


inline size_t option_to_index( rs2_option by_name_option )
{
    // Registered rs2_option value will be negative, so we negate
    // Indices are 0-based but negative rs2_options values are 1-based, so minus 1:
    return -by_name_option - 1;
}


inline rs2_option index_to_option( size_t index )
{
    // Index = index into the _name_by_index array -> 0-based
    // Registered rs2_option value will be negative, and starting at -1:
    return rs2_option( -( int( index ) + 1 ) );
}


inline size_t new_index()
{
    // _name_by_index and _option_by_name do not have the same size:
    // _option_by_name is also populated with non-registered options names!
    auto const index = _name_by_index.size();
    // We have to return strings by reference, which means we cannot control access simply with a mutex!
    // The mutex will still be able to control registration, but otherwise we want to ensure that the names stay
    // inviolable after registered, so the name-by-index vector cannot be allowed to grow beyond a certain limit.
    if( index >= 1000 )
        throw std::runtime_error( "reached option limit of 1000" );
    _name_by_index.reserve( 1000 );
    return index;
}


}  // namespace by_name


rs2_option options_registry::register_option_by_name( std::string const & option_name, bool ok_if_there )
{
    if( option_name.empty() )
        throw invalid_value_exception( "cannot register an empty option name" );

    std::lock_guard< std::mutex > lock( by_name::_mutex );
    auto const index = by_name::new_index();
    auto const option = by_name::index_to_option( index );
    auto it_new = by_name::_option_by_name.emplace( option_name, option );
    if( ! it_new.second )
    {
        if( ! ok_if_there )
            throw invalid_value_exception( "option '" + option_name + "' was already registered" );
        return it_new.first->second;
    }
    by_name::_name_by_index.push_back( option_name );
    LOG_DEBUG( "        new option registered: '" << option_name << "' = " << int( option ) );
    return option;
}


rs2_option options_registry::find_option_by_name( std::string const & option_name )
{
    // Both regular and by-name options are supported
    std::lock_guard< std::mutex > lock( by_name::_mutex );
    auto it = by_name::_option_by_name.find( option_name );
    if( it == by_name::_option_by_name.end() )
        return RS2_OPTION_COUNT;
    return it->second;
}


bool options_registry::is_option_registered( rs2_option const option )
{
    if( option >= 0 )
        return false;

    auto const index = by_name::option_to_index( option );
    if( index >= by_name::_name_by_index.size() )
        return false;

    return true;
}


std::string const & options_registry::get_registered_option_name( rs2_option const option )
{
    if( option >= 0 )
        throw invalid_value_exception( "option " + std::to_string( option ) + " is not registered" );

    auto const index = by_name::option_to_index( option );
    if( index >= by_name::_name_by_index.size() )
        throw invalid_value_exception( "bad option " + std::to_string( option ) );

    return by_name::_name_by_index[index];
}


}  // namespace librealsense
