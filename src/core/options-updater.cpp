// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#include "options-updater.h"
#include <proc/synthetic-stream.h>


namespace librealsense {


options_updater::options_updater( std::chrono::milliseconds update_interval )
    : _update_interval( update_interval ),
    _updater([this](dispatcher::cancellable_timer cancellable_timer) { update_loop(cancellable_timer); })
{
}

options_updater::~options_updater()
{
    _updater.stop();
}

void options_updater::register_option( rs2_option id, std::shared_ptr< option > option )
{
    _options[id] = option;
    _option_values[id] = option->query();

    if( ! _updater.is_active() )
        _updater.start();
}

void options_updater::unregister_option( rs2_option id )
{
    _options.erase( id );

    if( _options.empty() )
        _updater.stop();
}

rsutils::subscription options_updater::subscribe_to_updates( std::function< void( const rs2_options_list * ) > && callback )
{
    return _on_values_changed.subscribe( std::move( callback ) );
}

rsutils::subscription_slot options_updater::add_callback( std::function< void( const rs2_options_list * ) > && callback )
{
    return _on_values_changed.add( std::move( callback ) );
}

bool options_updater::remove_callback( rsutils::subscription_slot callback_id )
{
    return _on_values_changed.remove( callback_id );
}

void options_updater::update_loop( dispatcher::cancellable_timer cancellable_timer )
{
    if( cancellable_timer.try_sleep( std::chrono::milliseconds( _update_interval ) ) )
    {
        rs2_options_list list;

        for( auto & option : _options )
        {
            auto curr_val = option.second->query();
            if( _option_values[option.first] != curr_val )
            {
                _option_values[option.first] = curr_val;
                list.list.push_back( option.first );
            }
        }

        if( ! list.list.empty() )
            _on_values_changed.raise( &list );
    }
}


}  // namespace librealsense