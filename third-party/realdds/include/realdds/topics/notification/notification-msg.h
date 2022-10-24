// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


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


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;


namespace topics {

    
namespace raw {
namespace device {
class notificationPubSubType;
class notification;
}  // namespace device
}  // namespace raw


namespace device {

    
class notification
{
public:
    using type = raw::device::notificationPubSubType;

    bool is_valid() const { return _msg_type < msg_type::MAX_MSG_TYPE; }
    void invalidate() { _msg_type = msg_type::MAX_MSG_TYPE; }

    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );
    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      std::string const & topic_name )
    {
        return create_topic( participant, topic_name.c_str() );
    }

    // This helper method will take the next sample from a reader.
    //
    // Returns true if successful. Make sure you still check is_valid() in case the sample info isn't!
    // Returns false if no more data is available.
    // Will throw if an unexpected error occurs.
    //
    static bool
    take_next( dds_topic_reader &, notification * output, eprosima::fastdds::dds::SampleInfo * optional_info = nullptr );

    // Currently we use constant MAX size of profiles,
    // If we decide we want a scaled solution we may need to split the profiles to 
    // message for each profile (But then we have multiple transport overhead..)
    static const size_t MAX_VIDEO_PROFILES = 400;
    static const size_t MAX_MOTION_PROFILES = 20;

#pragma pack( push, 1 )
    enum class msg_type : uint16_t
    {
        DEVICE_HEADER,
        VIDEO_STREAM_PROFILES,
        MOTION_STREAM_PROFILES,
        CONTROL_RESPONSE,
        MAX_MSG_TYPE
    };

    struct device_header_msg
    {
        size_t num_of_streams;
    };

    struct video_stream_profile
    {
        int8_t stream_index;     // Used to distinguish similar streams like IR L / R
        int16_t uid;             // Stream unique ID
        int16_t framerate;       // FPS
        int8_t format;           // Corresponds to rs2_format
        int8_t type;             // Corresponds to rs2_stream
        int16_t width;           // Resolution width [pixels]
        int16_t height;          // Resolution width [pixels]
        bool default_profile;    // Is default stream of the sensor
        //intrinsics - TODO
    };

    struct motion_stream_profile
    {
        int8_t stream_index;     // Used to distinguish similar streams like IR L / R
        int16_t uid;             // Stream unique ID
        int16_t framerate;       // FPS
        int8_t format;           // Corresponds to rs2_format
        int8_t type;             // Corresponds to rs2_stream
        bool default_profile;    // Is default stream of the sensor
    };

    struct video_stream_profiles_msg
    {
        char group_name[32];     // Streams can be grouped together to indicate physical or logical connection
        size_t num_of_profiles;  
        video_stream_profile profiles[MAX_VIDEO_PROFILES];
    };

    struct motion_stream_profiles_msg
    {
        char group_name[32];     // Streams can be grouped together to indicate physical or logical connection
        size_t num_of_profiles; 
        motion_stream_profile profiles[MAX_MOTION_PROFILES];
    };

    enum class control_result : uint16_t
    {
        SUCCESSFUL,
        FAILED,
        MAX_CONTROL_RESULT  // TODO - for success/failure bool is enough. Consider how to pass exception information
    };

    struct control_response_msg
    {
        uint32_t message_id;  // Running counter
        uint32_t response_to; // message_id of the control this message is responding to
        control_result result;
        //TODO - double value for options or vector of bytes for HW monitor
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
    notification( const raw::device::notification & );

    // Get the raw data with casting to the desired type
    // Syntax: auto msg = raw_msg.get<DEVICE_STREAMS_INFO>();
    //         if ( msg ) do something with it;
    template < typename T > 
    T * get()
    {
        return _raw_data.size() > 0 ? reinterpret_cast< T * >( _raw_data.data() ) : nullptr;
    }

    msg_type _msg_type = msg_type::MAX_MSG_TYPE;
    std::vector<uint8_t> _raw_data;
    int _size;
};


}  // namespace device
}  // namespace topics
}  // namespace realdds
