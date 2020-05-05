// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#include <fstream>
#include <unordered_set>
#include <algorithm>

#include "json.hpp"
#include "update_handler.h"
#include "http_downloader.h"

namespace rs2
{
    using json = nlohmann::json;

    long long update_handler::check_for_updates(const component_part_type cp, const update_policy_type up)
    {
        // Load server versions info on first access
        if (!_server_versions_loaded)
        {
            if (load_server_versions())
            {
                _server_versions_loaded = true;
            }
            else
            {
                return 0; // Cannot download server file
            }
        }

        auto platform = get_platform();

        // Look for the required version
        auto res = std::find_if(_server_versions_vec.begin(), _server_versions_vec.end(),
            [cp, up, platform](server_version_type ver) {return ((ver.component == cp) && (ver.policy == up) && (ver.platform == platform)); });

        if (res != _server_versions_vec.end())
        {
            return (*res).version;
        }

        return 0; // Nothing found
    }

    bool update_handler::get_download_link(const component_part_type cp, const long long version, std::string& dl_link)
    {
        // Load server versions info on first access
        if (!_server_versions_loaded)
        {
            if (load_server_versions())
            {
                _server_versions_loaded = true;
            }
            else
            {
                return 0; // Cannot download server file
            }
        }

        std::string platform = get_platform();

        // Look for the required version
        auto res = std::find_if(_server_versions_vec.begin(), _server_versions_vec.end(),
            [cp, version, platform](server_version_type ver) {return ((ver.component == cp) && (ver.version == version) && (ver.platform == platform)); });

        if (res != _server_versions_vec.end())
        {
            dl_link = (*res).link;
            return true;
        }

        return false;
    }

    bool update_handler::convert_component_name(const std::string& comp, component_part_type &out) const
    {
        bool convert_ok(true);
        if (comp == "LIBREALSENSE") out = LIBREALSENSE;
        else if (comp == "VIEWER") out = VIEWER;
        else if (comp == "FIRMWARE") out = FIRMWARE;
        else convert_ok = false;

        return convert_ok;
    }

    bool update_handler::convert_update_policy(const std::string& comp, update_policy_type &out) const
    {
        bool convert_ok(true);
        if (comp == "EXPERIMENTAL") out = EXPERIMENTAL;
        else if (comp == "RECOMMENDED") out = RECOMMENDED;
        else if (comp == "REQUIRED") out = REQUIRED;
        else convert_ok = false;

        return convert_ok;
    }

    bool update_handler::get_server_data(std::stringstream &ver_data)
    {
        bool server_data_retrieved(false);

        if (!_dev_info_url.empty())
        {
            http_downloader hd;
            if (hd.download_to_stream(_dev_info_url, ver_data))
            {
                server_data_retrieved = true;
            }
        }
        else
        {
            // Temporary TBR
            //read from local file
            std::ifstream file("rs_updater.json");
            if (file.good())
            {
                server_data_retrieved = true;
            }
            ver_data << file.rdbuf();
        }
        return server_data_retrieved;
    }

    void update_handler::parse_versions_data(const std::stringstream &ver_data)
    {
        struct json_schema_verifier_type
        {
            int comp_cnt;
            int policy_cnt;
            int version_cnt;
            int platform_cnt;
            int link_cnt;

            json_schema_verifier_type() : comp_cnt(0), policy_cnt(0), version_cnt(0), platform_cnt(0), link_cnt(0) {}
            void reset() { comp_cnt = policy_cnt = version_cnt = platform_cnt = link_cnt = 0; }
            bool validate() { return (comp_cnt && policy_cnt && version_cnt && platform_cnt && link_cnt); }
        } json_schema_verifier;

        json j = json::parse(ver_data.str());

        // Validate json file title
        auto title_found = j.find("versions");
        if (title_found != j.end())
        {
            // Iterate through the versions
            for (auto &ver : j["versions"])
            {   // Iterate through the version fields
                server_version_type version_info;
                json_schema_verifier.reset();
                for (auto it = ver.begin(); it != ver.end(); ++it)
                {
                    auto cvt_ok(true);
                    std::string element_key = it.key();
                    if (element_key == "component")
                    {
                        convert_component_name(it.value().get<std::string>(), version_info.component) ?
                            json_schema_verifier.comp_cnt++ :
                            cvt_ok = false;
                    }
                    else if (element_key == "policy_type")
                    {
                        convert_update_policy(it.value().get<std::string>(), version_info.policy) ?
                            json_schema_verifier.policy_cnt++ :
                            cvt_ok = false;
                    }
                    else if (element_key == "version")
                    {
                        version_info.version = it.value().get<long long>();
                        json_schema_verifier.version_cnt++;
                    }
                    else if (element_key == "platform")
                    {
                        if (verify_platform(it.value().get<std::string>()))
                        {
                            version_info.platform = it.value().get<std::string>();
                            json_schema_verifier.platform_cnt++;
                        }
                    }
                    else if (element_key == "link")
                    {
                        version_info.link = it.value().get<std::string>();
                        json_schema_verifier.link_cnt++;
                    }
                    else
                    {
                        cvt_ok = false;
                    }

                    if (!cvt_ok)
                    {
                        throw std::runtime_error("Server versions json file corrupted\n");
                    }
                }
                if (json_schema_verifier.validate())
                {
                    _server_versions_vec.emplace_back(version_info);
                    json_schema_verifier.reset();
                }
                else
                {
                    throw std::runtime_error("Server versions json file corrupted\n");
                }
            }
        }
        else
        {
            throw std::runtime_error("Server versions json file corrupted - Title mot as expected\n");
        }
    }

    const std::string update_handler::get_platform() const
    {
        std::string platform;

#ifdef _WIN64
        platform = "Windows amd64";

#elif _WIN32
        platform = "Windows x86";
#elif __linux__ 
#ifdef __arm__
        platform = "Linux arm";
#else
        platform = "Linux amd64";
#endif
#elif __APPLE__
        platform = "Mac OS";
#elif __ANDROID__
        //TBD
#endif
        return platform;
    }

    bool update_handler::verify_platform(const std::string& pl) const
    {
        std::unordered_set<std::string> pl_set;
        pl_set.emplace("Windows amd64");
        pl_set.emplace("Windows x86");
        pl_set.emplace("Linux amd64");
        pl_set.emplace("Linux arm");
        pl_set.emplace("Mac OS");

        auto val = pl_set.find(pl);
        if (val != pl_set.end())
        {
            return true;
        }
        return false;
    }
    bool update_handler::load_server_versions()
    {
        std::stringstream server_versions_data;
        if (!get_server_data(server_versions_data))
        {
            return false; // Failed to get version from server
        }
        else
        {
            try
            {
                parse_versions_data(server_versions_data);
            }
            catch (std::exception& e)
            {
                // Parsing fail
                std::string error = e.what();
                throw std::runtime_error("json file corrupted:" + error + "\n");
            };
        }

        return true;
    }
}