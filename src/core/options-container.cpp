// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023-4 Intel Corporation. All Rights Reserved.

#include "options-container.h"
#include "enum-helpers.h"
#include <src/librealsense-exception.h>
#include <rsutils/string/from.h>


namespace librealsense {


const option & options_container::get_option( rs2_option id ) const
{
    auto it = _options_by_id.find( id );
    if( it == _options_by_id.end() )
    {
        throw invalid_value_exception( rsutils::string::from()
                                       << "option '" << get_option_name( id ) << "' not supported" );
    }
    return *it->second;
}


void options_container::update( std::shared_ptr< extension_snapshot > ext )
{
    auto ctr = As< options_container >( ext );
    if( ! ctr )
        return;
    for( auto id : ctr->_ordered_options )
    {
        auto & opt = _options_by_id[id];
        if( ! opt )
            _ordered_options.push_back( id );
        opt = ctr->_options_by_id[id];
    }
}


void options_container::register_option( rs2_option id, std::shared_ptr< option > option )
{
    auto & opt = _options_by_id[id];
    if( ! opt )
        _ordered_options.push_back( id );
    opt = option;
    _recording_function( *this );
}


void options_container::unregister_option( rs2_option id )
{
    for( auto it = _ordered_options.begin(); it != _ordered_options.end(); ++it )
    {
        if( *it == id )
        {
            _ordered_options.erase( it );
            break;
        }
    }
    _options_by_id.erase( id );
}


std::vector< rs2_option > options_container::get_supported_options() const
{
    // Return options ordered by their ID values! This means custom options will be first!
    // Kept for backwards compatibility with existing devices; software sensors will return the _ordered_options

    std::vector< rs2_option > options;
    for( auto option : _options_by_id )
        options.push_back( option.first );

    return options;
}


std::string const & options_container::get_option_name( rs2_option option ) const
{
    return get_string( option );
}


}  // namespace librealsense