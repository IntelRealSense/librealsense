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
#include <mutex>


namespace librealsense {


// Watches registered options value and notifies interested users.
// When a user subscribes to notification the options_watcher will automatically update (query) registered options
// values in set time intervals (creates a thread). If one or more of the values have changed the watcher will notify
// through the callback subscription.
class options_watcher
{
public:
    using callback = std::function< void( const std::map< rs2_option, std::shared_ptr< option > > & ) >;

    options_watcher( std::chrono::milliseconds update_interval = std::chrono::milliseconds( 1000 ) );
    ~options_watcher();

    void register_option( rs2_option id, std::shared_ptr< option > option );
    void unregister_option( rs2_option id );

    rsutils::subscription subscribe( callback && cb);

    void set_update_interval( std::chrono::milliseconds update_interval ) { _update_interval = update_interval; }

private:
    bool should_start() const;
    bool should_stop() const;
    void start();
    void stop();
    void thread_loop();
    std::map< rs2_option, std::shared_ptr< option > > update_options();
    void notify( const std::map< rs2_option, std::shared_ptr< option > > & updated_options );

    std::map< rs2_option, std::shared_ptr< option > > _options;
    std::map< rs2_option, float > _option_values;
    rsutils::signal< const std::map< rs2_option, std::shared_ptr< option > > & > _on_values_changed;
    std::chrono::milliseconds _update_interval;
    std::thread _updater;
    std::mutex _mutex;
};


}  // namespace librealsense
