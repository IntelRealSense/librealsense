// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-stream-base.h"

#include <string>
#include <vector>
#include <set>
#include <functional>

namespace realdds {

namespace topics {
namespace device {
    class image;
} // namespace device
} // namespace topics

class dds_subscriber;
class dds_topic_reader;

// Represents a stream of information (images, motion data, etc..) from a single source received via the DDS system.
// A stream can have several profiles, i.e different data frequency, image resolution, etc..
// 
// This is a base class: you need to specify the type of stream via the instantiation of a video_stream, etc.
//
class dds_stream : public dds_stream_base
{
    typedef dds_stream_base super;

protected:
    dds_stream( std::string const & stream_name, std::string const & sensor_name );

public:
    bool is_open() const override { return !! _reader; }
    virtual void open( std::string const & topic_name, std::shared_ptr< dds_subscriber > const & );
    virtual void close();

    bool is_streaming() const override { return _on_data_available != nullptr; }
    typedef std::function< void( topics::device::image && f) > on_data_available_callback;
    void start_streaming( on_data_available_callback cb );
    void stop_streaming();

    std::shared_ptr< dds_topic > const & get_topic() const override;

protected:
    void handle_frames();

    std::shared_ptr< dds_topic_reader > _reader;
    on_data_available_callback _on_data_available = nullptr;
};

class dds_video_stream : public dds_stream
{
    typedef dds_stream super;

public:
    dds_video_stream( std::string const & stream_name, std::string const & sensor_name );

    void set_intrinsics( const std::set< video_intrinsics > & intrinsics ) { _intrinsics = intrinsics; }
    const std::set< video_intrinsics > & get_intrinsics() const { return _intrinsics; }

private:
    class impl;
    std::shared_ptr< impl > _impl;

    std::set< video_intrinsics > _intrinsics;
};

class dds_depth_stream : public dds_video_stream
{
    typedef dds_video_stream super;

public:
    dds_depth_stream( std::string const & stream_name, std::string const & sensor_name );

    char const * type_string() const override { return "depth"; }
};

class dds_ir_stream : public dds_video_stream
{
    typedef dds_video_stream super;

public:
    dds_ir_stream( std::string const & stream_name, std::string const & sensor_name );

    char const * type_string() const override { return "ir"; }
};

class dds_color_stream : public dds_video_stream
{
    typedef dds_video_stream super;

public:
    dds_color_stream( std::string const & stream_name, std::string const & sensor_name );

    char const * type_string() const override { return "color"; }
};

class dds_fisheye_stream : public dds_video_stream
{
    typedef dds_video_stream super;

public:
    dds_fisheye_stream( std::string const & stream_name, std::string const & sensor_name );

    char const * type_string() const override { return "fisheye"; }
};

class dds_confidence_stream : public dds_video_stream
{
    typedef dds_video_stream super;

public:
    dds_confidence_stream( std::string const & stream_name, std::string const & sensor_name );

    char const * type_string() const override { return "confidence"; }
};

class dds_motion_stream : public dds_stream
{
    typedef dds_stream super;

public:
    dds_motion_stream( std::string const & stream_name, std::string const & sensor_name );

    void set_intrinsics( const motion_intrinsics & intrinsics ) { _intrinsics = intrinsics; }
    const motion_intrinsics & get_intrinsics() const { return _intrinsics; }

private:
    class impl;
    std::shared_ptr< impl > _impl;

    motion_intrinsics _intrinsics;
};

class dds_accel_stream : public dds_motion_stream
{
    typedef dds_motion_stream super;

public:
    dds_accel_stream( std::string const & stream_name, std::string const & sensor_name );

    char const * type_string() const override { return "accel"; }
};

class dds_gyro_stream : public dds_motion_stream
{
    typedef dds_motion_stream super;

public:
    dds_gyro_stream( std::string const & stream_name, std::string const & sensor_name );

    char const * type_string() const override { return "gyro"; }
};

class dds_pose_stream : public dds_motion_stream
{
    typedef dds_motion_stream super;

public:
    dds_pose_stream( std::string const & stream_name, std::string const & sensor_name );

    char const * type_string() const override { return "pose"; }
};


typedef std::vector< std::shared_ptr< dds_stream > > dds_streams;


}  // namespace realdds
