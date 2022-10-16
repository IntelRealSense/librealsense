// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-stream-profile.h"

#include <memory>
#include <string>
#include <vector>


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


typedef std::vector< std::shared_ptr< dds_stream_profile > > dds_stream_profiles;


// Distributes stream images into a dedicated topic
// 
// This is a base class: you need to specify the type of stream via the instantiation of a video_stream_server, etc.
//
class dds_stream_server
{
    std::string const _name;
    int const _default_profile_index;
    dds_stream_profiles const _profiles;

protected:
    dds_stream_server( std::string const & name, dds_stream_profiles const & profiles, int default_profile_index );

public:
    virtual ~dds_stream_server();

    std::string const & name() const { return _name; }
    dds_stream_profiles const & profiles() const { return _profiles; }
    int default_profile_index() const { return _default_profile_index; }

    bool is_open() const { return !! _writer; }
    virtual void open( std::string const & topic_name, std::shared_ptr< dds_publisher > const & ) = 0;

    bool is_streaming() const { return _image_header.is_valid(); }
    void start_streaming( const image_header & header );
    void stop_streaming();

    void publish_image( const uint8_t * data, size_t size );

    std::shared_ptr< dds_topic > const & get_topic() const;

protected:
    std::shared_ptr< dds_topic_writer > _writer;
    image_header _image_header;
};


class dds_video_stream_server : public dds_stream_server
{
public:
    dds_video_stream_server( std::string const & name,
                             dds_stream_profiles const & profiles,
                             int default_profile_index = 0 );

    void open( std::string const & topic_name, std::shared_ptr< dds_publisher > const & ) override;
};


class dds_motion_stream_server : public dds_stream_server
{
public:
    dds_motion_stream_server( std::string const & name,
                              dds_stream_profiles const & profiles,
                              int default_profile_index = 0 );
    
    void open( std::string const & topic_name, std::shared_ptr< dds_publisher > const & ) override;
};


}  // namespace realdds
