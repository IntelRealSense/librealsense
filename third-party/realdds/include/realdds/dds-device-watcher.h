// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-participant.h"
#include "dds-guid.h"
#include "dds-time.h"

#include <map>
#include <memory>
#include <mutex>


namespace realdds {

namespace topics {
class device_info;
}  // namespace topics


class dds_device;
class dds_topic_reader;


// Client-side device discovery service:
//      See docs/discovery.md
// 
// Watches topics::DEVICE_INFO_TOPIC_NAME and sends out notifications of additions/removals.
// 
// Devices are kept track of and an actual database of dds_device objects is maintained. Devices get created as soon as
// they're discovered and held as long as they're being broadcast. A device that loses discovery (e.g., is undergoing a
// HW reset) and is then discovered again will reuse the same dds_device object. The topic-root is how devices are
// distinguished.
//
class dds_device_watcher
{
public:
    dds_device_watcher() = delete;
    dds_device_watcher( std::shared_ptr< dds_participant > const & );
    ~dds_device_watcher();

    // The callback is called whenever discovery is lost or gained
    typedef std::function< void( std::shared_ptr< dds_device > const & ) > on_device_change_callback;

    void on_device_added( on_device_change_callback callback ) { _on_device_added = std::move( callback ); }
    void on_device_removed( on_device_change_callback callback ) { _on_device_removed = std::move( callback ); }

    void start();
    void stop();
    bool is_stopped() const;

    // Iterate over discovered devices until the callback returns false; returns true if all devices were iterated over
    // and this never happened.
    //
    bool foreach_device( std::function< bool( std::shared_ptr< dds_device > const & ) > ) const;

    // Returns true if the device is currently being broadcast. Custom devices that do not go through discovery will
    // return false.
    // 
    // Devices whose server is offline should return false but this can be unreliable if the server has crashed and DDS
    // liveliness has not yet removed it from the system.
    //
    bool is_device_broadcast( std::shared_ptr< dds_device > const & ) const;

private:
    void init();

    std::shared_ptr< dds_participant > _participant;
    std::shared_ptr< dds_topic_reader > _device_info_topic;

    on_device_change_callback _on_device_added;
    on_device_change_callback _on_device_removed;

    struct device_liveliness
    {
        std::shared_ptr< dds_device > alive;  // reset in remove_device()
        std::weak_ptr< dds_device > in_use;   // when !alive, to detect if it's still being used
        dds_guid writer_guid;
        dds_time last_seen;
    };

    void device_discovery_lost( device_liveliness &, std::lock_guard< std::mutex > & lock );

    using liveliness_map = std::map< std::string /*root*/, device_liveliness >;
    liveliness_map _device_by_root;
    mutable std::mutex _devices_mutex;
};


}  // namespace realdds
