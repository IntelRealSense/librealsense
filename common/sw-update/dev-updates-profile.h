// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <regex>

#include <librealsense2/rs.hpp>

#include "versions-db-manager.h"

namespace rs2
{
    namespace sw_update
    {
        // The dev_updates_profile class builds and holds a specific device versions profile.
        // It queries the versions DB according to the desired components
        // and supply a complete profile of the device software & firmware available update.
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
                bool dev_active;

                update_profile() :dev_active(true){};

            };

            explicit dev_updates_profile(const device& dev, const std::string &url, const bool use_url_as_local_path = false, http::user_callback_func_type download_callback = http::user_callback_func_type());

            ~dev_updates_profile() {};

            bool retrieve_updates(versions_db_manager::component_part_type comp);
            update_profile & get_update_profile() { return _update_profile; };

        private:
            bool try_parse_update(versions_db_manager& up_handler,
                const std::string& dev_name,
                versions_db_manager::update_policy_type policy,
                versions_db_manager::component_part_type part,
                update_description& result);


            versions_db_manager _versions_db;
            update_profile _update_profile;
            bool _keep_trying;
        };
    }
}
