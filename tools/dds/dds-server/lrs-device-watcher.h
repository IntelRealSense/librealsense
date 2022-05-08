// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

namespace tools {

// This class is in charge of notifying a RS device connection / disconnection
// It will also notify of all connected devices during it's wakeup.
class lrs_device_watcher
{
public:
    lrs_device_watcher();
    ~lrs_device_watcher();

private:
};  // class lrs_device_watcher
}  // namespace tools
