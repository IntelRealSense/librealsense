// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "options-container.h"
#include "enum-helpers.h"
#include <src/librealsense-exception.h>
#include <rsutils/string/from.h>


namespace librealsense {


const option & options_container::get_option( rs2_option id ) const
{
    auto it = _options.find( id );
    if( it == _options.end() )
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
    for( auto && opt : ctr->_options )
    {
        _options[opt.first] = opt.second;
    }
}


std::vector< rs2_option > options_container::get_supported_options() const
{
    std::vector< rs2_option > options;
    for( auto option : _options )
        options.push_back( option.first );

    return options;
}


std::string const & options_container::get_option_name( rs2_option option ) const
{
    return get_string( option );
}


}  // namespace librealsense