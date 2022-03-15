// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <iostream>
#include <map>
#include <functional>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <concurrency.h>
#include <callback-invocation.h>

namespace librealsense {

// struct dds_device_info
//{
//    std::string device_name;
//};

class dds_device_watcher : public librealsense::platform::device_watcher
{
public:
    dds_device_watcher();
    ~dds_device_watcher();
    void init();  // May throw

    void start( platform::device_changed_callback callback ) override;
    void stop() override;
    bool is_stopped() const override { return ! _active_object.is_active(); }

private:
    class DiscoveryDomainParticipantListener
        : public eprosima::fastdds::dds::DomainParticipantListener
    {
    public:
        DiscoveryDomainParticipantListener() = delete;
        DiscoveryDomainParticipantListener( std::function< void( uint32_t ) > callback );
        virtual void
        on_publisher_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                                eprosima::fastrtps::rtps::WriterDiscoveryInfo && info ) override;

    private:
        std::function< void( uint32_t ) > _callback;
    };

    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Subscriber * _subscriber;
    eprosima::fastdds::dds::Topic * _topic;
    eprosima::fastdds::dds::DataReader * _reader;
    eprosima::fastdds::dds::TypeSupport _type_ptr;
    bool _init_done;
    active_object<> _active_object;
    platform::device_changed_callback _callback;
    callbacks_heap _callback_inflight;
    std::map< uint32_t, std::string > _dds_devices;
    std::mutex _devices_mutex;
    std::shared_ptr< DiscoveryDomainParticipantListener > _domain_listener;
};
}  // namespace librealsense
