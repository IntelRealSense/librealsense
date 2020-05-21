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
                std::regex rgx("^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,5})?$");
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
                std::stringstream ss;
                ss << mjor << "." << mnor << "." << patch << "." << build;
                return ss.str();
            }
        };

        enum update_policy_type { EXPERIMENTAL, RECOMMENDED, ESSENTIAL };
        enum component_part_type { LIBREALSENSE, VIEWER, DEPTH_QUALITY_TOOL, FIRMWARE };
        enum update_source_type { FROM_FILE, FROM_SERVER };

        explicit versions_db_manager(const std::string &url, const bool use_url_as_local_path = false, user_callback_func_type download_callback = user_callback_func_type())
            : _dev_info_url(url), _local_source_file(use_url_as_local_path), _server_versions_vec(), _server_versions_loaded(false), _download_cb_func(download_callback) {};

        ~versions_db_manager() {};

        bool query_versions(const std::string &dev_name, const component_part_type cp, const update_policy_type up, version&version);
        bool get_version_download_link(const component_part_type cp, const version& version, std::string& dl_link) { return get_ver_data(cp, version, "link", dl_link); };
        bool get_version_release_notes(const component_part_type cp, const version& version, std::string& ver_rel_notes_link) { return get_ver_data(cp, version, "release_notes_link", ver_rel_notes_link); };
        bool get_version_description(const component_part_type cp, const version& version, std::string& ver_desc) { return get_ver_data(cp, version, "description", ver_desc); };
        std::string convert_version_to_formatted_string(const long long ver_num) const;

        std::string to_string(const component_part_type& comp) const;
        std::string to_string(const update_policy_type& pol) const;
        bool from_string(std::string name, component_part_type& res) const;
        bool from_string(std::string name, update_policy_type& res) const;

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
        bool get_ver_data(const component_part_type cp, const version& version, const std::string& req_field, std::string& out);

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