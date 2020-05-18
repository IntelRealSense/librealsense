// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#include <fstream>
#include <unordered_set>
#include <algorithm>
#include <regex>

#include "json.hpp"
#include "update_handler.h"
#include "http_downloader.h"
#include "types.h"

namespace rs2
{
    using json = nlohmann::json;

    bool update_handler::query_versions(const std::string &dev_name, component_part_type cp, const update_policy_type up, long long &out_version)
    {
        // Load server versions info on first access
        if (!init())
        {
            out_version = 0;
            return false;
        }

        auto platform = get_platform();

        std::string up_str(convert_update_policy(up));
        std::string comp_str(convert_component_name(cp));

        if (up_str.empty() || comp_str.empty()) return false;

        // Look for the required version
        auto res = std::find_if(_server_versions_vec.begin(), _server_versions_vec.end(),
            [&, dev_name, up_str, comp_str, platform](std::unordered_map<std::string, std::string> ver)
        {
            return (dev_name == ver["device_name"] && up_str == ver["policy_type"] && comp_str == ver["component"] && platform == ver["platform"]);
        });

        if (res != _server_versions_vec.end())
        {
            auto version_str = (*res)["version"];
            out_version = std::stoll(version_str.c_str()); // No need for validation.
            return true;
        }

        out_version = 0;
        return false; // Nothing found
    }

    bool update_handler::get_ver_data(const component_part_type cp, const long long version, const std::string& req_field, std::string& out)
    {
        // Load server versions info on first access
        if (!init()) return false;

        std::string platform = get_platform();

        std::string comp_str(convert_component_name(cp));

        if (comp_str.empty()) return false;

        // Look for the required version
        auto res = std::find_if(_server_versions_vec.begin(), _server_versions_vec.end(),
            [this, version, comp_str, platform](std::unordered_map<std::string, std::string> ver)
        {
            long long version_num = std::stoll(ver["version"]);
            return (version_num == version && comp_str == ver["component"] && platform == ver["platform"]);
        });

        if (res != _server_versions_vec.end())
        {
            out = (*res)[req_field];
            return true;
        }

        return false; // Nothing found
    }


    std::string update_handler::convert_component_name(const component_part_type& comp) const
    {
        switch (comp)
        {
        case LIBREALSENSE: return "LIBREALSENSE";
        case VIEWER: return "VIEWER";
        case DEPTH_QUALITY_TOOL: return "DEPTH_QUALITY_TOOL";
        case FIRMWARE: return "FIRMWARE";
            break;
        default:
            break;
        }
        return "";
    }

    std::string update_handler::convert_update_policy(const update_policy_type& pol) const
    {

        switch (pol)
        {
        case EXPERIMENTAL: return "EXPERIMENTAL";
        case RECOMMENDED: return "RECOMMENDED";
        case REQUIRED: return "REQUIRED";
            break;
        default:
            break;
        }
        return "";
    }

    bool update_handler::get_server_data(std::stringstream &ver_data)
    {
        bool server_data_retrieved(false);

        if (false == _local_source_file)
        {
            // download file from URL
            http_downloader hd;
            if (hd.download_to_stream(_dev_info_url, ver_data, _download_cb_func))
            {
                server_data_retrieved = true;
            }
        }
        else
        {
            // read from local file
            std::ifstream file(_dev_info_url);
            if (file.good())
            {
                server_data_retrieved = true;
                ver_data << file.rdbuf();
            }
        }
        return server_data_retrieved;
    }

