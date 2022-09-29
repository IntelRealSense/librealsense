// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <string>
#include <memory>


namespace eprosima {
namespace fastdds {
namespace dds {
struct SampleInfo;
}
}  // namespace fastdds
}  // namespace eprosima


namespace librealsense {
namespace dds {


class dds_participant;
class dds_topic;
class dds_topic_reader;


namespace topics {
namespace raw {
class device_infoPubSubType;
class device_info;
}  // namespace raw


class device_info
{
public:
    using type = raw::device_infoPubSubType;
    static constexpr char const * TOPIC_NAME = "realsense/device-info";

    device_info() = default;
    device_info( const raw::device_info & );

    bool is_valid() const { return ! name.empty(); }
    void invalidate() { name.clear(); }

    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );

    // This helper method will take the next sample from a reader. 
    // 
    // Returns true if successful. Make sure you still check is_valid() in case the sample info isn't!
    // Returns false if no more data is available.
    // Will throw if an unexpected error occurs.
    //
    static bool
    take_next( dds_topic_reader &, device_info * output, eprosima::fastdds::dds::SampleInfo * optional_info = nullptr );

    std::string name;
    std::string serial;
    std::string product_line;
    std::string topic_root;
    bool locked = false;
};


}  // namespace topics
}  // namespace dds
}  // namespace librealsense
