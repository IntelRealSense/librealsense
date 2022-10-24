// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <realdds/topics/notification/notificationPubSubTypes.h>  // raw::device::notification
#include <librealsense2/utilities/concurrency/concurrency.h>

#include <memory>
#include <string>


namespace realdds {


namespace topics {
namespace raw {
namespace device {
class notification;
}
}  // namespace raw
}  // namespace topics


class dds_publisher;
class dds_topic_writer;


// A notification is a generic message, with data, in a specific direction: the server (this class) sends and a client
// receives and interprets.
// 
// Notifications are either on-demand (i.e., send this message right now) or on-discovery (send these messages to any
// new client that connects). We optionally store and automatically send such messages.
//
class dds_notification_server
{
public:
    dds_notification_server( std::shared_ptr< dds_publisher > const & publisher, const std::string & topic_name );
    ~dds_notification_server();

    // On-demand notification: these happen sequentially and from another thread
    void send_notification( topics::raw::device::notification && notification );

    // On-discovery notification: when a new client is detected
    void add_discovery_notification( topics::raw::device::notification && notification );

private:
    void send_discovery_notifications();

    std::shared_ptr< dds_publisher > _publisher;
    std::shared_ptr< dds_topic_writer > _writer;
    active_object<> _notifications_loop;
    single_consumer_queue< topics::raw::device::notification > _instant_notifications;
    std::vector< topics::raw::device::notification > _discovery_notifications;
    std::mutex _notification_send_mutex;
    std::condition_variable _send_notification_cv;
    std::atomic_bool _send_init_msgs = { false };
    std::atomic_bool _new_instant_notification = { false };
    std::atomic_bool _active = { false };
};


}
