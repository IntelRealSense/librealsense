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
            eprosima::fastdds::dds::DataWriter * writer,
            const eprosima::fastdds::dds::PublicationMatchedStatus & info ) override;

        std::atomic< int > _matched = { 0 };
    };

    class DiscoveryDomainParticipantListener
        : public eprosima::fastdds::dds::DomainParticipantListener
    {

        virtual void on_participant_discovery(
            eprosima::fastdds::dds::DomainParticipant * participant,
            eprosima::fastrtps::rtps::ParticipantDiscoveryInfo && info ) override;

    } _domain_listener;

    struct dds_device_info
    {
        rs2::device device;
        eprosima::fastdds::dds::DataWriter * data_writer;
        std::shared_ptr< dds_serverListener > listener;
    };

    // This function handles the DDS publication of connected/disconnected RS devices.
    // It prepares the input and dispatch the DDS work to a worker thread.
    // If a device was removed:
    //   * remove it from the connected devices (destruct the data writer, the client will be
    //   notified automatically)
    // If a device was added:
    //   * Create a new data writer for it
    //   * Publish the device name
    void handle_device_changes( std::vector< std::string > devices_to_remove,
        std::vector< std::pair< std::string, rs2::device > > devices_to_add );

    std::atomic_bool _running;
    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    eprosima::fastdds::dds::Topic * _topic;
    eprosima::fastdds::dds::TypeSupport _type_support_ptr;
    std::unordered_map< std::string, dds_device_info > _devices_writers;
    rs2::context _ctx;
    dispatcher _dds_device_dispatcher;
};  // class dds_server
