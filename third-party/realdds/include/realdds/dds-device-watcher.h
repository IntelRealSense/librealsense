// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-participant.h"

#include <map>
#include <memory>
#include <mutex>


namespace realdds {

namespace topics {
class device_info;
}  // namespace topics


class dds_device;
class dds_topic_reader;


// Watches the device_info.TOPIC_NAME topic and sends out notifications of additions/removals.
// 
// Using this class means that devices are kept track of and an actual database of dds_device objects is maintained: you
// can use foreach_device() to enumerate them. By default they do not get initialized and only contain the device-info
// data.
//
class dds_device_watcher
{
public:
    dds_device_watcher() = delete;
    dds_device_watcher( std::shared_ptr< dds_participant > const & );
    ~dds_device_watcher();

    // The callback is called right after construction and right before deletion
    typedef std::function< void( std::shared_ptr< dds_device > const & ) > on_device_change_callback;

    void on_device_added( on_device_change_callback callback ) { _on_device_added = std::move( callback ); }
    void on_device_removed( on_device_change_callback callback ) { _on_device_removed = std::move( callback ); }

    void start();
    void stop();
    bool is_stopped() const;

    bool foreach_device( std::function< bool( std::shared_ptr< dds_device > const & ) > ) const;

private:
    // The device exists - we know about it - but is unusable until we get details (sensors, profiles, etc.) and
    // initialization is complete. This initialization depends on several messages from the server,
    // and may take some time.
    // Returns true if initialization was successful.
    // Restrictions: May throw
    void init();  

    void remove_device( dds_guid const & );

    std::shared_ptr< dds_participant > _participant;
    std::shared_ptr< dds_participant::listener > _listener;
    std::shared_ptr< dds_topic_reader > _device_info_topic;

    on_device_change_callback _on_device_added;
    on_device_change_callback _on_device_removed;
    std::map< dds_guid, std::shared_ptr< dds_device > > _dds_devices;
    mutable std::mutex _devices_mutex;
};


}  // namespace realdds
