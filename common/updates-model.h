// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "sw-update/dev-updates-profile.h"
#include "ux-window.h"
#include "notifications.h"
#include "fw-update-helper.h"

#include <string>
#include <vector>
#include <regex>
#include <mutex>
#include <atomic>

namespace rs2
{
    class updates_model
    {
    public:
        struct update_profile_model
        {
            sw_update::dev_updates_profile::update_profile profile;
            context ctx;
            device_model* dev_model;
            update_profile_model(sw_update::dev_updates_profile::update_profile p,
                context contex, device_model* device_model) : profile(p), ctx(contex), dev_model(device_model) {};
        };
        void add_profile(const update_profile_model& update)
        {
            std::lock_guard<std::mutex> lock(_lock);
            auto it = std::find_if(_updates.begin(), _updates.end(), [&](update_profile_model& p) {
                return (p.profile.device_name == update.profile.device_name && p.profile.serial_number == update.profile.serial_number);
            });
            if (it == _updates.end())
                _updates.push_back(update);
            else
            {
                *it = update;
            }
        }

        void update_profile(const update_profile_model& update)
        {
            std::lock_guard<std::mutex> lock(_lock);
            auto it = std::find_if(_updates.begin(), _updates.end(), [&](update_profile_model& p) {
                return (p.profile.device_name == update.profile.device_name && p.profile.serial_number == update.profile.serial_number);
            });
            if (it != _updates.end())
            {
                *it = update;
            }
        }
        void remove_profile(const sw_update::dev_updates_profile::update_profile &update)
        {
            std::lock_guard<std::mutex> lock(_lock);
            auto it = std::find_if(_updates.begin(), _updates.end(), [&](update_profile_model& p) {
                return (p.profile.device_name == update.device_name && p.profile.serial_number == update.serial_number);
            });
            if (it != _updates.end())
                _updates.erase(it);
        }

        // This is a helper function to indicate if a device is connected or not.
        // It change its value on device connect/disconnect
        // cause calling ctx.query_devices() is too slow for the UI
        void set_device_status(const sw_update::dev_updates_profile::update_profile &update, bool active)
        {
            std::lock_guard<std::mutex> lock(_lock);
            auto it = std::find_if(_updates.begin(), _updates.end(), [&](update_profile_model& p) {
                return (p.profile.device_name == update.device_name && p.profile.serial_number == update.serial_number);
            });
            if (it != _updates.end())
                it->profile.dev_active = active;
        }

        void draw(viewer_model& viewer, ux_window& window, std::string& error_message);
    private:
        struct position_params
        {
            float w;
            float h;
            ImVec2 orig_pos;
            float mid_y;
            float x0;
            float y0;
            position_params() :w(0.0f), h(0.0f), orig_pos(), x0(0.0f), y0(0.0f) {}
        };

        bool draw_software_section(const char * window_name, update_profile_model& selected_profile, position_params& pos_params , ux_window& window, std::string& error_message);
        bool draw_firmware_section(viewer_model& viewer, const char * window_name, update_profile_model& selected_profile, position_params& pos_params, ux_window& window, std::string& error_message);


        int selected_index = 0;
        int selected_software_update_index = 0;
        int selected_firmware_update_index = 0;
        bool ignore = false;
        std::vector<update_profile_model> _updates;
        std::shared_ptr<texture_buffer> _icon = nullptr;
        std::mutex _lock;
        bool emphasize_dismiss_text = false;

        std::shared_ptr<firmware_update_manager> _fw_update = nullptr;

        enum class fw_update_states
        {
            ready = 0,
            downloading = 1,
            started = 2,
            completed = 3,
            failed_downloading = 4,
            failed_updating = 5
        };

        enum class fw_update_fail_reason
        {
            none = 0,
            downloading_error = 1,
            updating_error = 2
        };

        fw_update_states _fw_update_state = fw_update_states::ready;

        progress_bar _progress;
        std::atomic<int> _fw_download_progress{ 0 };
        bool _retry = false;
        std::shared_ptr<firmware_update_manager> _update_manager = nullptr;
        std::vector<uint8_t> _fw_image;
    };
}
