// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once


#include <librealsense2/h/rs_option.h>
#include <src/core/option-interface.h>

#include <rsutils/signal.h>
#include <rsutils/concurrency/concurrency.h>

#include <map>
#include <chrono>
#include <functional>
#include <memory>


namespace librealsense {


class options_updater
{
public:
    options_updater( std::chrono::milliseconds update_interval = std::chrono::milliseconds( 1000 ) );
    ~options_updater();

    void register_option( rs2_option id, std::shared_ptr< option > option );
    void unregister_option( rs2_option id );

    rsutils::subscription subscribe_to_updates( std::function< void( const rs2_options_list * ) > && callback );
    rsutils::subscription_slot add_callback( std::function< void( const rs2_options_list * ) > && callback );
    bool remove_callback( rsutils::subscription_slot callback_id );

private:
    void update_loop( dispatcher::cancellable_timer cancellable_timer );

    std::map< rs2_option, std::shared_ptr< option > > _options;
    std::map< rs2_option, float > _option_values;
    rsutils::signal< const rs2_options_list * > _on_values_changed;
    std::chrono::milliseconds _update_interval;
    active_object<> _updater;
};


}  // namespace librealsense
