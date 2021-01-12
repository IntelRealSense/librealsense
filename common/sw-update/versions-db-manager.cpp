// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <regex>

#include "json.hpp"
#include "versions-db-manager.h"
#include <types.h>

namespace rs2
{

    namespace sw_update
    {
        using json = nlohmann::json;
        using namespace http;

        // Get current platform
        constexpr const char* PLATFORM =

#ifdef _WIN64 
            "Windows amd64";
#elif _WIN32 
            "Windows x86";
#elif __linux__ 
#ifdef __arm__ 
            "Linux arm";
#else 
            "Linux amd64";
#endif
#elif __APPLE__ 
            "Mac OS";
#elif __ANDROID__  
            "Linux arm";
#else 
            "";
#endif

        versions_db_manager::query_status_type versions_db_manager::query_versions(const std::string &device_name, component_part_type component, const update_policy_type policy, version& out_version)
        {
            // Load server versions info on first access
            if (!init())
            {
                out_version = 0;
                return DB_LOAD_FAILURE;
            }

            std::string platform(PLATFORM);

            std::string up_str(to_string(policy));
            std::string comp_str(to_string(component));

            if (up_str.empty() || comp_str.empty()) return NO_VERSION_FOUND;

            // Look for the required version
            auto res = std::find_if(_server_versions_vec.begin(), _server_versions_vec.end(),
                [&, device_name, up_str, comp_str, platform](std::unordered_map<std::string, std::string> ver)
            {
                return (is_device_name_equal(ver["device_name"],device_name,true) && up_str == ver["policy_type"] && comp_str == ver["component"] && (platform == ver["platform"] || ver["platform"] == "*"));
            });

            if (res != _server_versions_vec.end())
            {
                auto version_str = (*res)["version"];
                out_version = version(version_str);
                return VERSION_FOUND;
            }

            out_version = 0;
            return NO_VERSION_FOUND; // Nothing found
        }

        bool versions_db_manager::get_version_data_common(const component_part_type component, const version& version, const std::string& req_field, std::string& out)
        {
            // Check if server versions are loaded
            if (!_server_versions_loaded) return false;

            std::string platform = PLATFORM;

            std::string component_str(to_string(component));

            if (component_str.empty()) return false;

            // Look for the required version
            auto res = std::find_if(_server_versions_vec.begin(), _server_versions_vec.end(),
                [version, component_str, platform](std::unordered_map<std::string, std::string> version_map)
            {
                return (versions_db_manager::version(version_map["version"]) == version && component_str == version_map["component"] && (platform == version_map["platform"] || version_map["platform"] == "*"));
            });

            if (res != _server_versions_vec.end())
            {
                out = (*res)[req_field];
                return true;
            }

            return false; // Nothing found
        }


        std::string versions_db_manager::to_string(const component_part_type& component) const
        {
            switch (component)
            {
            case LIBREALSENSE: return "LIBREALSENSE";
            case VIEWER: return "VIEWER";
            case DEPTH_QUALITY_TOOL: return "DEPTH_QUALITY_TOOL";
            case FIRMWARE: return "FIRMWARE";
                break;
            default:
                LOG_ERROR("Unknown component type: " + component);
                break;
            }
            return "";
        }

        std::string versions_db_manager::to_string(const update_policy_type& policy) const
        {

            switch (policy)
            {
            case EXPERIMENTAL: return "EXPERIMENTAL";
            case RECOMMENDED: return "RECOMMENDED";
            case ESSENTIAL: return "ESSENTIAL";
                break;
            default:
                LOG_ERROR("Unknown policy type: " + policy);
                break;
            }
            return "";
        }

