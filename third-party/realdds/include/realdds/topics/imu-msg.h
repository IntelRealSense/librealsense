// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once


#include <realdds/dds-defines.h>
#include <fastdds/rtps/common/Time_t.h>

#include <realdds/topics/ros2/ros2imu.h>

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
class ImuPubSubType;
}  // namespace msg
}  // namespace sensor_msgs

namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;
class dds_topic_writer;


namespace topics {


class imu_msg
{
    sensor_msgs::msg::Imu _data;

public:
    using type = sensor_msgs::msg::ImuPubSubType;

    // Default sets all the fields properly
    imu_msg();

    // Disable copy
    imu_msg( const imu_msg & ) = delete;
    imu_msg & operator=( const imu_msg & ) = delete;

    // Move is OK
    imu_msg( imu_msg && ) = default;
    imu_msg( sensor_msgs::msg::Imu && );
    imu_msg & operator=( imu_msg && ) = default;
    imu_msg & operator=( sensor_msgs::msg::Imu && );

    sensor_msgs::msg::Imu const & imu_data() const { return _data; }
    sensor_msgs::msg::Imu & imu_data() { return _data; }

    geometry_msgs::msg::Vector3 const & accel_data() const { return imu_data().linear_acceleration(); }
    geometry_msgs::msg::Vector3 & accel_data() { return imu_data().linear_acceleration(); }

    geometry_msgs::msg::Vector3 const & gyro_data() const { return imu_data().angular_velocity(); }
    geometry_msgs::msg::Vector3 & gyro_data() { return imu_data().angular_velocity(); }

    void timestamp( dds_time const & t )
    {
        imu_data().header().stamp().sec( t.seconds );
        imu_data().header().stamp().nanosec( t.nanosec );
    }
    dds_time timestamp() const
    {
        return { imu_data().header().stamp().sec(), imu_data().header().stamp().nanosec() };
    }

    bool is_valid() const { return timestamp() != eprosima::fastrtps::c_TimeInvalid; }
    void invalidate() { timestamp( eprosima::fastrtps::c_TimeInvalid ); }

    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );

    // This helper method will take the next sample from a reader. 
    // 
    // Returns true if successful. Make sure you still check is_valid() in case the sample info isn't!
    // Returns false if no more data is available.
    // Will throw if an unexpected error occurs.
    //
    // Note - copies the data.
    static bool take_next( dds_topic_reader &,
                           imu_msg * output,
                           eprosima::fastdds::dds::SampleInfo * optional_info = nullptr );

    void write_to( dds_topic_writer & );

    std::string to_string() const;
};


}  // namespace topics
}  // namespace realdds
