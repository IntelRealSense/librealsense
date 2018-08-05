// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <inttypes.h>
#include <stdlib.h>
#include <sstream>
#include <ios>
#include <android/ndk-version.h>

#if __NDK_MAJOR__ <= 16
namespace std
{
    template <typename T>
    inline std::string to_string(T value)
    {
        std::ostringstream os;
        os << value;
        return os.str();
    }

    template <typename T>
    inline T stoT(const std::string& value)
    {
        char* endptr;
        auto x = strtoimax(value.c_str(), &endptr, 10);

        return static_cast<T>(x);
    }

    inline long long stoll(const std::string& value)
    {
        return stoT<long long>(value);
    }

    inline unsigned long stoul(const std::string& value)
    {
        return stoT<unsigned long>(value);
    }

    inline int stoi(const std::string& value)
    {
        return stoT<int>(value);
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

    inline std::ios_base& hexfloat(std::ios_base& str)
    {
        str.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield);
        return str;
    }
}
#endif // ANDROID_NDK_MAJOR

