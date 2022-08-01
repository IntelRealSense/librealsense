// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <librealsense2/h/rs_sensor.h>
#include "notificationPubSubTypes.h"

namespace librealsense {
namespace dds {
namespace topics {
namespace device {
class notification
{
public:
    using type = raw::device::notificationPubSubType;

    // Currently we use constant MAX size of profiles,
    // If we decide we want a scaled solution we may need to split the profiles to 
    // message for each profile (But then we have multiple transport overhead..)
    static const size_t MAX_VIDEO_PROFILES = 200;
    static const size_t MAX_MOTION_PROFILES = 20;

#pragma pack( push, 1 )
    enum class msg_type : uint16_t
    {
        DEVICE_HEADER,
        SENSOR_HEADER,
        VIDEO_STREAM_PROFILES,
        MOTION_STREAM_PROFILES,
        MAX_MSG_TYPE
    };

    struct device_header_msg
    {
        size_t num_of_sensors;
    };

    struct sensor_header_msg
    {
        enum sensor_type // maybe enum class with 1 byte inheritance 
        {
            VIDEO_SENSOR,
            MOTION_SENSOR
        };

        sensor_type type;
        uint8_t index;           // Index of the current sensor [0-(num_of_sensors-1)]
        char name[32];           // Sensor name
    };


    struct video_stream_profile
    {
        int8_t stream_index;     // Normally used to distinguish IR L / R
        int16_t uid;             // Stream unique ID
        int16_t framerate;       // FPS
        rs2_format format;       // Transfer as uint8_t?
        rs2_stream type;         // Transfer as uint8_t?
        int16_t width;           // Resolution width [pixels]
        int16_t height;          // Resolution width [pixels]
        //intrinsics - TODO
    };

    struct motion_stream_profile
    {
        int8_t stream_index;     // Normally used to distinguish IR L / R
        int16_t uid;             // Stream unique ID
        int16_t framerate;       // FPS
        rs2_format format;       // Transfer as uint8_t?
        rs2_stream type;         // Transfer as uint8_t?
    };

    struct video_stream_profiles_msg
    {
        uint8_t dds_sensor_index;  // Index of the current dds sensor [0-(num_of_sensors-1)]
        size_t num_of_profiles;  
        video_stream_profile profiles[MAX_VIDEO_PROFILES];
    };

    struct motion_stream_profiles_msg
    {
        uint8_t dds_sensor_index; // Index of the current sensor [0-(num_of_sensors-1)]
        size_t num_of_profiles; 
        motion_stream_profile profiles[MAX_MOTION_PROFILES];
    };

#pragma pack( pop )

    static std::string construct_topic_name( const std::string& topic_root )
    {
        return topic_root + "/notification";
    }

    template<typename T>
    static void construct_raw_message( msg_type msg_id, const T& msg, raw::device::notification& raw_msg )
    {
        raw_msg.id() = static_cast< int16_t >( msg_id );
        raw_msg.size() = static_cast< uint32_t >( sizeof( msg ) );
        raw_msg.raw_data().assign( reinterpret_cast< const uint8_t * >( &msg ),
                                   reinterpret_cast< const uint8_t * >( &msg ) + raw_msg.size() );
    }

    notification() = default;

    notification( const raw::device::notification & main )
        : _msg_type( static_cast<msg_type>(main.id()) )
        , _raw_data(main.raw_data()) // TODO: avoid data copy?
        , _size( main.size() )
    {
        if( _msg_type >= msg_type::MAX_MSG_TYPE )
        {
            throw std::runtime_error(" unsupported message type received, id:" + main.id());
        }
    }

    // Get the raw data with casting to the desired type
    // Syntax: auto msg = raw_msg.get<DEVICE_STREAMS_INFO>();
    //         if ( msg ) do something with it;
    template < typename T > 
    T * get()
    {
        return _raw_data.size() > 0 ? reinterpret_cast< T * >( _raw_data.data() ) : nullptr;
    }

    msg_type _msg_type;
    std::vector<uint8_t> _raw_data;
    int _size;
};

}  // namespace device
}  // namespace topics
}  // namespace dds
}  // namespace librealsense
