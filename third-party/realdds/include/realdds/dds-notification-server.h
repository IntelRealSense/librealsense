// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <realdds/topics/flexible-msg.h>
#include <rsutils/concurrency/concurrency.h>

#include <memory>
#include <string>


namespace realdds {


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

    dds_guid const & guid() const;

    // By default we're not running, to avoid on-discovery before all discovery messages have been collected
    void run();
    bool is_running() const { return _active; }

    // On-demand notification: these happen sequentially and from another thread
    void send_notification( topics::flexible_msg && notification );

    // On-discovery notification: when a new client is detected
    void add_discovery_notification( topics::flexible_msg && notification );
    // Allow manual trigger of discovery
    void trigger_discovery_notifications();

private:
    void send_discovery_notifications();

    std::shared_ptr< dds_publisher > _publisher;
    std::shared_ptr< dds_topic_writer > _writer;
    active_object<> _notifications_loop;
    single_consumer_queue< topics::raw::flexible > _instant_notifications;
    std::vector< topics::raw::flexible > _discovery_notifications;
    std::mutex _notification_send_mutex;
    std::condition_variable _send_notification_cv;
    std::atomic_bool _send_init_msgs = { false };
    std::atomic_bool _new_instant_notification = { false };
    std::atomic_bool _active = { false };
};


}
