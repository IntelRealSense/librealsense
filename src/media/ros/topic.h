// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include<stdarg.h>

namespace librealsense
{
    namespace file_format
    {
        class topic
        {
        private:
            std::string m_value;
            const std::string elements_separator = "/";

        public:
            topic(const std::string& value) : m_value(value)
            {

            }

            std::string to_string() const
            {
                return m_value;
            }

            std::string at(uint32_t index) const
            {
                size_t current_pos = 0;
                std::string token;
                std::string value_copy = m_value;
                uint32_t elements_iterator = 0;
                while ((current_pos = value_copy.find(elements_separator)) != std::string::npos) {
                    token = value_copy.substr(0, current_pos);
                    if(elements_iterator == index) return token;
                    value_copy.erase(0, current_pos + elements_separator.length());
                     ++elements_iterator;
                }

                if( elements_iterator == index)
                    return value_copy;

                return std::string();
            }
        };
    }
}
