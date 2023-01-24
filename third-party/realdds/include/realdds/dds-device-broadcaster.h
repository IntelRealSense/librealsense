// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <realdds/topics/device-info-msg.h>
#include <rsutils/concurrency/concurrency.h>

#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Forward declare FastDDS types
namespace eprosima {
namespace fastdds {
namespace dds {
class DomainParticipant;
class Publisher;
class Topic;
class DataWriter;
class TypeSupport;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {


class dds_participant;
class dds_publisher;
class dds_topic_writer;
class dds_topic;

// We're responsible for broadcasting each device to listeners on the DDS network
//
class dds_device_broadcaster
{
public:
    using device_info = topics::device_info;

    dds_device_broadcaster( std::shared_ptr< dds_participant > const & participant );
    ~dds_device_broadcaster();

    // Start listening for device changes
    bool run();

    // Broadcast a device addition
    void add_device( device_info const & dev_info );

    // Broadcast that a device was removed and is no longer available
    void remove_device( device_info const & dev_info );

private:

    class dds_client_listener;

    struct dds_device_handle
    {
        device_info info;
        std::shared_ptr< dds_client_listener > client_listener;
    };

    // handles the DDS writers pool of connected/disconnected RS devices.
    // It dispatch the DDS work to a worker thread.
    // If a device was removed:
    //   * remove it from the connected devices (destruct the data writer, the client will be
    //   notified automatically)
    // If a device was added:
    //   * Create a new data writer for it
    //   * Publish the device name

    void handle_device_changes(const std::vector< std::string > & devices_to_remove,
                               const std::vector< device_info > & devices_to_add );


    void remove_dds_device( const std::string & serial_number );
    bool add_dds_device( const device_info & dev_info );
    bool create_device_writer( device_info const & dev_info );
    bool send_device_info_msg( const dds_device_handle & handle );

    std::atomic_bool _trigger_msg_send;
    std::shared_ptr< dds_participant > _participant;
    std::shared_ptr< dds_publisher > _publisher;
    std::shared_ptr< dds_topic > _topic;
    std::unordered_map< std::string, dds_device_handle > _device_handle_by_sn;
    dispatcher _dds_device_dispatcher;
    active_object<> _new_client_handler;
    std::condition_variable _new_client_cv;
    std::mutex _new_client_mutex;
    std::atomic_bool _active = { false };
};  // class dds_device_broadcaster


}  // namespace realdds
