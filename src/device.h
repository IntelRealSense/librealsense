// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "backend.h"
#include "archive.h"
#include "hw-monitor.h"
#include "option.h"
#include "sensor.h"
#include "sync.h"
#include "core/streaming.h"

namespace librealsense
{
    class device : public virtual device_interface, public info_container
    {
    public:
        size_t get_sensors_count() const override;

        sensor_interface& get_sensor(size_t subdevice) override;
        const sensor_interface& get_sensor(size_t subdevice) const override;

        virtual void hardware_reset()
        {
            throw not_implemented_exception(to_string() << __FUNCTION__ << " is not implemented for this device!");
        }

        rs2_extrinsics get_extrinsics(size_t from, rs2_stream from_stream, size_t to, rs2_stream to_stream) const override;

    protected:
        int add_sensor(std::shared_ptr<sensor_interface> sensor_base);

        uvc_sensor& get_uvc_sensor(int subdevice);

    private:
        std::vector<std::shared_ptr<sensor_interface>> _sensors;
    };
}