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
    try
    {
        std::lock_guard< std::mutex > lock( _mutex );
        _options.clear();
        stop();
    }
    catch( ... )
    {
        LOG_DEBUG( "Failures taking lock or stoping thread while destructing options_watcher" );
    }
}

void options_watcher::register_option( rs2_option id, std::shared_ptr< option > option )
{
    {
        std::lock_guard< std::mutex > lock( _mutex );
        _options[id] = option;
        _option_values[id] = option->query();
    }

    if( should_start() )
        start();
}

void options_watcher::unregister_option( rs2_option id )
{
    {
        std::lock_guard< std::mutex > lock( _mutex );
        _options.erase( id );
    }

    if( should_stop() )
        stop();
}

rsutils::subscription options_watcher::subscribe( callback && cb )
{
    rsutils::subscription ret = _on_values_changed.subscribe( std::move( cb ) );

    if( should_start() )
        start();

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
        _updater = std::thread( [this]() { thread_loop(); } );
    }
}

void options_watcher::stop()
{
    if( _updater.joinable() )
        _updater.join();
}

void options_watcher::thread_loop()
{
    // Checking should_stop because subscriptions can be canceled without us knowing
    while( ! should_stop() )
    {
        std::map< rs2_option, std::shared_ptr< option > > updated_options = update_options();
        notify( updated_options );

        // Checking again for stop conditions after callbacks but before sleep.
        if(  should_stop() )
            break;

        std::this_thread::sleep_for( _update_interval );
    }
}

std::map< rs2_option, std::shared_ptr< option > > options_watcher::update_options()
{
    std::map< rs2_option, std::shared_ptr< option > > updated_options;

    {
        std::lock_guard< std::mutex > lock( _mutex );

        for( const auto & option : _options )
        {
            auto curr_val = option.second->query();

            if( _option_values[option.first] != curr_val )
            {
                _option_values[option.first] = curr_val;
                updated_options.insert( option );
            }
        }
    }

    return updated_options;
}

void options_watcher::notify( const std::map< rs2_option, std::shared_ptr< option > > & updated_options )
{
    if( ! updated_options.empty() )
        _on_values_changed.raise( updated_options );
}

}  // namespace librealsense