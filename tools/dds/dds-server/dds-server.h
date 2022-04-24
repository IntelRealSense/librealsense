// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API
#include <librealsense2/utilities/concurrency/concurrency.h> 

namespace librealsense {
namespace dds {
namespace topics {
namespace raw {
class device_info;
}  // namespace raw
struct device_info;
}  // namespace topics
}  // namespace dds
}  // namespace librealsense

namespace tools
{
    class dds_server
    {
    public:
        dds_server();
        ~dds_server();
        bool init( eprosima::fastdds::dds::DomainId_t domain_id );
        void run();

    private:
        // We want to know when readers join our topic
        class dds_serverListener : public eprosima::fastdds::dds::DataWriterListener
        {
        public:
            void on_publication_matched(
                eprosima::fastdds::dds::DataWriter* writer,
                const eprosima::fastdds::dds::PublicationMatchedStatus& info ) override;

            std::atomic< int > _matched = { 0 };
        };

        class DiscoveryDomainParticipantListener
            : public eprosima::fastdds::dds::DomainParticipantListener
        {

            virtual void on_participant_discovery(
                eprosima::fastdds::dds::DomainParticipant* participant,
                eprosima::fastrtps::rtps::ParticipantDiscoveryInfo&& info ) override;

        } _domain_listener;

        struct dds_device_info
        {
            rs2::device device;
            eprosima::fastdds::dds::DataWriter* data_writer;
            std::shared_ptr< dds_serverListener > listener;
        };

        // This 2 functions (prepare & post) handles the DDS publication of connected/disconnected RS devices.
        // It prepares the input and dispatch the DDS work to a worker thread.
        // If a device was removed:
        //   * remove it from the connected devices (destruct the data writer, the client will be
        //   notified automatically)
        // If a device was added:
        //   * Create a new data writer for it
        //   * Publish the device name
        bool prepare_devices_changed_lists(
            const rs2::event_information& info,
            std::vector< std::string >& devices_to_remove,
            std::vector< std::pair< librealsense::dds::topics::device_info, rs2::device > >& devices_to_add );

        void post_device_changes( const std::vector< std::string >& devices_to_remove,
            const std::vector< std::pair< librealsense::dds::topics::device_info, rs2::device > >& devices_to_add );


        void remove_dds_device( const std::string& device_key );
        void add_dds_device( const librealsense::dds::topics::device_info& dev_info, const rs2::device& rs2_dev );
        bool verify_client_exist( const std::string& device_key,
            std::chrono::steady_clock::duration timeout
            = std::chrono::steady_clock::duration::zero() ) const;
        bool create_device_writer( const std::string& device_key, rs2::device rs2_device );
        bool create_dds_participant( eprosima::fastdds::dds::DomainId_t domain_id );
        bool create_dds_publisher();
        void post_connected_devices_on_wakeup();

        librealsense::dds::topics::device_info query_device_info( const rs2::device& rs2_dev ) const;
        void fill_device_msg( const librealsense::dds::topics::device_info& dev_info, librealsense::dds::topics::raw::device_info& msg ) const;

        std::atomic_bool _running;
        eprosima::fastdds::dds::DomainParticipant* _participant;
        eprosima::fastdds::dds::Publisher* _publisher;
        eprosima::fastdds::dds::Topic* _topic;
        eprosima::fastdds::dds::TypeSupport _type_support_ptr;
        std::unordered_map< std::string, dds_device_info > _devices_writers;
        rs2::context _ctx;
        dispatcher _dds_device_dispatcher;
    };  // class dds_server
}
