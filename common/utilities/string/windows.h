// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <sstream>

#ifdef WIN32
#include <Windows.h>

namespace utilities {
namespace string {
namespace windows {

        inline std::string win_to_utf(const WCHAR * s)
        {
            auto len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
            std::ostringstream ss;

            if (len == 0)
            {
                ss << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError();
                throw std::runtime_error(ss.str());
            }

            std::string buffer(len - 1, ' ');
            len = WideCharToMultiByte(CP_UTF8, 0, s, -1, &buffer[0], static_cast<int>(buffer.size()) + 1, nullptr, nullptr);
            if (len == 0)
            {
                ss.clear();
                ss << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError();
                throw std::runtime_error(ss.str());
            }

            return buffer;
        }
}  // namespace windows
}  // namespace string
}  // namespace utilities
#endif