// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <Windows.h>
#include <functional>
#include <comdef.h>
#include <sstream>
#include "../string/string-utilities.h"

namespace utilities {
namespace hresult {

    inline std::string hr_to_string(HRESULT hr)
    {
        _com_error err(hr);
        std::wstring errorMessage = (err.ErrorMessage()) ? err.ErrorMessage() : L"";
        std::ostringstream ss;
        ss << "HResult 0x" << std::hex << hr << ": \"" << string::win_to_utf(errorMessage.data()) << "\"";
        return ss.str();
    }


#define CHECK_HR_STR_THROW( call, hr )                                                             \
    if( FAILED( hr ) )                                                                             \
    {                                                                                              \
        std::stringstream ss;                                                                      \
        ss << call << " returned: " << utilities::hresult::hr_to_string( hr );                     \
        std::string descr = ss.str();                                                              \
        throw std::runtime_error( descr );                                                         \
    }

#define CHECK_HR_STR_LOG( call, hr )                                                               \
    if( FAILED( hr ) )                                                                             \
    {                                                                                              \
        std::stringstream ss;                                                                      \
        ss << call << " returned: " << utilities::hresult::hr_to_string( hr );                     \
        std::string descr = ss.str();                                                              \
        CLOG( DEBUG, "librealsense" ) << descr;                                                    \
    }

#define CHECK_HR( x ) CHECK_HR_STR_THROW( #x, x )

#if BUILD_EASYLOGGINGPP
    #define LOG_HR( x ) CHECK_HR_STR_LOG( #x, x )
#endif
}
}
