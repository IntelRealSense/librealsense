// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <memory>
#include <string>


// Forward declaration
namespace eprosima {
namespace fastdds {
namespace dds {
class Publisher;
class Topic;
class TypeSupport;
class DataWriter;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {


class dds_participant;


struct image_header
{
    int format;
    int height;
    int width;
};


// Distributes stream frames into a dedicated topic
//
class dds_stream_server
{
public:
    dds_stream_server( std::shared_ptr< dds_participant > const& participant,
        eprosima::fastdds::dds::Publisher* publisher,
        const std::string& topic_root,
        const std::string& stream_name );
    ~dds_stream_server();


    void publish_image( const uint8_t* data, size_t size );
    void set_image_header( const image_header & header ) { _image_header = header; }

private:
    std::shared_ptr< dds_participant > _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    eprosima::fastdds::dds::Topic * _topic;
    eprosima::fastdds::dds::DataWriter * _data_writer;
    image_header _image_header;
};


}  // namespace realdds
