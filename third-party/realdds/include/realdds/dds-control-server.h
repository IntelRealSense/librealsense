// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <realdds/topics/flexible/flexiblePubSubTypes.h>
#include <librealsense2/utilities/concurrency/concurrency.h>

#include <memory>
#include <string>


namespace realdds {


namespace topics {
namespace raw {
namespace device {
class control;
}
}  // namespace raw
}  // namespace topics


class dds_subscriber;
class dds_topic_reader;


// A control is a command sent from a client (camera user) and the server (this class) receives and interprets.
//
class dds_control_server
{
public:
    dds_control_server( std::shared_ptr< dds_subscriber > const & subscriber, const std::string & topic_name );
    ~dds_control_server();

    typedef std::function< void() > on_control_message_received_callback;
    void on_control_message_received( on_control_message_received_callback callback );

    std::shared_ptr< dds_topic_reader > reader() { return _reader; }

private:
    std::shared_ptr< dds_subscriber > _subscriber;
    std::shared_ptr< dds_topic_reader > _reader;
};


}
