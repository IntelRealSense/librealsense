// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <mutex>
#include <string>
#include "../device.h"
#include "../context.h"
#include "../backend.h"
#include "../hw-monitor.h"
#include "../image.h"
#include "../stream.h"
#include "l500-private.h"
#include "../error-handling.h"
#include "../global_timestamp_reader.h"
#include "../fw-update/fw-update-device-interface.h"

namespace librealsense
{
    class l500_depth_sensor;
    class l500_color_sensor;

    class l500_device
        : public virtual device
        , public debug_interface
        , public global_time_interface
        , public updatable
    {
    public:
        l500_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::shared_ptr<synthetic_sensor> create_depth_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& all_device_infos);

        virtual void configure_depth_options();

        virtual l500_color_sensor * get_color_sensor() = 0;

        synthetic_sensor & get_synthetic_depth_sensor() { return dynamic_cast< synthetic_sensor &>(get_sensor( _depth_device_idx )); }
        l500_depth_sensor & get_depth_sensor();
        uvc_sensor& get_raw_depth_sensor()
        {
            synthetic_sensor& depth_sensor = get_synthetic_depth_sensor();
            return dynamic_cast<uvc_sensor&>(*depth_sensor.get_raw_sensor());
        }

        std::vector< uint8_t > send_receive_raw_data(const std::vector< uint8_t > & input) override;

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
        bool check_fw_compatibility(const std::vector<uint8_t>& image) const override;

        ivcam2::extended_temperatures get_temperatures() const;


    protected:
        void start_temperatures_reader();
        void stop_temperatures_reader();

        friend class l500_depth_sensor;

        std::shared_ptr<hw_monitor> _hw_monitor;
        uint8_t _depth_device_idx;
        uint16_t _pid;

        std::shared_ptr<polling_error_handler> _polling_error_handler;

        lazy<ivcam2::intrinsic_depth> _calib_table;
        firmware_version _fw_version;
        std::shared_ptr<stream_interface> _depth_stream;
        std::shared_ptr<stream_interface> _ir_stream;
        std::shared_ptr<stream_interface> _confidence_stream;
        
        void force_hardware_reset() const;
        bool _is_locked = true;

        //TODO - add these to device class as pure virtual methods
        command get_firmware_logs_command() const;
        command get_flash_logs_command() const;

        std::vector<rs2_option> _advanced_options;

        platform::usb_spec _usb_mode;

        mutable std::mutex _temperature_mutex;
        std::atomic_bool _keep_reading_temperature{ false };  // If true temperature reading thread is working, otherwise indicate to the thread to stop
        std::atomic_bool _have_temperatures{ false }; //  If true, then the _temperatures are valid and up-to-date.
        std::thread _temperature_reader;
        ivcam2::extended_temperatures _temperatures;
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
