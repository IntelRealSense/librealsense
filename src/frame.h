// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/frame-interface.h"
#include "core/frame-continuation.h"
#include "core/frame-additional-data.h"
#include "basics.h"
#include <atomic>
#include <vector>
#include <memory>


namespace librealsense {


// Define a movable but explicitly noncopyable buffer type to hold our frame data
class LRS_EXTENSION_API frame : public frame_interface
{
public:
    std::vector< uint8_t > data;
    frame_additional_data additional_data;
    std::shared_ptr< metadata_parser_map > metadata_parsers = nullptr;
    
    explicit frame()
        : ref_count( 0 )
        , owner( nullptr )
        , on_release()
        , _kept( false )
    {
    }
    frame( const frame & r ) = delete;
    frame( frame && r );

    frame & operator=( const frame & r ) = delete;
    frame & operator=( frame && r );

    virtual ~frame() { on_release.reset(); }
    frame_header const & get_header() const override { return additional_data; }
    bool find_metadata( rs2_frame_metadata_value, rs2_metadata_type * p_output_value ) const override;
    int get_frame_data_size() const override;
    const uint8_t * get_frame_data() const override;
    rs2_time_t get_frame_timestamp() const override;
    rs2_timestamp_domain get_frame_timestamp_domain() const override;
    void set_timestamp( double new_ts ) override { additional_data.timestamp = new_ts; }
    unsigned long long get_frame_number() const override;
    void set_timestamp_domain( rs2_timestamp_domain timestamp_domain ) override
    {
        additional_data.timestamp_domain = timestamp_domain;
    }

    // Return FPS calculated as (1000*d_frames/d_timestamp), or 0 if this cannot be estimated
    double calc_actual_fps() const;

    rs2_time_t get_frame_system_time() const override;

    std::shared_ptr< stream_profile_interface > get_stream() const override { return stream; }
    void set_stream( std::shared_ptr< stream_profile_interface > sp ) override
    {
        stream = std::move( sp );
    }

    void acquire() override { ref_count.fetch_add( 1 ); }
    void release() override;
    void keep() override;

    frame_interface * publish( std::shared_ptr< archive_interface > new_owner ) override;
    void unpublish() override {}
    void attach_continuation( frame_continuation && continuation ) override
    {
        on_release = std::move( continuation );
    }
    void disable_continuation() override { on_release.reset(); }

    archive_interface * get_owner() const override;

    std::shared_ptr< sensor_interface > get_sensor() const override;
    void set_sensor( std::shared_ptr< sensor_interface > ) override;

    void mark_fixed() override { _fixed = true; }
    bool is_fixed() const override { return _fixed; }

    void set_blocking( bool state ) override { additional_data.is_blocking = state; }
    bool is_blocking() const override { return additional_data.is_blocking; }

private:
    // TODO: check boost::intrusive_ptr or an alternative
    std::atomic< int > ref_count;  // the reference count is on how many times this placeholder has
                                   // been observed (not lifetime, not content)
    std::shared_ptr< archive_interface > owner;  // pointer to the owner to be returned to by last observe
    std::weak_ptr< sensor_interface > sensor;
    frame_continuation on_release;
    bool _fixed = false;
    std::atomic_bool _kept;
    std::shared_ptr< stream_profile_interface > stream;
};


}  // namespace librealsense
