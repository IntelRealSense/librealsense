/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
//
// Created by or on 6/26/19.
//

#ifndef LIBREALSENSE2_TRACKINGDEVICEHOLDER_H
#define LIBREALSENSE2_TRACKINGDEVICEHOLDER_H

#include <memory>

namespace perc
{
    class DLL_EXPORT TrackingDeviceHolder{
    public:
        virtual ~TrackingDeviceHolder() {}

        virtual perc::TrackingData::DeviceInfo get_device_info() { return perc::TrackingData::DeviceInfo(); };
        virtual std::shared_ptr<perc::TrackingDevice> get_device() = 0;

    };
}

#endif //LIBREALSENSE2_TRACKINGDEVICEHOLDER_H
