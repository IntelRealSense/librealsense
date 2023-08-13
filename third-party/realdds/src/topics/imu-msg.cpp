// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/topics/ros2/ros2imu.h>
#include <realdds/topics/imu-msg.h>
#include <realdds/topics/ros2/ros2imuPubSubTypes.h>

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic-writer.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-time.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/Topic.hpp>


namespace realdds {
namespace topics {


imu_msg::imu_msg()
{
    // Everything (Vector3, Quaternion) should be zero'ed out except the arrays
    // "If you have no estimate for one of the data elements (e.g. your IMU doesn't produce an orientation estimate),
    // please set element 0 of the associated covariance matrix to -1"
    imu_data().orientation_covariance() = { -1., 0., 0., 0., 0., 0., 0., 0., 0. };
    // "you can initiate the sensor covariance matrix by giving some small values on the diagonal"
    double const cov = 0.01; // TODO from context?
    imu_data().linear_acceleration_covariance() = { cov, 0., 0., 0., cov, 0., 0., 0., cov };
    imu_data().angular_velocity_covariance() = { cov, 0., 0., 0., cov, 0., 0., 0., cov };
    timestamp( eprosima::fastrtps::c_TimeInvalid );
}


imu_msg::imu_msg( sensor_msgs::msg::Imu && rhs )
{
    imu_data() = std::move( rhs );
}


imu_msg & imu_msg::operator=( sensor_msgs::msg::Imu && rhs )
{
    imu_data() = std::move( rhs );
    return *this;
}


/*static*/ std::shared_ptr< dds_topic >
imu_msg::create_topic( std::shared_ptr< dds_participant > const & participant, char const * topic_name )
{
    return std::make_shared< dds_topic >( participant,
                                          eprosima::fastdds::dds::TypeSupport( new imu_msg::type ),
                                          topic_name );
}


/*static*/ bool
imu_msg::take_next( dds_topic_reader & reader, imu_msg * output, eprosima::fastdds::dds::SampleInfo * info )
{
    sensor_msgs::msg::Imu raw_data;
    eprosima::fastdds::dds::SampleInfo info_;
    if( ! info )
        info = &info_;  // use the local copy if the user hasn't provided their own
    auto status = reader->take_next_sample( &raw_data, info );
    if( status == ReturnCode_t::RETCODE_OK )
    {
        // We have data
        if( output )
        {
            // Only samples for which valid_data is true should be accessed
            // valid_data indicates that the instance is still ALIVE and the `take` return an
            // updated sample
            if( ! info->valid_data )
                output->invalidate();
            else
                *output = std::move( raw_data );  // TODO - optimize copy, use dds loans
        }

        return true;
    }
    if( status == ReturnCode_t::RETCODE_NO_DATA )
    {
        // This is an expected return code and is not an error
        return false;
    }
    DDS_API_CALL_THROW( "imu_msg::take_next", status );
}


void imu_msg::write_to( dds_topic_writer & writer )
{
    DDS_API_CALL( writer.get()->write( &imu_data() ) );
}


std::string imu_msg::to_string() const
{
    std::ostringstream ss;
    if( is_valid() )
    {
        ss << "g[" << gyro_data().x() << ',' << gyro_data().y() << ',' << gyro_data().z();
        ss << "] a[" << accel_data().x() << ',' << accel_data().y() << ',' << accel_data().z();
        ss << "] @ " << time_to_string( timestamp() );
    }
    else
    {
        ss << "INVALID";
    }
    return ss.str();
}


}  // namespace topics
}  // namespace realdds
