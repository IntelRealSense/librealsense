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

#include "context.h"

namespace librealsense
{
    class device : public virtual device_interface, public info_container
    {
    public:
        size_t get_sensors_count() const override;

        sensor_interface& get_sensor(size_t subdevice) override;
        const sensor_interface& get_sensor(size_t subdevice) const override;

        void hardware_reset() override;

        virtual std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        size_t find_sensor_idx(const sensor_interface& s) const;

        std::shared_ptr<context> get_context() const override { return _context; }


    protected:
        int add_sensor(std::shared_ptr<sensor_interface> sensor_base);

        uvc_sensor& get_uvc_sensor(int subdevice);

        explicit device(std::shared_ptr<context> ctx, const device_info& info): _context(ctx),_info(info)  {}

    private:
        std::vector<std::shared_ptr<sensor_interface>> _sensors;
        std::shared_ptr<context> _context;
        const device_info& _info;
    };
}
