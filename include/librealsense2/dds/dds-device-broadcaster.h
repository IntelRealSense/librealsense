// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API
#include <librealsense2/utilities/concurrency/concurrency.h>

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


namespace librealsense {
namespace dds {
namespace topics {
namespace raw {
class device_info;
}  // namespace raw
class device_info;
}  // namespace topics


class dds_participant;

class dds_device_broadcaster
{
public:
    dds_device_broadcaster( librealsense::dds::dds_participant & participant );
    ~dds_device_broadcaster();

    // Start listening for device changes
    bool run();

    // Create a new data writer for a new connected device and return its topic root
    std::string add_device( rs2::device dev );

    // Destroy the removed device data writer
    void remove_device( rs2::device dev );

private:

    class dds_client_listener;

    struct dds_device_handle
    {
        rs2::device device;
        eprosima::fastdds::dds::DataWriter * data_writer;
        std::shared_ptr< dds_client_listener > listener;
    };

    // handles the DDS writers pool of connected/disconnected RS devices.
    // It dispatch the DDS work to a worker thread.
    // If a device was removed:
    //   * remove it from the connected devices (destruct the data writer, the client will be
    //   notified automatically)
    // If a device was added:
    //   * Create a new data writer for it
    //   * Publish the device name

    void handle_device_changes(
        const std::vector< std::string > & devices_to_remove,
        const std::vector< std::pair< std::string, rs2::device > > & devices_to_add );


    void remove_dds_device( const std::string & device_key );
    bool add_dds_device( const std::string & device_key, const rs2::device & rs2_dev );
    bool create_device_writer( const std::string & device_key, rs2::device rs2_device );
    void create_dds_publisher();
    bool send_device_info_msg( const librealsense::dds::topics::device_info & dev_info );
    void fill_device_msg( const librealsense::dds::topics::device_info & dev_info,
                          librealsense::dds::topics::raw::device_info & msg ) const;
    librealsense::dds::topics::device_info query_device_info( const rs2::device & rs2_dev ) const;

    std::string get_topic_root( const std::string& dev_name, const std::string& dev_sn ) const;
    std::atomic_bool _trigger_msg_send;
    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    eprosima::fastdds::dds::Topic * _topic;
    std::unordered_map< std::string, dds_device_handle > _device_handle_by_sn;
    dispatcher _dds_device_dispatcher;
    active_object<> _new_client_handler;
    std::condition_variable _new_client_cv;
    std::mutex _new_client_mutex;
};  // class dds_device_broadcaster
}  // namespace dds
}  // namespace librealsense
