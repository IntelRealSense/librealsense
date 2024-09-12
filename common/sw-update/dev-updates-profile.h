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

            struct version_info
            {
                sw_update::version ver;
                std::string name_for_display;
                std::string release_page;
                std::string download_link;
                std::string description;
                update_policy_type policy;
            };

            struct update_profile
            {
                std::string device_name;
                std::string serial_number;
                std::string fw_update_id;

                sw_update::version software_version;
                sw_update::version firmware_version;

                typedef std::map< sw_update::version, version_info > version_to_info;
                version_to_info software_versions;
                version_to_info firmware_versions;

                device dev;
                bool dev_active;

                update_profile() :dev_active(true){};

                bool get_sw_update(update_policy_type policy, version_info& info) const;
                bool get_fw_update(update_policy_type policy, version_info& info) const;

            };

            explicit dev_updates_profile(const device& dev, const std::string &url, const bool use_url_as_local_path = false, http::user_callback_func_type download_callback = http::user_callback_func_type());

            ~dev_updates_profile() {};

            bool retrieve_updates(component_part_type comp, bool& fail_access_db);
            update_profile & get_update_profile() { return _update_profile; };

        private:
            query_status_type try_parse_update(versions_db_manager& up_handler,
                const std::string& dev_name,
                update_policy_type policy,
                component_part_type part,
                version_info& result);


            versions_db_manager _versions_db;
            update_profile _update_profile;
            bool _keep_trying;
        };
    }
}
