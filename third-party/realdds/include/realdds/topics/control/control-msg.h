// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <librealsense2/h/rs_sensor.h>
#include "controlPubSubTypes.h"

namespace realdds {

    
class dds_participant;
class dds_topic;


namespace topics {
namespace device {

    
class control
{
public:
    using type = raw::device::controlPubSubType;

    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );
    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      std::string const & topic_name )
    {
        return create_topic( participant, topic_name.c_str() );
    }

    //Open uses a vector of profiles, to have constant message size we limit it
    static const uint8_t MAX_OPEN_STREAMS = 8;

#pragma pack( push, 1 )
    enum class msg_type : uint16_t
    {
        STREAMS_OPEN,
        STREAMS_CLOSE,
        MAX_CONTROL_TYPE
    };

    struct stream_profile
    {
        int16_t uid;             // Stream unique ID
        int8_t stream_index;     // Used to distinguish similar streams like IR L / R
        int16_t framerate;       // FPS
        int8_t format;           // Corresponds to rs2_format
        int16_t width;           // Resolution width [pixels]
        int16_t height;          // Resolution width [pixels]
    };

    struct streams_open_msg
    {
        uint32_t message_id; // Running counter
        uint8_t number_of_streams;
        stream_profile streams[MAX_OPEN_STREAMS];
    };

    struct streams_close_msg
    {
        uint32_t message_id; // Running counter
        uint8_t number_of_streams;
        int16_t stream_uids[MAX_OPEN_STREAMS];
        int8_t stream_indexes[MAX_OPEN_STREAMS];
    };


#pragma pack( pop )

    static std::string construct_topic_name( const std::string& topic_root )
    {
        return topic_root + "/control";
    }

    template<typename T>
    static void construct_raw_message( msg_type msg_id, const T& msg, raw::device::control& raw_msg )
    {
        raw_msg.id() = static_cast< int16_t >( msg_id );
        raw_msg.size() = static_cast< uint32_t >( sizeof( msg ) );
        raw_msg.data().assign( reinterpret_cast< const uint8_t * >( &msg ),
                               reinterpret_cast< const uint8_t * >( &msg ) + raw_msg.size() );
    }

    control() = default;

    control( const raw::device::control&& main )
        : _msg_type( static_cast< msg_type >(main.id()) )
        , _raw_data(main.data()) // TODO: avoid data copy?
        , _size( main.size() )
    {
        if( _msg_type >= msg_type::MAX_CONTROL_TYPE )
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

    msg_type _msg_type;
    std::vector<uint8_t> _raw_data;
    int _size;
};


}  // namespace device
}  // namespace topics
}  // namespace realdds