        bool versions_db_manager::from_string(std::string component_str, component_part_type& component_val) const
        {
            static std::unordered_map<std::string, component_part_type> map =
            { {"LIBREALSENSE",LIBREALSENSE},
            {"VIEWER", VIEWER} ,
            {"DEPTH_QUALITY_TOOL", DEPTH_QUALITY_TOOL},
            {"FIRMWARE", FIRMWARE} };

            auto val = map.find(component_str);
            if (val != map.end())
            {
                component_val = val->second;
                return true;
            }

            LOG_ERROR("Unknown component type: " + component_str);
            return false;
        }
        bool versions_db_manager::from_string(std::string policy_str, update_policy_type& policy_val) const
        {
            static std::unordered_map<std::string, update_policy_type> map =
            { {"EXPERIMENTAL",EXPERIMENTAL},
            {"RECOMMENDED", RECOMMENDED} ,
            {"ESSENTIAL", ESSENTIAL} };

            auto val = map.find(policy_str);
            if (val != map.end())
            {
                policy_val = val->second;
                return true;
            }

            LOG_ERROR("Unknown policy type: " + policy_str);
            return false;
        }

        bool versions_db_manager::get_server_data(std::stringstream &ver_data)
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
                else
                {
                    LOG_ERROR("Cannot open file: " + _dev_info_url);
                }
            }
            return server_data_retrieved;
        }

        void versions_db_manager::parse_versions_data(const std::stringstream &ver_data)
        {
            // Parse the json file
            json j(json::parse(ver_data.str()));

            std::unordered_map<std::string, std::function<bool(const std::string&)>> schema;
            build_schema(schema);

            // Validate json file has a versions array 
            if (j.begin().key() == "versions" && j.begin().value().is_array())
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
                                    std::string error_str("Server versions file parsing error - validation fail on key: " + element_key + " value: " + it.value().get<std::string>() + " \n");
                                    LOG_ERROR(error_str);
                                    throw std::runtime_error(error_str);
                                }
                            }
                            else
                            {
                                std::string error_str("Server versions file parsing error - " + element_key + " should be represented as a string");
                                LOG_ERROR(error_str);
                                throw std::runtime_error(error_str);
                            }
                        }
                        else
                        {
                            std::string error_str("Server versions file parsing error - " + element_key + " - unknown field");
                            LOG_ERROR(error_str);
                            throw std::runtime_error(error_str);
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
                        std::string error_str("Server versions json file corrupted - not matching schema requirements");
                        LOG_ERROR(error_str);
                        throw std::runtime_error(error_str);
                    }
                }
            }
            else
            {
                std::string error_str("Server versions json file corrupted - Expect versions field of type array \n)"
                    "Parsed key: " + j.begin().key() + ", value type array: " + (j.begin().value().is_array() ? "TRUE" : "FALSE"));
                LOG_ERROR(error_str);
                throw std::runtime_error(error_str);
            }
        }


        bool versions_db_manager::init()
        {
            // Load server versions info on first access
            if (!_server_versions_loaded)
            {
                std::stringstream server_versions_data;
                // Download / Open the json file
                if (!get_server_data(server_versions_data))
                {
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

        void versions_db_manager::build_schema(std::unordered_map<std::string, std::function<bool(const std::string&)>>& verifier)
        {
            // Builds a map of fields + validation function
            verifier.emplace("device_name", [](const std::string& val) -> bool {  return true;  });
            verifier.emplace("policy_type", [](const std::string& val) -> bool {  return (val == "EXPERIMENTAL") || (val == "RECOMMENDED") || (val == "ESSENTIAL"); });
            verifier.emplace("component", [](const std::string& val) -> bool {  return (val == "LIBREALSENSE") || (val == "VIEWER") || (val == "DEPTH_QUALITY_TOOL") || (val == "FIRMWARE"); });
            verifier.emplace("version", [](const std::string& val) -> bool { return version(val) != version(); });
            verifier.emplace("platform", [&](const std::string& val) -> bool {  return (val == "*") || (val == "Windows amd64") || (val == "Windows x86") || (val == "Linux amd64") || (val == "Linux arm") || (val == "Mac OS"); });
            verifier.emplace("link", [](const std::string& val) -> bool {  return true;  });
            verifier.emplace("release_notes_link", [](const std::string& val) -> bool {  return true;  });
            verifier.emplace("description", [](const std::string& val) -> bool {  return true;  });

        }

        bool versions_db_manager::is_device_name_equal(const std::string &str_from_db, const std::string &str_compared, bool allow_wildcard)
        {
            if (allow_wildcard)
                return (0 == str_from_db.compare(0, str_from_db.find('*'), str_compared, 0, str_from_db.find('*')));
            else
                return (str_from_db == str_compared);
        }
    }


}
