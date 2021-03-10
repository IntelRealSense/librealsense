// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "dev-updates-profile.h"

namespace rs2
{
    namespace sw_update
    {
        using namespace http;

        dev_updates_profile::dev_updates_profile(const device& dev, const std::string &url, const bool use_url_as_local_path, user_callback_func_type download_callback)
            : _versions_db(url, use_url_as_local_path, download_callback), _keep_trying(true)
        {

            std::string dev_name = (dev.supports(RS2_CAMERA_INFO_NAME)) ? dev.get_info(RS2_CAMERA_INFO_NAME) : "Unknown";
            std::string serial = (dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER)) ? dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) : "Unknown";
            std::string firmware_ver = (dev.supports(RS2_CAMERA_INFO_FIRMWARE_VERSION)) ? dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) : "0.0.0";

            _update_profile.software_version = versions_db_manager::version(RS2_API_FULL_VERSION_STR);
            _update_profile.firmware_version = versions_db_manager::version(firmware_ver);

            _update_profile.dev = dev;

            _update_profile.device_name = dev_name;
            _update_profile.serial_number = serial;
        }

        bool dev_updates_profile::retrieve_updates(versions_db_manager::component_part_type comp)
        {
            bool update_required(false);

            if (_update_profile.device_name.find("Recovery") == std::string::npos)
            {
                std::map<versions_db_manager::version, update_description> &versions_vec((comp == versions_db_manager::FIRMWARE) ?
                    _update_profile.firmware_versions : _update_profile.software_versions);

                versions_db_manager::version &current_version((comp == versions_db_manager::FIRMWARE) ? _update_profile.firmware_version : _update_profile.software_version);
                {
                    update_description experimental_update;
                    if (try_parse_update(_versions_db, _update_profile.device_name, versions_db_manager::EXPERIMENTAL, comp, experimental_update))
                    {
                        versions_vec[experimental_update.ver] = experimental_update;
                    }
                    update_description recommened_update;
                    if (try_parse_update(_versions_db, _update_profile.device_name, versions_db_manager::RECOMMENDED, comp, recommened_update))
                    {
                        versions_vec[recommened_update.ver] = recommened_update;
                    }
                    update_description required_update;
                    if (try_parse_update(_versions_db, _update_profile.device_name, versions_db_manager::ESSENTIAL, comp, required_update))
                    {
                        versions_vec[required_update.ver] = required_update;
                        // Ignore version zero as an indication of Recovery mode.
                        update_required = update_required || (current_version < required_update.ver);
                    }
                }
            }

            return update_required;
        }

        bool dev_updates_profile::try_parse_update(versions_db_manager& up_handler,
            const std::string& dev_name,
            versions_db_manager::update_policy_type policy,
            versions_db_manager::component_part_type part,
            dev_updates_profile::update_description& result)
        {
            if (_keep_trying)
            {
                versions_db_manager::version required_version;
                auto query_status = up_handler.query_versions(dev_name, part, policy, required_version);
                if (query_status == versions_db_manager::VERSION_FOUND)
                {
                    up_handler.get_version_download_link(part, required_version, result.download_link);
                    up_handler.get_version_release_notes(part, required_version, result.release_page);
                    up_handler.get_version_description(part, required_version, result.description);
                    result.ver = required_version;

                    std::stringstream ss;
                    ss << std::string(result.ver) << " (" << up_handler.to_string(policy) << ")";
                    result.name = ss.str();
                    return true;
                }
                else if (query_status == versions_db_manager::DB_LOAD_FAILURE)
                {
                    _keep_trying = false;
                }

            }
            return false;
        }
    }
}
