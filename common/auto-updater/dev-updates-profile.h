// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <regex>

#include <librealsense2/rs.hpp>

#include "versions-db-manager.h"

namespace rs2
{
    class dev_updates_profile
    {
    public:

        struct update_description
        {
            versions_db_manager::version ver;
            std::string name;
            std::string release_page;
            std::string download_link;
            std::string description;
        };

        struct update_profile
        {
            std::string device_name;
            std::string serial_number;

            versions_db_manager::version software_version;
            versions_db_manager::version firmware_version;

            std::map<versions_db_manager::version, update_description> software_versions;
            std::map<versions_db_manager::version, update_description> firmware_versions;

            device dev;
        };

        explicit dev_updates_profile(const device& dev, const std::string &url, const bool use_url_as_local_path = false, user_callback_func_type download_callback = user_callback_func_type());

        ~dev_updates_profile() {};

        bool check_for_updates();
        update_profile & get_update_profile() { return _update_profile; };

    private:
        bool try_parse_update(versions_db_manager& up_handler,
            const std::string& dev_name,
            versions_db_manager::update_policy_type policy,
            versions_db_manager::component_part_type part,
            update_description& result);


        versions_db_manager _versions_db;
        update_profile _update_profile;
    };
}