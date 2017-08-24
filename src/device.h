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

        platform::backend_device_group get_device_data() const override
        {
            return _group;
        }

        std::pair<uint32_t, rs2_extrinsics> get_extrinsics(const stream_interface& stream) const override;

    protected:
        int add_sensor(std::shared_ptr<sensor_interface> sensor_base);
        void register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t groupd_index);
        uvc_sensor& get_uvc_sensor(int subdevice);

        explicit device(std::shared_ptr<context> ctx, const platform::backend_device_group group): _context(ctx),_group(group)  {}

    private:
        std::map<int, std::pair<uint32_t, std::shared_ptr<const stream_interface>>> _extrinsics;
        std::vector<std::shared_ptr<sensor_interface>> _sensors;
        std::shared_ptr<context> _context;
        const platform::backend_device_group _group;
    };
}
