// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <rsutils/json.h>
#include <stdint.h>
#include <sstream>
#include <stdexcept>

namespace rsutils
{
    namespace json_validator
    {
        template <typename ExpectedType>
        void validate_json_field(const json &j, const std::string &field_name, size_t size = 0)
        {
            if (!j.contains(field_name))
            {
                std::ostringstream oss;
                oss << "Missing field '" << field_name << "' in JSON.";
                throw std::runtime_error(oss.str());
            }
            if (
                std::is_same<ExpectedType, uint8_t>::value ||
                std::is_same<ExpectedType, uint16_t>::value ||
                std::is_same<ExpectedType, uint32_t>::value ||
                std::is_same<ExpectedType, uint64_t>::value )
            {
                if (!j.at(field_name).is_number_unsigned())
                {
                    std::ostringstream oss;
                    oss << "Invalid type for field '" << field_name << "'";
                    throw std::runtime_error(oss.str());
                }
            }
            else if (
                std::is_same<ExpectedType, int>::value ||
                std::is_same<ExpectedType, int8_t>::value ||
                std::is_same<ExpectedType, int16_t>::value ||
                std::is_same<ExpectedType, int32_t>::value ||
                std::is_same<ExpectedType, int64_t>::value )
            {
                if (!j.at(field_name).is_number_integer())
                {
                    std::ostringstream oss;
                    oss << "Invalid type for field '" << field_name << "'";
                    throw std::runtime_error(oss.str());
                }
            }
            else if (std::is_same<ExpectedType, float>::value )
            {
                if (!j.at(field_name).is_number_float())
                {
                    std::ostringstream oss;
                    oss << "Invalid type for field '" << field_name << "'";
                    throw std::runtime_error(oss.str());
                }
            }
            else if (
                std::is_same<ExpectedType, std::vector<uint8_t>>::value ||
                std::is_same<ExpectedType, std::vector<uint16_t>>::value ||
                std::is_same<ExpectedType, std::vector<uint32_t>>::value ||
                std::is_same<ExpectedType, std::vector<uint64_t>>::value )
            {
                if (!j.at(field_name).is_array())
                {
                    std::ostringstream oss;
                    oss << "Invalid type for field '" << field_name << "'";
                    throw std::runtime_error(oss.str());
                }
                if (j.at(field_name).size() != size)
                {
                    std::ostringstream oss;
                    oss << "Invalid size for array field '" << field_name << "'";
                    throw std::runtime_error(oss.str());
                }
                for (const auto &item : j.at(field_name))
                {
                    if (!item.is_number_unsigned())
                    {
                        std::ostringstream oss;
                        oss << "Invalid element type in array field '" << field_name << "'";
                        throw std::runtime_error(oss.str());
                    }
                }
            }
            else if (
                std::is_same<ExpectedType, std::vector<int>>::value ||
                std::is_same<ExpectedType, std::vector<int8_t>>::value ||
                std::is_same<ExpectedType, std::vector<int16_t>>::value ||
                std::is_same<ExpectedType, std::vector<int32_t>>::value ||
                std::is_same<ExpectedType, std::vector<int64_t>>::value )
            {
                if (!j.at(field_name).is_array())
                {
                    std::ostringstream oss;
                    oss << "Invalid type for field '" << field_name << "'";
                    throw std::runtime_error(oss.str());
                }
                if (j.at(field_name).size() != size)
                {
                    std::ostringstream oss;
                    oss << "Invalid size for array field '" << field_name << "'";
                    throw std::runtime_error(oss.str());
                }
                for (const auto &item : j.at(field_name))
                {
                    if (!item.is_number_integer())
                    {
                        std::ostringstream oss;
                        oss << "Invalid element type in array field '" << field_name << "'";
                        throw std::runtime_error(oss.str());
                    }
                }
            }
            else
            {
                std::ostringstream oss;
                oss << "Unsupported type for field '" << field_name << "'";
                throw std::runtime_error(oss.str());
            }
        }
    }
}
