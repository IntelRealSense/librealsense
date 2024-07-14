// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <rsutils/json.h>
#include <rsutils/json-validator.h>
#include <stdint.h>
#include <sstream>
#include <stdexcept>

namespace librealsense
{
    using rsutils::json;
    using rsutils::json_validator::validate_json_field;

#pragma pack(push, 1)

    class table_header
    {
    public:
        table_header(uint16_t version, uint16_t table_type, uint32_t table_size, uint32_t calib_version, uint32_t crc32) : m_version(version), m_table_type(table_type), m_table_size(table_size), m_calib_version(calib_version), m_crc32(crc32)
        {
        }

        uint32_t get_crc32() const
        {
            return m_crc32;
        }

    private:
        uint16_t m_version;       // major.minor. Big-endian
        uint16_t m_table_type;    // type
        uint32_t m_table_size;    // full size including: header footer
        uint32_t m_calib_version; // major.minor.index
        uint32_t m_crc32;         // crc of all the data in table excluding this header/CRC
    };

#pragma pack(pop)

    // helper function
    template <typename T, size_t N>
    std::vector<T> native_arr_to_std_vector(const T (&arr)[N])
    {
        return std::vector<T>(std::begin(arr), std::end(arr));
    }
}
