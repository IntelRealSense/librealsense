// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <unordered_map>
#include <functional>
#include <string>
#include <vector>

namespace rs2
{

    class update_handler
    {
    public:
        typedef std::function<bool(uint64_t dl_current_bytes, uint64_t dl_total_bytes, double dl_time)> user_callback_func_type;

        enum update_policy_type { EXPERIMENTAL, RECOMMENDED, REQUIRED };
        enum component_part_type { LIBREALSENSE , VIEWER, DEPTH_QUALITY_TOOL, FIRMWARE };
        enum update_source_type { FROM_FILE, FROM_SERVER };

        explicit update_handler(const std::string &url, const bool use_url_as_local_path = false, user_callback_func_type download_callback = user_callback_func_type()) : _dev_info_url(url), _local_source_file(use_url_as_local_path), _server_versions_vec(), _server_versions_loaded(false) , _download_cb_func(download_callback){};
        ~update_handler() {};
        
        bool query_versions(const std::string &dev_name, const component_part_type cp, const update_policy_type up, long long &version);
        bool get_ver_download_link(const component_part_type cp, const long long version, std::string& dl_link) { return get_ver_data(cp, version, "link", dl_link); };
        bool get_ver_rel_notes(const component_part_type cp, const long long version, std::string& ver_rel_notes_link) { return get_ver_data(cp, version, "release_notes_link", ver_rel_notes_link); };
        bool get_ver_description(const component_part_type cp, const long long version, std::string& ver_desc) { return get_ver_data(cp, version, "description", ver_desc); };
        std::string convert_ver_to_str(const long long ver_num) const;

        std::string convert_component_name(const component_part_type& comp) const;
        std::string convert_update_policy(const update_policy_type& pol) const;

    private:
        struct server_version_type
        {
            std::string policy;
            std::string component;
            std::string version;
            std::string platform;
            std::string link;
            std::string device_name;
            std::string rel_notes_link;
            std::string desc;


            server_version_type() :
                policy(),
                component(),
                version(),
                platform(),
                link(),
                device_name() ,
                rel_notes_link(), 
                desc(){}
        };

        bool get_server_data(std::stringstream &ver_data);
        void parse_versions_data(const std::stringstream &ver_data);
        bool get_ver_data(const component_part_type cp, const long long version, const std::string& req_field, std::string& out);
        
        const std::string get_platform() const;
        bool init();
        void build_schema(std::unordered_map<std::string, std::function<bool(const std::string&)>>& verifier);


        const std::string _dev_info_url;
        bool  _local_source_file;
        bool _server_versions_loaded;
        std::vector<std::unordered_map<std::string, std::string>> _server_versions_vec;
        user_callback_func_type _download_cb_func;
    };
}