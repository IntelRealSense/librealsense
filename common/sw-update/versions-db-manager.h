// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <unordered_map>
#include <functional>
#include <string>
#include <vector>

#include "http-downloader.h"

namespace rs2
{

    namespace sw_update
    {
        // The version_db_manager class download, parse and supply queries for the RS components versions information.
        // The version file can be stored locally on the file system or on a HTTP server.
        class versions_db_manager
        {
        public:
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
                    constexpr int MINIMAL_MATCH_SECTIONS = 4;
                    constexpr int MATCH_SECTIONS_INC_BUILD_NUM = 5;
                    std::regex rgx("^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,5})?$");
                    std::smatch match;

                    if (std::regex_search(str.begin(), str.end(), match, rgx) && match.size() >= MINIMAL_MATCH_SECTIONS)
                    {
                        mjor = atoi(std::string(match[1]).c_str());
                        mnor = atoi(std::string(match[2]).c_str());
                        patch = atoi(std::string(match[3]).c_str());
                        if (match.size() == MATCH_SECTIONS_INC_BUILD_NUM)
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

                bool operator> (const version& other) const { return !(*this <= other); }
                bool operator!=(const version& other) const { return !(*this == other); }
                bool operator>=(const version& other) const { return (*this == other) || (*this > other); }
                bool operator<(const version& other) const { return !(*this >= other); }

                bool is_between(const version& from, const version& until) const
                {
                    return (from <= *this) && (*this <= until);
                }

                operator std::string() const
                {
                    std::stringstream ss;
                    ss << mjor << "." << mnor << "." << patch << "." << build;
                    return ss.str();
                }
            };

            enum update_policy_type { EXPERIMENTAL, RECOMMENDED, ESSENTIAL };
            enum component_part_type { LIBREALSENSE, VIEWER, DEPTH_QUALITY_TOOL, FIRMWARE };
            enum update_source_type { FROM_FILE, FROM_SERVER };
            enum query_status_type { VERSION_FOUND, NO_VERSION_FOUND, DB_LOAD_FAILURE };

            explicit versions_db_manager(const std::string &url, const bool use_url_as_local_path = false, http::user_callback_func_type download_callback = http::user_callback_func_type())
                : _dev_info_url(url), _local_source_file(use_url_as_local_path), _server_versions_vec(), _server_versions_loaded(false), _download_cb_func(download_callback) {};

            ~versions_db_manager() {};

            query_status_type query_versions(const std::string &device_name, component_part_type component, const update_policy_type policy, version& out_version);
            bool get_version_download_link(const component_part_type component, const version& version, std::string& dl_link) { return get_version_data_common(component, version, "link", dl_link); };
            bool get_version_release_notes(const component_part_type component, const version& version, std::string& version_release_notes_link) { return get_version_data_common(component, version, "release_notes_link", version_release_notes_link); };
            bool get_version_description(const component_part_type component, const version& version, std::string& version_description) { return get_version_data_common(component, version, "description", version_description); };

            std::string to_string(const component_part_type& component) const;
            std::string to_string(const update_policy_type& policy) const;
            bool from_string(std::string component_str, component_part_type& component_val) const;
            bool from_string(std::string policy_str, update_policy_type& policy_val) const;

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
                    device_name(),
                    rel_notes_link(),
                    desc() {}
            };

            bool get_server_data(std::stringstream &ver_data);
            void parse_versions_data(const std::stringstream &ver_data);
            bool get_version_data_common(const component_part_type component, const version& version, const std::string& req_field, std::string& out);
            bool is_device_name_equal(const std::string &str_from_db, const std::string &str_compared, bool allow_wildcard);
            

            bool init();
            void build_schema(std::unordered_map<std::string, std::function<bool(const std::string&)>>& verifier);


            const std::string _dev_info_url;
            bool  _local_source_file;
            bool _server_versions_loaded;
            std::vector<std::unordered_map<std::string, std::string>> _server_versions_vec;
            http::user_callback_func_type _download_cb_func;
        };
    }
}
