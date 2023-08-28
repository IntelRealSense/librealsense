// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/frame-continuation.h"
#include "core/frame-additional-data.h"
#include "core/extension.h"
#include "basics.h"
#include <atomic>
#include <array>
#include <vector>
#include <map>
#include <math.h>

namespace librealsense {


class stream_profile_interface;
class archive_interface;
class sensor_interface;


class frame_interface
{
public:
    virtual frame_header const & get_header() const = 0;
    virtual rs2_metadata_type get_frame_metadata( const rs2_frame_metadata_value & frame_metadata ) const = 0;
    virtual bool supports_frame_metadata( const rs2_frame_metadata_value & frame_metadata ) const = 0;
    virtual int get_frame_data_size() const = 0;
    virtual const uint8_t * get_frame_data() const = 0;
    virtual rs2_time_t get_frame_timestamp() const = 0;
    virtual rs2_timestamp_domain get_frame_timestamp_domain() const = 0;
    virtual void set_timestamp( double new_ts ) = 0;
    virtual unsigned long long get_frame_number() const = 0;

    virtual void set_timestamp_domain( rs2_timestamp_domain timestamp_domain ) = 0;
    virtual rs2_time_t get_frame_system_time() const = 0;
    virtual std::shared_ptr< stream_profile_interface > get_stream() const = 0;
    virtual void set_stream( std::shared_ptr< stream_profile_interface > sp ) = 0;

    virtual std::shared_ptr< sensor_interface > get_sensor() const = 0;
    virtual void set_sensor( std::shared_ptr< sensor_interface > ) = 0;

    virtual void acquire() = 0;
    virtual void release() = 0;
    virtual frame_interface * publish( std::shared_ptr< archive_interface > new_owner ) = 0;
    virtual void unpublish() = 0;
    virtual void attach_continuation( frame_continuation && continuation ) = 0;
    virtual void disable_continuation() = 0;
    virtual archive_interface * get_owner() const = 0;

    virtual void mark_fixed() = 0;
    virtual bool is_fixed() const = 0;
    virtual void set_blocking( bool state ) = 0;
    virtual bool is_blocking() const = 0;

    virtual void keep() = 0;

    virtual ~frame_interface() = default;
};


struct LRS_EXTENSION_API frame_holder
{
    frame_interface * frame = nullptr;

    frame_holder() = default;
    frame_holder( const frame_holder & other ) = delete;
    frame_holder( frame_holder && other ) { std::swap( frame, other.frame ); }
    // non-acquiring ctor: will assume the frame has already been acquired!
    frame_holder( frame_interface * const f ) { frame = f; }
    ~frame_holder() { reset(); }

    // return a new holder after acquiring the frame
    frame_holder clone() const { return acquire( frame ); }
    static frame_holder acquire( frame_interface * const f ) { if( f ) f->acquire(); return frame_holder( f ); }

    operator frame_interface *() const { return frame; }
    frame_interface * operator->() const { return frame; }
    operator bool() const { return frame != nullptr; }

    frame_holder & operator=( const frame_holder & other ) = delete;
    frame_holder & operator=( frame_holder && other )
    {
        reset();
        std::swap( frame, other.frame );
        return *this;
    }

    void reset()
    {
        if( frame )
        {
            frame->release();
            frame = nullptr;
        }
    }
};


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
    rs2_metadata_type get_frame_metadata( const rs2_frame_metadata_value & frame_metadata ) const override;
    bool supports_frame_metadata( const rs2_frame_metadata_value & frame_metadata ) const override;
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
    void set_sensor( std::shared_ptr< sensor_interface > s ) override;

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
