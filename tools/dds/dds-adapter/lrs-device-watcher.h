// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <atomic>
#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API

namespace tools {

// This class is in charge of notifying a RS device connection / disconnection
// It will also notify of all connected devices during it's wakeup.
class lrs_device_watcher : public std::enable_shared_from_this<lrs_device_watcher>
{
public:
    lrs_device_watcher( rs2::context &_ctx );
    ~lrs_device_watcher();
    void run( std::function< void( rs2::device ) > add_device_cb,
              std::function< void( rs2::device ) > remove_device_cb );

private:
    void notify_connected_devices_on_wake_up( std::function< void( rs2::device ) > add_device_cb );
    rs2::context &_ctx;
    std::function<void( rs2::device )> _add_device_cb;
    std::function<void( rs2::device )> _remove_device_cb;
    std::shared_ptr<std::vector<rs2::device>> _rs_device_list;
};  // class lrs_device_watcher
}  // namespace tools
