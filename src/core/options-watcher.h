// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once


#include <librealsense2/h/rs_option.h>
#include <src/core/option-interface.h>

#include <rsutils/signal.h>
#include <rsutils/concurrency/concurrency.h>

#include <map>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>


namespace librealsense {


// Automatically updates (queries) registered options values according to set time intervals
// Can notify if a value have changed through a callback subscription.
class options_watcher
{
public:
    options_watcher( std::chrono::milliseconds update_interval = std::chrono::milliseconds( 1000 ) );
    ~options_watcher();

    // Register option to monitor value updates. Can set if update from HW (query) is necessary.
    void register_option( rs2_option id, std::shared_ptr< option > option, bool update_from_hw = false );
    void unregister_option( rs2_option id );

    rsutils::subscription subscribe( std::function< void( const std::map< rs2_option, std::shared_ptr< option > > & ) > && callback );
    rsutils::subscription_slot add_callback( std::function< void( const std::map< rs2_option, std::shared_ptr< option > > & ) > && callback );
    bool remove_callback( rsutils::subscription_slot callback_id );

private:
    bool should_start() const;
    bool should_stop() const;
    void start();
    void stop();
    void update_options_and_notify();

    std::map< rs2_option, std::shared_ptr< option > > _options;
    std::map< rs2_option, float > _option_values;
    std::map< rs2_option, float > _update_from_hw;
    rsutils::signal< const std::map< rs2_option, std::shared_ptr< option > > & > _on_values_changed;
    std::chrono::milliseconds _update_interval;
    std::thread _updater;
    bool _should_update = false;
};


}  // namespace librealsense
