// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "info-interface.h"
#include "options-container.h"
#include "recommended-processing-blocks-interface.h"

#include "tagged-profile.h"

#include <src/core/options-watcher.h>
#include <librealsense2/hpp/rs_types.hpp>

#include <rsutils/subscription.h>

#include <vector>
#include <memory>


namespace librealsense {


class stream_profile_interface;
class device_interface;


class sensor_interface
    : public virtual info_interface
    , public virtual options_interface
    , public virtual recommended_proccesing_blocks_interface
{
public:
    virtual ~sensor_interface() = default;

    virtual device_interface & get_device() = 0;

    using stream_profiles = std::vector< std::shared_ptr< stream_profile_interface > >;

    virtual stream_profiles get_stream_profiles( int tag = profile_tag::PROFILE_TAG_ANY ) const = 0;
    virtual stream_profiles get_active_streams() const = 0;
    virtual stream_profiles const & get_raw_stream_profiles() const = 0;

    virtual void open( const stream_profiles & requests ) = 0;
    virtual void start( rs2_frame_callback_sptr callback ) = 0;
    virtual void stop() = 0;
    virtual void close() = 0;

    virtual bool is_streaming() const = 0;

    virtual rs2_notifications_callback_sptr get_notifications_callback() const = 0;
    virtual void register_notifications_callback( rs2_notifications_callback_sptr callback ) = 0;

    virtual int register_before_streaming_changes_callback( std::function< void( bool ) > callback ) = 0;
    virtual void unregister_before_start_callback( int token ) = 0;

    virtual rs2_frame_callback_sptr get_frames_callback() const = 0;
    virtual void set_frames_callback( rs2_frame_callback_sptr cb ) = 0;

    virtual rsutils::subscription register_options_changed_callback( options_watcher::callback && cb ) = 0;
};


}  // namespace librealsense
