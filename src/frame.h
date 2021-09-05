// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "depth-sensor.h"
#include "core/extension.h"
#include <atomic>
#include <array>
#include <math.h>

namespace librealsense {
class sensor_interface;
class archive_interface;
class md_attribute_parser_base;
class depth_sensor;


// multimap is necessary here in order to permit registration to some metadata value in multiple
// places in metadata as it is required for D405, in which exposure should be available from the
// same sensor both for depth and color frames
typedef std::multimap< rs2_frame_metadata_value, std::shared_ptr< md_attribute_parser_base > >
    metadata_parser_map;

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
    std::array< uint8_t, MAX_META_DATA_SIZE > metadata_blob;
    rs2_time_t last_timestamp = 0;
    unsigned long long last_frame_number = 0;
    bool is_blocking  = false;  // when running from recording, this bit indicates
                                // if the recorder was configured to realtime mode or not
                                // if true, this will force any queue receiving this frame not to drop it

    float depth_units = 0.0f;  // adding depth units to frame metadata is a temporary solution, it
                               // will be replaced by FW metadata

    uint32_t raw_size = 0;     // The frame transmitted size (payload only)

    frame_additional_data() {}

    frame_additional_data( rs2_time_t in_timestamp,
                           unsigned long long in_frame_number,
                           rs2_time_t in_system_time,
                           uint8_t md_size,
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
        // Copy up to 255 bytes to preserve metadata as raw data
        if( metadata_size )
            std::copy( md_buf, md_buf + std::min( md_size, MAX_META_DATA_SIZE ), metadata_blob.begin() );
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

class video_frame : public frame
{
public:
    video_frame()
        : frame()
        , _width( 0 )
        , _height( 0 )
        , _bpp( 0 )
        , _stride( 0 )
    {
    }

    int get_width() const { return _width; }
    int get_height() const { return _height; }
    int get_stride() const { return _stride; }
    int get_bpp() const { return _bpp; }

    void assign( int width, int height, int stride, int bpp )
    {
        _width = width;
        _height = height;
        _stride = stride;
        _bpp = bpp;
    }

private:
    int _width, _height, _bpp, _stride;
};

MAP_EXTENSION( RS2_EXTENSION_VIDEO_FRAME, librealsense::video_frame );

class depth_frame : public video_frame
{
public:
    depth_frame()
        : video_frame()
    {
    }

    frame_interface * publish( std::shared_ptr< archive_interface > new_owner ) override
    {
        return video_frame::publish( new_owner );
    }

    void keep() override
    {
        if( _original )
            _original->keep();
        video_frame::keep();
    }

    float get_distance( int x, int y ) const;

    float get_units() const { return additional_data.depth_units; }

    void set_original( frame_holder h )
    {
        _original = std::move( h );
        attach_continuation( frame_continuation(
            [this]() {
                if( _original )
                {
                    _original = {};
                }
            },
            nullptr ) );
    }

protected:
    static float query_units( const std::shared_ptr< sensor_interface > & sensor )
    {
        if( sensor != nullptr )
        {
            try
            {
                auto depth_sensor = As< librealsense::depth_sensor >( sensor );
                if( depth_sensor != nullptr )
                {
                    return depth_sensor->get_depth_scale();
                }
                else
                {
                    // For playback sensors
                    auto extendable = As< librealsense::extendable_interface >( sensor );
                    if( extendable
                        && extendable->extend_to(
                            TypeToExtension< librealsense::depth_sensor >::value,
                            (void **)( &depth_sensor ) ) )
                    {
                        return depth_sensor->get_depth_scale();
                    }
                }
            }
            catch( const std::exception & e )
            {
                LOG_ERROR( "Failed to query depth units from sensor. " << e.what() );
            }
            catch( ... )
            {
                LOG_ERROR( "Failed to query depth units from sensor" );
            }
        }
        else
        {
            LOG_WARNING( "sensor was nullptr" );
        }

        return 0;
    }

    frame_holder _original;
};

MAP_EXTENSION( RS2_EXTENSION_DEPTH_FRAME, librealsense::depth_frame );

// Disparity frame provides an alternative representation of the depth data for stereo-based depth
// sensors In addition for the depth frame API it allows to query the stereoscopic baseline required
// to transform depth to disparity and vice versa
class disparity_frame : public depth_frame
{
public:
    disparity_frame()
        : depth_frame()
    {
    }

    // TODO Refactor to framemetadata
    float get_stereo_baseline( void ) const { return query_stereo_baseline( this->get_sensor() ); }

protected:
    static float query_stereo_baseline( const std::shared_ptr< sensor_interface > & sensor )
    {
        if( sensor != nullptr )
        {
            try
            {
                auto stereo_sensor = As< librealsense::depth_stereo_sensor >( sensor );
                if( stereo_sensor != nullptr )
                {
                    return stereo_sensor->get_stereo_baseline_mm();
                }
                else
                {
                    // For playback sensors
                    auto extendable = As< librealsense::extendable_interface >( sensor );
                    if( extendable
                        && extendable->extend_to(
                            TypeToExtension< librealsense::depth_stereo_sensor >::value,
                            (void **)( &stereo_sensor ) ) )
                    {
                        return stereo_sensor->get_stereo_baseline_mm();
                    }
                }
            }
            catch( const std::exception & e )
            {
                LOG_ERROR( "Failed to query stereo baseline from sensor. " << e.what() );
            }
            catch( ... )
            {
                LOG_ERROR( "Failed to query stereo baseline from sensor" );
            }
        }
        else
        {
            LOG_WARNING( "sensor was nullptr" );
        }

        return 0;
    }
};

MAP_EXTENSION( RS2_EXTENSION_DISPARITY_FRAME, librealsense::disparity_frame );

class motion_frame : public frame
{
public:
    motion_frame()
        : frame()
    {
    }
};

MAP_EXTENSION( RS2_EXTENSION_MOTION_FRAME, librealsense::motion_frame );

class pose_frame : public frame
{
public:
    // pose frame data buffer is pose info struct
    struct pose_info
    {
        float3 translation;  /**< X, Y, Z values of translation, in meters (relative to initial
                                position)                                    */
        float3 velocity;     /**< X, Y, Z values of velocity, in meter/sec     */
        float3 acceleration; /**< X, Y, Z values of acceleration, in meter/sec^2 */
        float4 rotation;     /**< Qi, Qj, Qk, Qr components of rotation as represented in quaternion
                                rotation (relative to initial position) */
        float3 angular_velocity; /**< X, Y, Z values of angular velocity, in radians/sec */
        float3
            angular_acceleration; /**< X, Y, Z values of angular acceleration, in radians/sec^2 */
        uint32_t tracker_confidence; /**< pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 -
                                        Medium, 0x3 - High                                     */
        uint32_t mapper_confidence; /**< pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium,
                                       0x3 - High                                     */
    };

    pose_frame()
        : frame()
    {
    }

    float3 get_translation() const
    {
        return reinterpret_cast< const pose_info * >( get_frame_data() )->translation;
    }
    float3 get_velocity() const
    {
        return reinterpret_cast< const pose_info * >( get_frame_data() )->velocity;
    }
    float3 get_acceleration() const
    {
        return reinterpret_cast< const pose_info * >( get_frame_data() )->acceleration;
    }
    float4 get_rotation() const
    {
        return reinterpret_cast< const pose_info * >( get_frame_data() )->rotation;
    }
    float3 get_angular_velocity() const
    {
        return reinterpret_cast< const pose_info * >( get_frame_data() )->angular_velocity;
    }
    float3 get_angular_acceleration() const
    {
        return reinterpret_cast< const pose_info * >( get_frame_data() )->angular_acceleration;
    }
    uint32_t get_tracker_confidence() const
    {
        return reinterpret_cast< const pose_info * >( get_frame_data() )->tracker_confidence;
    }
    uint32_t get_mapper_confidence() const
    {
        return reinterpret_cast< const pose_info * >( get_frame_data() )->mapper_confidence;
    }
};

MAP_EXTENSION( RS2_EXTENSION_POSE_FRAME, librealsense::pose_frame );
}  // namespace librealsense
