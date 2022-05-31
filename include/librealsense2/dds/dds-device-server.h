// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <unordered_map>
#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API

// Forward declaration
namespace eprosima {
namespace fastdds {
namespace dds {
class DomainParticipant;
class Publisher;
class Topic;
class TypeSupport;
class DataWriter;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima

namespace librealsense {
namespace dds {

// Forward declaration
namespace topics {
namespace raw {
class device_info;
}  // namespace raw
class device_info;
}  // namespace topics


class dds_participant;

class dds_device_server
{
public:
    struct image_header
    {
        int format;
        int height;
        int width;
    };

    dds_device_server( librealsense::dds::dds_participant& participant, const std::string& topic_root );
    ~dds_device_server();
    bool init( const std::vector<std::string>& supported_streams_names );
    bool is_valid() const { return ( nullptr != _publisher ); }
    bool operator!() const { return ! is_valid(); }
    void set_image_header( const std::string &stream_name, const image_header & header ) 
    { 
        stream_name_to_server.at( stream_name )->set_image_header(header);
    }
    void publish_frame( const std::string &stream_name, uint8_t * frame, int size )
    {
        stream_name_to_server.at( stream_name )->publish_video_frame( frame, size );
    }
    
private:
    class dds_stream_server
    {
    public:
        dds_stream_server( eprosima::fastdds::dds::DomainParticipant * _participant, eprosima::fastdds::dds::Publisher * publisher, const std::string& topic_root, const std::string& stream_name );
        ~dds_stream_server();
        void publish_video_frame( uint8_t* frame, int size );
        void set_image_header( const image_header & header ) { _image_header = header; }

    private:
        std::string _topic_name;
        eprosima::fastdds::dds::DomainParticipant * _participant;
        eprosima::fastdds::dds::Publisher* _publisher;
        eprosima::fastdds::dds::Topic * _topic;
        std::shared_ptr<eprosima::fastdds::dds::TypeSupport> _topic_type_ptr;
        eprosima::fastdds::dds::DataWriter * _data_writer;
        image_header _image_header;
    };

    bool create_dds_publisher( );
    
    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    std::string _topic_root;
    std::unordered_map<std::string, std::shared_ptr<dds_stream_server>> stream_name_to_server;
};  // class dds_device_server
}  // namespace dds
}  // namespace librealsense
