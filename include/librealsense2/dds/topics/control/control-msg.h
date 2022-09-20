// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <librealsense2/h/rs_sensor.h>
#include "controlPubSubTypes.h"

namespace librealsense {
namespace dds {
namespace topics {
namespace device {
class control
{
public:
    using type = raw::device::controlPubSubType;

    //Open uses a vector of profiles, to have constant message size we limit it
    static const size_t MAX_OPEN_PROFILES = 8;

#pragma pack( push, 1 )
    enum class control_type : uint16_t
    {
        SENSOR_OPEN,
        SENSOR_CLOSE,
        MAX_CONTROL_TYPE
    };

    struct stream_profile
    {
        int16_t framerate;       // FPS
        rs2_format format;       // Transfer as uint8_t?
        rs2_stream type;         // Transfer as uint8_t?
        int16_t width;           // Resolution width [pixels]
        int16_t height;          // Resolution width [pixels]
    };

    struct sensor_open_msg
    {
        uint32_t message_id; // Running counter
        int16_t sensor_uid;
        stream_profile profiles[MAX_OPEN_PROFILES];
    };

    struct sensor_close_msg
    {
        uint32_t message_id; // Running counter
        int16_t sensor_uid;
    };


#pragma pack( pop )

    static std::string construct_topic_name( const std::string& topic_root )
    {
        return topic_root + "/control";
    }

    template<typename T>
    static void construct_raw_message( control_type msg_id, const T& msg, raw::device::control& raw_msg )
    {
        raw_msg.id() = static_cast< int16_t >( msg_id );
        raw_msg.size() = static_cast< uint32_t >( sizeof( msg ) );
        raw_msg.raw_data().assign( reinterpret_cast< const uint8_t * >( &msg ),
                                   reinterpret_cast< const uint8_t * >( &msg ) + raw_msg.size() );
    }

    control() = default;

    control( const raw::device::control& main )
        : _control_type( static_cast<control_type>(main.id()) )
        , _raw_data(main.raw_data()) // TODO: avoid data copy?
        , _size( main.size() )
    {
        if( _control_type >= control_type::MAX_CONTROL_TYPE )
        {
            throw std::runtime_error(" unsupported message type received, id:" + main.id());
        }
    }

    // Get the raw data with casting to the desired type
    // Syntax: auto msg = raw_msg.get<control_response_msg>();
    //         if ( msg ) do something with it;
    template < typename T > 
    T * get()
    {
        return _raw_data.size() > 0 ? reinterpret_cast< T * >( _raw_data.data() ) : nullptr;
    }

    control_type _control_type;
    std::vector<uint8_t> _raw_data;
    int _size;
};

}  // namespace device
}  // namespace topics
}  // namespace dds
}  // namespace librealsense
