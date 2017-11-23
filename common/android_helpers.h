// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <inttypes.h>
#include <stdlib.h>
#include <sstream>

namespace std
{
    template <typename T>
    inline std::string to_string(T value)
    {
        std::ostringstream os ;
        os << value ;
        return os.str() ;
    }

    inline long long stoll(const std::string& value)
    {
    char* endptr;
        auto x = strtoimax(value.c_str(), &endptr, 10);

        return static_cast<long long>(x);
    }

    inline unsigned long stoul(const std::string& value)
    {
        char* endptr;
        auto x = strtoimax(value.c_str(), &endptr, 10);

        return static_cast<unsigned long>(x);
    }

    inline int stoi(const std::string& value)
    {
        char* endptr;
        auto x = strtoimax(value.c_str(), &endptr, 10);

        return static_cast<int>(x);
    }

    inline float stof(const std::string& value)
    {
        char* pEnd;
        return strtof(value.c_str(), &pEnd);
    }

    inline double stod(const std::string& value)
    {
        char* pEnd;
        return strtod(value.c_str(), &pEnd);
    }
}
