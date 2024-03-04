// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once


namespace realdds {


class dds_device_watcher;


namespace topics {
class device_info;
}


// Base class for anything that needs discovery notifications. The device-watcher will update us with liveliness changes
// when the device starts/stops being broadcast (and therefore when we may need to change our internal state).
//
class dds_discovery_sink
{
    friend class dds_device_watcher;  // so we can get discovery updates

    // Everything else protected; only the device-watcher has access
protected:
    virtual ~dds_discovery_sink() = default;

    // Called when discovery is lost
    //
    virtual void on_discovery_lost() = 0;

    // Called when discovery is gained after being lost
    // The device-info (though not the topic-root) may have changed; the new one is passed in
    //
    virtual void on_discovery_restored( topics::device_info const & ) = 0;
};


}  // namespace realdds
