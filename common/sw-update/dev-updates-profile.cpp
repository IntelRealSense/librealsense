// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "dev-updates-profile.h"

namespace rs2
{
    namespace sw_update
    {
        using namespace http;

        dev_updates_profile::dev_updates_profile(const device& dev, const std::string& url, const bool use_url_as_local_path, user_callback_func_type download_callback)
            : _versions_db(url, use_url_as_local_path, download_callback), _keep_trying(true)
        {

            std::string dev_name = (dev.supports(RS2_CAMERA_INFO_NAME)) ? dev.get_info(RS2_CAMERA_INFO_NAME) : "Unknown";
            std::string serial = (dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER)) ? dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) : "Unknown";

            std::string fw_update_id = "Unknown";

            if (dev.supports(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID))
            {
                fw_update_id = dev.get_info(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID);
            }

            std::string firmware_ver = (dev.supports(RS2_CAMERA_INFO_FIRMWARE_VERSION)) ? dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) : "0.0.0";

            _update_profile.software_version = sw_update::version(RS2_API_FULL_VERSION_STR);
            _update_profile.firmware_version = sw_update::version(firmware_ver);

            _update_profile.dev = dev;

            _update_profile.device_name = dev_name;
            _update_profile.serial_number = serial;
            _update_profile.fw_update_id = fw_update_id;
        }

        bool dev_updates_profile::retrieve_updates(component_part_type comp, bool& fail_access_db)
        {
            // throw on unsupported components (Not implemented yet)
            if (comp != LIBREALSENSE && comp != FIRMWARE)
            {
                std::string error_str("update component: " + sw_update::to_string(comp) + " not supported");
                throw std::runtime_error(error_str);
            }

            bool update_available(false);

            auto &versions_vec((comp == FIRMWARE) ?
                _update_profile.firmware_versions : _update_profile.software_versions);

            version& current_version((comp == FIRMWARE) ? _update_profile.firmware_version : _update_profile.software_version);
            {
                version_info experimental_update;
                auto parse_update_stts = try_parse_update(_versions_db, _update_profile.device_name, EXPERIMENTAL, comp, experimental_update);
                if ( parse_update_stts == VERSION_FOUND)
                {
                    if (current_version < experimental_update.ver)
                    {
                        versions_vec[experimental_update.ver] = experimental_update;
                        update_available = true;
                    }
                }

                version_info recommened_update;
                if (try_parse_update(_versions_db, _update_profile.device_name, RECOMMENDED, comp, recommened_update) == VERSION_FOUND)
                {
                    if (current_version < recommened_update.ver)
                    {
                        versions_vec[recommened_update.ver] = recommened_update;
                        update_available = true;
                    }
                }

                version_info required_update;
                if (try_parse_update(_versions_db, _update_profile.device_name, ESSENTIAL, comp, required_update) == VERSION_FOUND)
                {
                    if (current_version < required_update.ver)
                    {
                        versions_vec[required_update.ver] = required_update;
                        update_available = true;
                    }
                }

                fail_access_db = (parse_update_stts == DB_LOAD_FAILURE);
            }
            return update_available;
        }

        query_status_type dev_updates_profile::try_parse_update(versions_db_manager& up_handler,
            const std::string& dev_name,
            update_policy_type policy,
            component_part_type part,
            dev_updates_profile::version_info& result)
        {
            query_status_type query_status = DB_LOAD_FAILURE;
            if (_keep_trying)
            {
                sw_update::version required_version;
                query_status = up_handler.query_versions(dev_name, part, policy, required_version);
                if (query_status == VERSION_FOUND)
                {
                    up_handler.get_version_download_link(part, required_version, result.download_link);
                    up_handler.get_version_release_notes(part, required_version, result.release_page);
                    up_handler.get_version_description(part, required_version, result.description);
                    result.ver = required_version;

                    std::stringstream ss;
                    ss << std::string(result.ver) << " (" << sw_update::to_string(policy) << ")";
                    result.name_for_display = ss.str();
                    result.policy = policy;
                }
                else if (query_status == DB_LOAD_FAILURE)
                {
                    _keep_trying = false;
                }

            }
            return query_status;
        }

        bool dev_updates_profile::update_profile::get_sw_update( update_policy_type policy,
                                                                 version_info & info ) const
        {
            auto found = std::find_if( software_versions.begin(),
                                       software_versions.end(),
                                       [policy]( const version_to_info::value_type & ver_info ) {
                                           return ver_info.second.policy == policy;
                                       } );
            if( found != software_versions.end() )
            {
                info = found->second;
                return true;
            }
            return false;
        }

        bool dev_updates_profile::update_profile::get_fw_update( update_policy_type policy,
                                                                 version_info & info ) const
        {
            auto found = std::find_if( firmware_versions.begin(),
                                       firmware_versions.end(),
                                       [policy]( const version_to_info::value_type & ver_info ) {
                                           return ver_info.second.policy == policy;
                                       } );
            if( found != firmware_versions.end() )
            {
                info = found->second;
                return true;
            }
            return false;
        }
    }
}
