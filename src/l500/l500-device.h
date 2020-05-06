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
#include "fw-update/fw-update-device-interface.h"

namespace librealsense
{

    class l500_device : public virtual device, public debug_interface, public global_time_interface, public updatable
    {
    public:
        l500_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::shared_ptr<synthetic_sensor> create_depth_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& all_device_infos);

        synthetic_sensor& get_depth_sensor() { return dynamic_cast<synthetic_sensor&>(get_sensor(_depth_device_idx)); }

        uvc_sensor& get_raw_depth_sensor()
        {
            synthetic_sensor& depth_sensor = get_depth_sensor();
            return dynamic_cast<uvc_sensor&>(*depth_sensor.get_raw_sensor());
        }

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
        double get_device_time_ms() override;

        void enter_update_state() const override;
        std::vector<uint8_t> backup_flash(update_progress_callback_ptr callback) override;
        void update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode) override;
        void update_flash_section(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& image, uint32_t offset, uint32_t size, 
            update_progress_callback_ptr callback, float continue_from, float ratio);
        void update_section(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& merged_image, flash_section fs, uint32_t tables_size,
            update_progress_callback_ptr callback, float continue_from, float ratio);
        void update_flash_internal(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& image, std::vector<uint8_t>& flash_backup,
            update_progress_callback_ptr callback, int update_mode);

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
        bool _is_locked = true;

        std::vector<rs2_option> _advanced_options;
    };

    class l500_notification_decoder : public notification_decoder
    {
    public:
        notification decode(int value) override;
    };

    class action_delayer
    {
    public:

        void do_after_delay(std::function<void()> action, int milliseconds = 2000)
        {
            wait(milliseconds);
            action();
            _last_update = std::chrono::system_clock::now();
        }

    private:
        void wait(int milliseconds)
        {
            auto now = std::chrono::system_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - _last_update).count();

            while (diff < milliseconds)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                now = std::chrono::system_clock::now();
                diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - _last_update).count();
            }
        }

        std::chrono::system_clock::time_point _last_update;
    };
}
