// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include "librealsense2/h/rs_sensor.h"
#include "notificationsPubSubTypes.h"

namespace librealsense {
namespace dds {
namespace topics {
namespace device {
class notifications
{
public:

    using type = raw::device::notificationsPubSubType;

#pragma pack( push, 1 )
    enum class msg_type : uint16_t
    {
        DEVICE_HEADER,
        STREAM_HEADER,
        VIDEO_STREAM_PROFILES,
        MOTION_STREAM_PROFILES,
        MAX_MSG_TYPE
    };

    struct device_header
    {
        size_t num_of_streams;
    };

    struct stream_header
    {
        uint8_t index;           // Index of the current stream [0-(num_of_streams-1)]
        char name[32];           // Stream name
        size_t num_of_profiles;  // Profile count for this stream
    };


    struct video_stream_profile
    {
        int8_t index;           // Sensor index (Normally )
        int16_t uid;            // Stream unique ID
        int8_t framerate;
        rs2_format format;      // Transfer as uint8_t?
        rs2_stream type;        // Transfer as uint8_t?
        int16_t width;          
        int16_t height;         
        //intrinsics - TODO
    };

    struct motion_stream_profile
    {
        int8_t index;           // Sensor index (Normally )
        int16_t uid;            // Stream unique ID
        int8_t framerate;
        rs2_format format;      // Transfer as uint8_t?
        rs2_stream type;        // Transfer as uint8_t?
    };

    struct video_stream_profiles
    {
        std::vector<video_stream_profile> profiles_vec;
    };

    struct motion_stream_profiles
    {
        std::vector<motion_stream_profile> profiles_vec;
    };

#pragma pack( pop )

    static std::string construct_notifications_topic_name( const std::string& topic_root )
    {
        return topic_root + "/notifications";
    }

    template<typename T>
    static void construct_raw_message( msg_type msg_id, const T& msg, raw::device::notifications& raw_msg )
    {
        raw_msg.id() = static_cast<int16_t>(msg_id);
        raw_msg.size() = sizeof( T );
        raw_msg.raw_data().assign((uint8_t* )&msg, (uint8_t* )&msg + raw_msg.size());
    }

    notifications() = default;

    notifications( const raw::device::notifications & main )
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
