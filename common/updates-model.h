// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "auto-updater/dev-updates-profile.h"
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
            dev_updates_profile::update_profile profile;
            context ctx;
            device_model* dev_model;
            update_profile_model(dev_updates_profile::update_profile p,
                context contex, device_model* device_model) : profile(p), ctx(contex), dev_model(device_model){};
        };
        void add_profile(update_profile_model update)
        {
            std::lock_guard<std::mutex> lock(_lock);
            auto it = std::find_if(_updates.begin(), _updates.end(), [&](update_profile_model& p){
                return (p.profile.device_name == update.profile.device_name && p.profile.serial_number == update.profile.serial_number);
            });
            if (it == _updates.end()) 
                _updates.push_back(update);
        }
        void remove_profile(const update_profile_model &update)
        {
            std::lock_guard<std::mutex> lock(_lock);
            auto it = std::find_if(_updates.begin(), _updates.end(), [&](update_profile_model& p) {
                return (p.profile.device_name == update.profile.device_name && p.profile.serial_number == update.profile.serial_number);
            });
            if (it != _updates.end())
                _updates.erase(it);
        }

        void draw(ux_window& window, std::string& error_message);

    private:
        int selected_index = 0;
        int selected_software_update_index = 0;
        int selected_firmware_update_index = 0;
        bool ignore = false;
        std::vector<update_profile_model> _updates;
        std::shared_ptr<texture_buffer> _icon = nullptr;
        std::mutex _lock;

        std::shared_ptr<firmware_update_manager> _fw_update = nullptr;

        enum class fw_update_states
        {
            ready = 0,
            downloading = 1,
            started = 2,
            completed = 3,
            failed = 4
        };
        fw_update_states _fw_update_state = fw_update_states::ready;

        progress_bar _progress;
        std::atomic<int> _fw_download_progress { 0 };

        std::shared_ptr<firmware_update_manager> _update_manager = nullptr;
        std::vector<uint8_t> _fw_image;
    };
}