    void update_handler::parse_versions_data(const std::stringstream &ver_data)
    {
        // Parse the json file
        json j(json::parse(ver_data.str()));

        std::unordered_map<std::string, std::function<bool(const std::string&)>> schema;
        build_schema(schema);

        // Validate json file title
        if (j.find("versions") != j.end())
        {
            // Iterate through the versions
            for (auto &ver : j["versions"])
            {   // Iterate through the version fields
                std::unordered_map<std::string, std::string> curr_version;
                for (auto it = ver.begin(); it != ver.end(); ++it)
                {
                    std::string element_key(it.key());

                    auto schema_field(schema.find(element_key));
                    if (schema_field != schema.end()) {
                        if (it.value().is_string())
                        {
                            if (schema_field->second(it.value().get<std::string>()))
                            {
                                // Value validation passed - add to current version
                                curr_version[element_key] = it.value().get<std::string>();
                            }
                            else
                            {
                                std::string error_str("Server versions file parsing error - " + element_key + " Validation fail on value : " + it.value().get<std::string>() + " \n");
                                if (element_key == "version") // Special case for version validation
                                {
                                    error_str += "Version should be represented as AABBCCDDDD format only";
                                }
                                LOG_WARNING(error_str);
                                throw std::runtime_error(error_str);
                            }
                        }
                        else
                        {
                            LOG_WARNING("Server versions file parsing error - " + element_key + " all values should be represented as a string \n");
                            throw std::runtime_error("Server versions file parsing error - " + element_key + " - unknown field \n");
                        }
                    }
                    else
                    {
                        LOG_WARNING("Server versions file parsing error - " + element_key + " - unknown field \n");
                        throw std::runtime_error("Server versions file parsing error - " + element_key + "- unknown field \n");
                    }
                }

                // Verify each key in the schema can be found in the current version fields
                //std::pair<const K, E> &item
                if (std::all_of(schema.cbegin(), schema.cend(), [curr_version](const std::pair<std::string, std::function<bool(const std::string&)>> &schema_item)
                {
                    return curr_version.end() != std::find_if(curr_version.cbegin(), curr_version.cend(), [schema_item](const std::pair<std::string, std::string> &ver_item) {return schema_item.first == ver_item.first; });
                }))
                {
                    _server_versions_vec.emplace_back(curr_version); // Version added to valid versions vector
                }
                else
                {
                    LOG_WARNING("Server versions json file corrupted - not matching schema requirements \n");
                    throw std::runtime_error("Server versions json file corrupted - not matching schema requirements \n");
                }
            }
        }
        else
        {
            LOG_WARNING("Server versions json file corrupted - Title not as expected\n");
            throw std::runtime_error("Server versions json file corrupted - Title not as expected\n");
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
        platform = "Linux arm";
#endif
        return platform;
    }

    bool update_handler::init()
    {
        // Load server versions info on first access
        if (!_server_versions_loaded)
        {
            std::stringstream server_versions_data;
            server_versions_data.clear();
            // Download / Open the json file
            if (!get_server_data(server_versions_data))
            {
                LOG_WARNING("Can not download version file " + _dev_info_url + " \n");
                return false; // Failed to get version from server/file
            }
            else
            {
                // Parse and validate the json file - Throws exception on error
                parse_versions_data(server_versions_data);
                _server_versions_loaded = true;
            }
        }
        return true;
    }

    void update_handler::build_schema(std::unordered_map<std::string, std::function<bool(const std::string&)>>& verifier)
    {
        // Builds a map of fields + validation function
        verifier.emplace("device_name", [](const std::string& val) -> bool {  return true;  });
        verifier.emplace("policy_type", [](const std::string& val) -> bool {  return (val == "EXPERIMENTAL") || (val == "RECOMMENDED") || (val == "REQUIRED"); });
        verifier.emplace("component", [](const std::string& val) -> bool {  return (val == "LIBREALSENSE") || (val == "VIEWER") || (val == "DEPTH_QUALITY_TOOL") || (val == "FIRMWARE"); });
        verifier.emplace("version", [](const std::string& val) -> bool {  return (std::regex_match(val, std::regex("[0-9]{10}")));  });
        verifier.emplace("platform", [&](const std::string& val) -> bool {  return (val == "Windows amd64") || (val == "Windows x86") || (val == "Linux amd64") || (val == "Linux arm") || (val == "Mac OS"); });
        verifier.emplace("link", [](const std::string& val) -> bool {  return true;  });
        verifier.emplace("release_notes_link", [](const std::string& val) -> bool {  return true;  });
        verifier.emplace("description", [](const std::string& val) -> bool {  return true;  });

    }


}
