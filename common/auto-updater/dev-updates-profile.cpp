// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "dev-updates-profile.h"

namespace rs2
{

    dev_updates_profile::dev_updates_profile(const device& dev, const std::string &url, const bool use_url_as_local_path, user_callback_func_type download_callback)
            : _versions_db(url, use_url_as_local_path, download_callback)
        {

            std::string dev_name = (dev.supports(RS2_CAMERA_INFO_NAME)) ? dev.get_info(RS2_CAMERA_INFO_NAME) : "Unknown";
            std::string serial = (dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER)) ? dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) : "Unknown";
            std::string firmware_ver = (dev.supports(RS2_CAMERA_INFO_FIRMWARE_VERSION)) ? dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) : "0.0.0";

            _update_profile.software_version = versions_db_manager::version(RS2_API_FULL_VERSION_STR);
            _update_profile.firmware_version = versions_db_manager::version(firmware_ver);

            _update_profile.dev = dev;

            _update_profile.device_name = dev_name;
            _update_profile.serial_number = serial;
        };

    bool dev_updates_profile::check_for_updates()
    {
        bool update_required(false);

        {
            update_description experimental_software_update;
            if (try_parse_update(_versions_db, _update_profile.device_name, versions_db_manager::EXPERIMENTAL, versions_db_manager::LIBREALSENSE, experimental_software_update))
            {
                _update_profile.software_versions[experimental_software_update.ver] = experimental_software_update;
            }
            update_description recommened_software_update;
            if (try_parse_update(_versions_db, _update_profile.device_name, versions_db_manager::RECOMMENDED, versions_db_manager::LIBREALSENSE, recommened_software_update))
            {
                _update_profile.software_versions[recommened_software_update.ver] = recommened_software_update;
            }
            update_description required_software_update;
            if (try_parse_update(_versions_db, _update_profile.device_name, versions_db_manager::ESSENTIAL, versions_db_manager::LIBREALSENSE, required_software_update))
            {
                _update_profile.software_versions[required_software_update.ver] = required_software_update;
                update_required = update_required || (_update_profile.software_version < required_software_update.ver);
            }
        }

        {
            update_description experimental_firmware_update;
            if (try_parse_update(_versions_db, _update_profile.device_name, versions_db_manager::EXPERIMENTAL, versions_db_manager::FIRMWARE, experimental_firmware_update))
            {
                _update_profile.firmware_versions[experimental_firmware_update.ver] = experimental_firmware_update;
            }
            update_description recommened_firmware_update;
            if (try_parse_update(_versions_db, _update_profile.device_name, versions_db_manager::RECOMMENDED, versions_db_manager::FIRMWARE, recommened_firmware_update))
            {
                _update_profile.firmware_versions[recommened_firmware_update.ver] = recommened_firmware_update;
            }
            update_description required_firmware_update;
            if (try_parse_update(_versions_db, _update_profile.device_name, versions_db_manager::ESSENTIAL, versions_db_manager::FIRMWARE, required_firmware_update))
            {
                _update_profile.firmware_versions[required_firmware_update.ver] = required_firmware_update;
                update_required = update_required || (_update_profile.firmware_version < required_firmware_update.ver);
            }
        }

        if (update_required)
        {
            if (!_update_profile.firmware_versions.size())
                _update_profile.firmware_versions[versions_db_manager::version(0)] = update_description{ versions_db_manager::version(0), "N/A", "", "", "" };

            if (!_update_profile.software_versions.size())
                _update_profile.software_versions[versions_db_manager::version(0)] = update_description{ versions_db_manager::version(0), "N/A", "", "", "" };
        }
        return update_required;
    }

    bool dev_updates_profile::try_parse_update(versions_db_manager& up_handler,
        const std::string& dev_name,
        versions_db_manager::update_policy_type policy,
        versions_db_manager::component_part_type part,
        dev_updates_profile::update_description& result)
    {
        versions_db_manager::version required_version;
        bool query_ok = up_handler.query_versions(dev_name, part, policy, required_version);
        if (query_ok)
        {
            auto dl_link_ok = up_handler.get_version_download_link(part, required_version, result.download_link);
            auto rel_ok = up_handler.get_version_release_notes(part, required_version, result.release_page);
            auto desc_ok = up_handler.get_version_description(part, required_version, result.description);
            result.ver = required_version;

            std::stringstream ss;
            ss << std::string(result.ver) << " (" << up_handler.to_string(policy) << ")";
            result.name = ss.str();
        }
        return query_ok;
    }

}
