// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <mutex>
#include <string>
#include "device.h"
#include "context.h"
#include "backend.h"
#include "hw-monitor.h"
#include "image.h"
#include "stream.h"
#include "l500-private.h"
#include "error-handling.h"
#include "global_timestamp_reader.h"

namespace librealsense
{

    class l500_device : public virtual device, public debug_interface, public global_time_interface
    {
    public:
        l500_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::shared_ptr<uvc_sensor> create_depth_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& all_device_infos);

        uvc_sensor& get_depth_sensor() { return dynamic_cast<uvc_sensor&>(get_sensor(_depth_device_idx)); }

        std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) override
        {
            return _hw_monitor->send(input);
        }

        void hardware_reset() override
        {
            force_hardware_reset();
        }

        void create_snapshot(std::shared_ptr<debug_interface>& snapshot) const override;
        void enable_recording(std::function<void(const debug_interface&)> record_action) override;
        double get_device_time_ms();

    protected:
        friend class l500_depth_sensor;

        std::shared_ptr<hw_monitor> _hw_monitor;
        uint8_t _depth_device_idx;

        std::unique_ptr<polling_error_handler> _polling_error_handler;

        lazy<std::vector<uint8_t>> _calib_table_raw;
        firmware_version _fw_version;

        std::shared_ptr<stream_interface> _depth_stream;
        std::shared_ptr<stream_interface> _ir_stream;
        std::shared_ptr<stream_interface> _confidence_stream;

        void force_hardware_reset() const;
    };

    class l500_notification_decoder : public notification_decoder
    {
    public:
        notification decode(int value) override;
    };
}
