// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#include "options-watcher.h"
#include <proc/synthetic-stream.h>


namespace librealsense {


options_watcher::options_watcher( std::chrono::milliseconds update_interval )
    : _update_interval( update_interval )
    , _updater()
{
}

options_watcher::~options_watcher()
{
    stop();
}

void options_watcher::register_option( rs2_option id, std::shared_ptr< option > option, bool update_from_hw )
{
    _options[id] = option;
    _update_from_hw[id] = update_from_hw;
    _option_values[id] = update_from_hw ? option->query() : option->query(); // TODO - when implemented use option->get_last_known_value();

    if( should_start() )
        start();
}

void options_watcher::unregister_option( rs2_option id )
{
    _options.erase( id );

    if( should_stop() )
        stop();
}

rsutils::subscription
options_watcher::subscribe( std::function< void( const std::map< rs2_option, std::shared_ptr< option > > & ) > && callback )
{
    rsutils::subscription ret = _on_values_changed.subscribe( std::move( callback ) );

    if( should_start() )
        start();

    return ret;
}

rsutils::subscription_slot options_watcher::add_callback(
    std::function< void( const std::map< rs2_option, std::shared_ptr< option > > & ) > && callback )
{
    rsutils::subscription_slot ret = _on_values_changed.add( std::move( callback ) );

    if( should_start() )
        start();

    return ret;
}

bool options_watcher::remove_callback( rsutils::subscription_slot callback_id )
{
    bool ret = _on_values_changed.remove( callback_id );

    if( should_stop() )
        stop();

    return ret;
}

bool options_watcher::should_start() const
{
    return ! should_stop();
}

bool options_watcher::should_stop() const
{
    return _on_values_changed.size() == 0 || _options.size() == 0;
}

void options_watcher::start()
{
    if( ! _updater.joinable() ) // If not already started
    {
        _updater = std::thread( [this]() { update_options_and_notify(); } );
    }
}

void options_watcher::stop()
{
    if( _updater.joinable() )
        _updater.join();
}

void options_watcher::update_options_and_notify()
{
    // Checking should_stop because subscriptions can be canceled without us knowing
    while( ! should_stop() )
    {
        std::map< rs2_option, std::shared_ptr< option > > updated_options;

        for( auto & option : _options )
        {
            if( ! _update_from_hw[option.first] )
                continue;

            auto curr_val = option.second->query();

            // TODO - When option->get_last_known_value() is implemented use line below instead of lines above 
            //auto curr_val = _update_from_hw[option.first] ? option.second->query()
            //                                              : option.second->get_last_known_value();

            if( _option_values[option.first] != curr_val )
            {
                _option_values[option.first] = curr_val;
                updated_options.insert( option );
            }
        }

        if( ! updated_options.empty() )
            _on_values_changed.raise( updated_options );

        // Checking again for stop conditions after callbacks but before sleep.
        if(  should_stop() )
            break;

        std::this_thread::sleep_for( _update_interval );
    }
}


}  // namespace librealsense