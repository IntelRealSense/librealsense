// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-stream-profile.h"
#include "dds-stream-base.h"

#include <memory>
#include <string>


namespace realdds {


class dds_topic;
class dds_topic_writer;
class dds_publisher;
class dds_stream_profile;


struct image_header
{
    int format;
    int height = 0;
    int width = 0;

    bool is_valid() const { return width != 0 && height != 0; }
};


// Distributes stream images into a dedicated topic
// 
// This is a base class: you need to specify the type of stream via the instantiation of a video_stream_server, etc.
//
class dds_stream_server : public dds_stream_base
{
protected:
    dds_stream_server( std::string const & stream_name, std::string const & sensor_name, int type = 0 );

public:
    virtual ~dds_stream_server();

    bool is_open() const override { return !! _writer; }
    virtual void open( std::string const & topic_name, std::shared_ptr< dds_publisher > const & ) = 0;

    bool is_streaming() const override { return _image_header.is_valid(); }
    void start_streaming( const image_header & header );
    void stop_streaming();

    void publish_image( const uint8_t * data, size_t size );

    std::shared_ptr< dds_topic > const & get_topic() const override;

protected:
    std::shared_ptr< dds_topic_writer > _writer;
    image_header _image_header;
};


class dds_video_stream_server : public dds_stream_server
{
    typedef dds_stream_server super;

public:
    dds_video_stream_server( std::string const & stream_name, std::string const & sensor_name, int type = 0 );

    void open( std::string const & topic_name, std::shared_ptr< dds_publisher > const & ) override;

private:
    void check_profile( std::shared_ptr< dds_stream_profile > const & ) const override;
};


class dds_motion_stream_server : public dds_stream_server
{
    typedef dds_stream_server super;

public:
    dds_motion_stream_server( std::string const & stream_name, std::string const & sensor_name, int type = 0 );
    
    void open( std::string const & topic_name, std::shared_ptr< dds_publisher > const & ) override;

private:
    void check_profile( std::shared_ptr< dds_stream_profile > const & ) const override;
};


}  // namespace realdds
