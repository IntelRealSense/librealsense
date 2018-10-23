// License: Apache 2.0. See LICENSE file in root directory.

#pragma once

#include <android/native_window.h>
#include <string>
#include <sstream>
#include <algorithm>

#include "librealsense2/rs.hpp"

void DrawRSFrame(ANativeWindow *window, rs2::frame &frame);

// trim from end (in place)
inline void rtrim(std::string &s, char c) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [c](int ch) {
        return (ch != c);
    }).base(), s.end());
}

inline std::string api_version_to_string(int version) {
    if (version / 10000 == 0) {
        std::stringstream ss;
        ss << version;
        return ss.str();
    }
    std::stringstream ss;
    ss << (version / 10000) << "." << (version % 10000) / 100 << "." << (version % 100);
    return ss.str();
}
