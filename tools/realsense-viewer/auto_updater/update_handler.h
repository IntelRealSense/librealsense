// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

namespace rs2
{
    class update_handler
    {
    public:
        enum update_policy_type { EXPERIMENTAL, RECOMMENDED, REQUIRED};
        enum component_part_type  { LIBREALSENSE , VIEWER, DEPTH_QUALITY_TOOL, FIRMWARE};

        update_handler() : update_handler("") {}; // Temp Read From local file - REMOVE!
        update_handler(const std::string &url) : _dev_info_url(url), _server_versions_vec(), _server_versions_loaded(false) {};
        ~update_handler() {};

        
        long long check_for_updates(const component_part_type cp, const update_policy_type up);
        bool get_download_link(const component_part_type part, const long long version, std::string& dl_link);

    private:
        struct server_version_type
        {
            update_policy_type policy;
            component_part_type component;
            long long version;
            std::string platform;
            std::string link;

            server_version_type() :
                policy(RECOMMENDED),
                component(LIBREALSENSE),
                version(0),
                platform(),
                link() {}

        };

        bool get_server_data(std::stringstream &ver_data);
        void parse_versions_data(const std::stringstream &ver_data);
        bool convert_component_name(const std::string& comp ,component_part_type &out) const;
        bool convert_update_policy(const std::string& comp, update_policy_type &out) const;
        bool verify_platform(const std::string& pl) const;
        const std::string get_platform() const;
        bool load_server_versions();


        const std::string _dev_info_url;
        bool _server_versions_loaded;
        std::vector< server_version_type> _server_versions_vec;
    };
}