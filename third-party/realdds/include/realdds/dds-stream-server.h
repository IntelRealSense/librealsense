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


class dds_topic;
class dds_topic_writer;


struct image_header
{
    int format;
    int height = 0;
    int width = 0;

    bool is_valid() const { return width != 0 && height != 0; }
};


// Distributes stream frames into a dedicated topic
//
class dds_stream_server
{
public:
    dds_stream_server( std::shared_ptr< dds_topic_writer > const & topic );
    ~dds_stream_server();

    bool is_streaming() const { return _image_header.is_valid(); }
    void start_streaming( const image_header & header );

    void publish_image( const uint8_t * data, size_t size );

    std::shared_ptr< dds_topic > const & get_topic() const;

private:
    std::shared_ptr< dds_topic_writer > _writer;
    image_header _image_header;
};


}  // namespace realdds
