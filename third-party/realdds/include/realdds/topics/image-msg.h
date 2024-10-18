// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022-4 Intel Corporation. All Rights Reserved.
#pragma once

#include <realdds/dds-defines.h>
#include <fastdds/rtps/common/Time_t.h>

#include <realdds/topics/ros2/ros2image.h>

#include <string>
#include <memory>
#include <vector>


namespace sensor_msgs {
namespace msg {
class ImagePubSubType;
}  // namespace msg
}  // namespace sensor_msgs


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;


namespace topics {


class image_msg
{
    sensor_msgs::msg::Image _raw;

public:
    using type = sensor_msgs::msg::ImagePubSubType;

    image_msg() = default;

    // Disable copy
    image_msg( const image_msg & ) = delete;
    image_msg & operator=( const image_msg & ) = delete;

    // Move is OK
    image_msg( image_msg && ) = default;
    image_msg( sensor_msgs::msg::Image && );
    image_msg & operator=( image_msg && ) = default;
    image_msg & operator=( sensor_msgs::msg::Image && );

    bool is_valid() const { return ! _raw.data().empty(); }
    void invalidate() { _raw.data().clear(); }

    sensor_msgs::msg::Image & raw() { return _raw; }
    sensor_msgs::msg::Image const & raw() const { return _raw; }

    auto width() const { return _raw.width(); }
    void set_width( uint32_t w ) { _raw.width( w ); }
    auto height() const { return _raw.height(); }
    void set_height( uint32_t h ) { _raw.height( h ); }
    auto step() const { return _raw.step(); }
    void set_step( uint32_t step ) { _raw.step( step ); }

    std::string const & frame_id() const { return _raw.header().frame_id(); }
    void set_frame_id( std::string new_id ) { _raw.header().frame_id( std::move( new_id ) ); }

    std::string const & encoding() const { return _raw.encoding(); }
    void set_encoding( std::string new_encoding ) { _raw.encoding( std::move( new_encoding ) ); }

    bool is_bigendian() const { return _raw.is_bigendian(); }

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
                           image_msg * output,
                           dds_sample * optional_sample = nullptr );
};


}  // namespace topics
}  // namespace realdds
