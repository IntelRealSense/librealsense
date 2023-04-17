// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <realdds/dds-defines.h>
#include <fastdds/rtps/common/Time_t.h>

#include <string>
#include <memory>
#include <vector>


namespace eprosima {
namespace fastdds {
namespace dds {
struct SampleInfo;
}
}  // namespace fastdds
}  // namespace eprosima

namespace sensor_msgs {
namespace msg {
class ImagePubSubType;
class Image;
}  // namespace msg
}  // namespace sensor_msgs

namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;


namespace topics {
namespace device {

    
class image
{
public:
    using type = sensor_msgs::msg::ImagePubSubType;

    image() = default;

    //Disable copy
    image( const image & ) = delete;
    image & operator=( const image & ) = delete;

    //move is OK
    image( image && ) = default;
    image( sensor_msgs::msg::Image && );
    image & operator=( image && ) = default;
    image & operator=( sensor_msgs::msg::Image && );

    bool is_valid() const { return width != -1 && height != -1; }
    void invalidate() { width = -1; }

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
                           image * output,
                           eprosima::fastdds::dds::SampleInfo * optional_info = nullptr );

    std::vector<uint8_t> raw_data;
    std::string frame_id;
    int width = -1;
    int height = -1;
    dds_time timestamp;
};


}  // namespace device
}  // namespace topics
}  // namespace realdds
