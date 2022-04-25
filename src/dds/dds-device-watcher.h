// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <iostream>
#include <map>
#include <functional>

#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <callback-invocation.h>

namespace librealsense {

namespace dds {
namespace topics {
struct device_info;
}  // namespace topics
}  // namespace dds

class dds_device_watcher : public librealsense::platform::device_watcher
{
public:
    dds_device_watcher() = delete;
    dds_device_watcher( int domain_id );
    ~dds_device_watcher();

    void start( platform::device_changed_callback callback ) override;
    void stop() override;
    bool is_stopped() const override { return ! _active_object.is_active(); }

private:
    void init( int domain_id );  // May throw

    class DiscoveryDomainParticipantListener
        : public eprosima::fastdds::dds::DomainParticipantListener
    {
    public:
        DiscoveryDomainParticipantListener() = delete;
        DiscoveryDomainParticipantListener( std::function< void( eprosima::fastrtps::rtps::GUID_t ) > callback );
        virtual void
        on_publisher_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                                eprosima::fastrtps::rtps::WriterDiscoveryInfo && info ) override;

    private:
        std::function< void( eprosima::fastrtps::rtps::GUID_t guid ) > _datawriter_removed_callback;
    };

    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Subscriber * _subscriber;
    eprosima::fastdds::dds::Topic * _topic;
    eprosima::fastdds::dds::DataReader * _reader;
    eprosima::fastdds::dds::TypeSupport _type_ptr;
    bool _init_done;
    int _domain_id;
    active_object<> _active_object;
    platform::device_changed_callback _callback;
    //callbacks_heap _callback_inflight;
    std::map< eprosima::fastrtps::rtps::GUID_t, dds::topics::device_info > _dds_devices; 
    std::mutex _devices_mutex;
    std::shared_ptr< DiscoveryDomainParticipantListener > _domain_listener;
};
}  // namespace librealsense
