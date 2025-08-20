// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 RealSense, Inc. All Rights Reserved.

#pragma once

#include <realdds/dds-defines.h>
#include <fastdds/rtps/common/Time_t.h>

#include <realdds/topics/ros2/sensor_msgs/msg/CompressedImage.h>

#include <string>
#include <memory>
#include <vector>


namespace sensor_msgs {
namespace msg {
class CompressedImagePubSubType;
}  // namespace msg
}  // namespace sensor_msgs


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;


namespace topics {


class compressed_image_msg
{
    sensor_msgs::msg::CompressedImage _raw;

public:
    using type = sensor_msgs::msg::CompressedImagePubSubType;

    compressed_image_msg() = default;

    // Disable copy
    compressed_image_msg( const compressed_image_msg & ) = delete;
    compressed_image_msg & operator=( const compressed_image_msg & ) = delete;

    // Move is OK
    compressed_image_msg( compressed_image_msg && ) = default;
    compressed_image_msg( sensor_msgs::msg::CompressedImage && );
    compressed_image_msg & operator=( compressed_image_msg && ) = default;
    compressed_image_msg & operator=( sensor_msgs::msg::CompressedImage && );

    bool is_valid() const { return ! _raw.data().empty(); }
    void invalidate() { _raw.data().clear(); }

    sensor_msgs::msg::CompressedImage & raw() { return _raw; }
    sensor_msgs::msg::CompressedImage const & raw() const { return _raw; }

    auto format() const { return _raw.format(); }
    void set_format( std::string f ) { _raw.format( f ); }

    std::string const & frame_id() const { return _raw.header().frame_id(); }
    void set_frame_id( std::string new_id ) { _raw.header().frame_id( std::move( new_id ) ); }

    void set_timestamp( dds_time const & t )
    {
        _raw.header().stamp().sec( t.seconds );
        _raw.header().stamp().nanosec( t.nanosec );
    }
    dds_time timestamp() const { return { _raw.header().stamp().sec(), _raw.header().stamp().nanosec() }; }


    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );

    // This helper method will take the next sample from a reader. 
    // 
    // Returns true if successful. Make sure you still check is_valid() in case the sample info isn't!
    // Returns false if no more data is available.
    // Will throw if an unexpected error occurs.
    //
    //Note - copies the data.
    //TODO - add an API for a function that loans the data and enables the user to free it later.
    static bool take_next( dds_topic_reader &,
                           compressed_image_msg * output,
                           dds_sample * optional_sample = nullptr );
};


}  // namespace topics
}  // namespace realdds
