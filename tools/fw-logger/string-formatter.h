/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once
#include <string>
#include <map>
#include <stdint.h>
#include <vector>
#include <unordered_map>

namespace fw_logger
{
    class string_formatter
    {
    public:
        string_formatter(std::unordered_map<std::string, std::vector<std::pair<int, std::string>>> enums);
        ~string_formatter(void);

        bool generate_message(const std::string& source, size_t num_of_params, const uint32_t* params, std::string* dest);

    private:
        bool replace_params(const std::string& source, const std::map<std::string, std::string>& exp_replace_map, const std::map<std::string, int>& enum_replace_map, std::string* dest);

        std::unordered_map<std::string, std::vector<std::pair<int, std::string>>> _enums;
    };
}
