// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "depth-sensor.h"
#include "core/extension.h"
#include <atomic>
#include <array>
#include <vector>
#include <map>
#include <math.h>

namespace librealsense {


class sensor_interface;
class archive_interface;
class md_attribute_parser_base;
class depth_sensor;


/** \brief Metadata fields that are utilized internally by librealsense
Provides extention to the r2_frame_metadata list of attributes*/
enum frame_metadata_internal
{
    RS2_FRAME_METADATA_HW_TYPE = RS2_FRAME_METADATA_COUNT + 1, /**< 8-bit Module type: RS4xx, IVCAM*/
    RS2_FRAME_METADATA_SKU_ID, /**< 8-bit SKU Id*/
    RS2_FRAME_METADATA_FORMAT, /**< 16-bit Frame format*/
    RS2_FRAME_METADATA_WIDTH, /**< 16-bit Frame width. pixels*/
    RS2_FRAME_METADATA_HEIGHT, /**< 16-bit Frame height. pixels*/
    RS2_FRAME_METADATA_ACTUAL_COUNT
};


// multimap is necessary here in order to permit registration to some metadata value in multiple
// places in metadata as it is required for D405, in which exposure should be available from the
// same sensor both for depth and color frames
typedef std::multimap< rs2_frame_metadata_value, std::shared_ptr< md_attribute_parser_base > >
    metadata_parser_map;

#pragma pack( push, 1 )
struct metadata_array_value
{
    bool is_valid;
    rs2_metadata_type value;
};
#pragma pack( pop )

typedef std::array< metadata_array_value, RS2_FRAME_METADATA_ACTUAL_COUNT > metadata_array;

static_assert( sizeof( metadata_array_value ) == sizeof( rs2_metadata_type ) + 1,
               "unexpected size for metadata array members" );


/*
    Each frame is attached with a static header
    This is a quick and dirty way to manage things like timestamp,
    frame-number, metadata, etc... Things shared between all frame extensions
    The point of this class is to be **fixed-sized**, avoiding per frame allocations
*/
struct frame_header
{
    unsigned long long   frame_number = 0;
    rs2_time_t           timestamp = 0;
    rs2_timestamp_domain timestamp_domain = RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;
    rs2_time_t           system_time = 0;       // sys-clock at the time the frame was received from the backend
    rs2_time_t           backend_timestamp = 0; // time when the frame arrived to the backend (OS dependent)

    frame_header() = default;
    frame_header( frame_header const& ) = default;
    frame_header( rs2_time_t in_timestamp,
        unsigned long long in_frame_number,
        rs2_time_t in_system_time,
        rs2_time_t backend_time )
        : timestamp( in_timestamp )
        , frame_number( in_frame_number )
        , system_time( in_system_time )
        , backend_timestamp( backend_time )
    {
    }
};

std::ostream & operator<<( std::ostream & os, frame_header const & header );

struct frame_additional_data : frame_header
{
    uint32_t metadata_size = 0;
    bool fisheye_ae_mode = false;  // TODO: remove in future release
    std::array< uint8_t, sizeof( metadata_array ) > metadata_blob = {};
    rs2_time_t last_timestamp = 0;
    unsigned long long last_frame_number = 0;
    bool is_blocking  = false;  // when running from recording, this bit indicates
                                // if the recorder was configured to realtime mode or not
                                // if true, this will force any queue receiving this frame not to drop it

    float depth_units = 0.0f;  // adding depth units to frame metadata is a temporary solution, it
                               // will be replaced by FW metadata

    uint32_t raw_size = 0;     // The frame transmitted size (payload only)

    frame_additional_data() {}

    frame_additional_data( metadata_array const & metadata )
    {
        metadata_size = (uint32_t) sizeof( metadata );
        memcpy( metadata_blob.data(), metadata.data(), metadata_size );
    }

    frame_additional_data( rs2_time_t in_timestamp,
                           unsigned long long in_frame_number,
                           rs2_time_t in_system_time,
                           uint32_t md_size,
                           const uint8_t * md_buf,
                           rs2_time_t backend_time,
                           rs2_time_t last_timestamp,
                           unsigned long long last_frame_number,
                           bool in_is_blocking,
                           float in_depth_units = 0,
                           uint32_t transmitted_size = 0 )
        : frame_header( in_timestamp, in_frame_number, in_system_time, backend_time )
        , metadata_size( md_size )
        , last_timestamp( last_timestamp )
        , last_frame_number( last_frame_number )
        , is_blocking( in_is_blocking )
        , depth_units( in_depth_units )
        , raw_size( transmitted_size )
    {
        if( metadata_size )
            std::copy( md_buf, md_buf + std::min( size_t( md_size ), metadata_blob.size() ), metadata_blob.begin() );
    }
};

class sensor_part
{
public:
    virtual std::shared_ptr< sensor_interface > get_sensor() const = 0;
    virtual void set_sensor( std::shared_ptr< sensor_interface > s ) = 0;
    virtual ~sensor_part() = default;
};


class stream_profile_interface;
class archive_interface;

class frame_interface : public sensor_part
{
public:
    virtual frame_header const & get_header() const = 0;
    virtual rs2_metadata_type get_frame_metadata( const rs2_frame_metadata_value & frame_metadata ) const = 0;
    virtual bool supports_frame_metadata( const rs2_frame_metadata_value & frame_metadata ) const = 0;
    virtual int get_frame_data_size() const = 0;
    virtual const byte * get_frame_data() const = 0;
    virtual rs2_time_t get_frame_timestamp() const = 0;
    virtual rs2_timestamp_domain get_frame_timestamp_domain() const = 0;
    virtual void set_timestamp( double new_ts ) = 0;
    virtual unsigned long long get_frame_number() const = 0;

    virtual void set_timestamp_domain( rs2_timestamp_domain timestamp_domain ) = 0;
    virtual rs2_time_t get_frame_system_time() const = 0;
    virtual std::shared_ptr< stream_profile_interface > get_stream() const = 0;
    virtual void set_stream( std::shared_ptr< stream_profile_interface > sp ) = 0;

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
    std::vector< byte > data;
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
    const byte * get_frame_data() const override;
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
