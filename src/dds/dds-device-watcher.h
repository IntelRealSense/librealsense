// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/dds/dds-participant.h>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>

#include <map>


namespace librealsense {

namespace dds {
    namespace topics {
        class device_info;
    }  // namespace topics
    class dds_device;
}  // namespace dds


class dds_device_watcher : public librealsense::platform::device_watcher
{
public:
    dds_device_watcher() = delete;
    dds_device_watcher( std::shared_ptr< dds::dds_participant > );
    ~dds_device_watcher();

    void start( platform::device_changed_callback callback ) override;
    void stop() override;
    bool is_stopped() const override { return ! _active_object.is_active(); }

    bool foreach_device( std::function< bool( dds::dds_guid const &, std::shared_ptr< dds::dds_device > const & ) > );

private:
    void init();  // May throw

    std::shared_ptr< dds::dds_participant > _participant;
    std::shared_ptr< dds::dds_participant::listener > _listener;
    eprosima::fastdds::dds::Subscriber * _subscriber;
    eprosima::fastdds::dds::Topic * _topic;
    eprosima::fastdds::dds::DataReader * _reader;
    eprosima::fastdds::dds::TypeSupport _topic_type;
    bool _init_done;
    active_object<> _active_object;
    platform::device_changed_callback _callback;
    std::map< dds::dds_guid, std::shared_ptr< dds::dds_device > > _dds_devices; 
    std::mutex _devices_mutex;
};


}  // namespace librealsense
