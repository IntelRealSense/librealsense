// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "auto_updater/update_handler.h"
#include "ux-window.h"

#include <string>
#include <vector>
#include <regex>
#include <mutex>

namespace rs2
{
    struct version
    {
        int mjor, mnor, patch, build;

        version() : mjor(0), mnor(0), patch(0), build(0) {}

        version(long long ver) : version() 
        {
            build = ver % 10000;
            patch = (ver / 10000) % 100;
            mnor = (ver / 1000000) % 100;
            mjor = (ver / 100000000) % 100;
        }

        version(const std::string& str) : version() 
        {
            std::regex rgx("(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,5})?");
            std::smatch match;

            if (std::regex_search(str.begin(), str.end(), match, rgx) && match.size() >= 4)
            {
                mjor = atoi(std::string(match[1]).c_str());
                mnor = atoi(std::string(match[2]).c_str());
                patch = atoi(std::string(match[3]).c_str());
                if (match.size() == 5)
                    build = atoi(std::string(match[4]).c_str());
            }
        }

        bool operator<=(const version& other) const
        {
            if (mjor > other.mjor) return false;
            if ((mjor == other.mjor) && (mnor > other.mnor)) return false;
            if ((mjor == other.mjor) && (mnor == other.mnor) && (patch > other.patch)) return false;
            if ((mjor == other.mjor) && (mnor == other.mnor) && (patch == other.patch) && (build > other.build)) return false;
            return true;
        }
        bool operator==(const version& other) const
        {
            return (other.mjor == mjor && other.mnor == mnor && other.patch == patch && other.build == build);
        }

        bool operator> (const version& other) const { return !(*this < other); }
        bool operator!=(const version& other) const { return !(*this == other); }
        bool operator<(const version& other) const { return !(*this == other) && (*this <= other); }
        bool operator>=(const version& other) const { return (*this == other) || (*this > other); }

        bool is_between(const version& from, const version& until) const
        {
            return (from <= *this) && (*this <= until);
        }

        operator std::string() const
        {
            return rs2::to_string() << mjor << "." << mnor << "." << patch << "." << build;
        }
    };

    struct update_description
    {
        version ver;
        std::string name;
        std::string release_page;
        std::string download_link;
        std::string description;
    };

    struct update_profile
    {
        std::string device_name;
        std::string serial_number;

        version software_version;
        version firmware_version;

        std::map<version, update_description> software_versions;
        std::map<version, update_description> firmware_versions;
    };

    class updates_model
    {
    public:
        void add_profile(update_profile update)
        {
            std::lock_guard<std::mutex> lock(_lock);
            auto it = std::find_if(_updates.begin(), _updates.end(), [&](update_profile& p){
                return (p.device_name == update.device_name && p.serial_number == update.serial_number);
            });
            if (it == _updates.end()) 
                _updates.push_back(update);
        }

        void draw(ux_window& window, std::string& error_message);

    private:
        int selected_index = 0;
        int selected_software_update_index = 0;
        int selected_firmware_update_index = 0;
        std::vector<update_profile> _updates;
        std::shared_ptr<texture_buffer> _icon = nullptr;
        std::mutex _lock;
    };
}