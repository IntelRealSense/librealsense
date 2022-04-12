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

    std::atomic_bool _running;
    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    eprosima::fastdds::dds::Topic * _topic;
    eprosima::fastdds::dds::TypeSupport _type_support_ptr;
    std::unordered_map< std::string, dds_device_info > _devices_writers;
    rs2::context _ctx;
    std::vector< std::thread > _dds_device_handler_vec;
    dispatcher _dds_device_dispatcher;
    std::mutex _worker_mutex;
};  // class dds_server
