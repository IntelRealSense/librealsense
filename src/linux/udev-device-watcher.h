// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "../backend.h"
#include "../concurrency.h"
#include "../callback-invocation.h"

#include <libudev.h>


namespace librealsense {


class udev_device_watcher : public librealsense::platform::device_watcher
{
    active_object<> _active_object;

    callbacks_heap _callback_inflight;
    platform::backend const * _backend;

    platform::backend_device_group _devices_data;
    platform::device_changed_callback _callback;

    struct udev * _udev_ctx;
    struct udev_monitor * _udev_monitor;
    int _udev_monitor_fd;
    bool _changed = false;

public:
    udev_device_watcher( platform::backend const * );
    ~udev_device_watcher();

    // device_watcher
public:
    void start( platform::device_changed_callback callback ) override
    {
        stop();
        _callback = std::move( callback );
        _active_object.start();
    }

    void stop() override
    {
        _active_object.stop();
        _callback_inflight.wait_until_empty();
    }

    bool is_stopped() const override { return ! _active_object.is_active(); }

private:
    bool foreach_device( std::function< bool( struct udev_device* udev_dev ) > );
};


}  // namespace librealsense
