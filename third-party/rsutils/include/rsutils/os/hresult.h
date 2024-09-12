// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <Windows.h>
#include <functional>
#include <comdef.h>
#include <sstream>
#include <rsutils/string/windows.h>

namespace rsutils {
namespace hresult {

    inline std::string hr_to_string(HRESULT hr)
    {
        _com_error err(hr);
        const TCHAR * error_text = err.ErrorMessage();
        if( ! error_text )
            error_text = TEXT( "" );
        std::ostringstream ss;
        ss << "HResult 0x" << std::hex << hr << ": \"" << string::windows::win_to_utf( error_text ) << "\"";
        return ss.str();
    }


#define CHECK_HR_STR( call, hr )                                                                   \
    if( FAILED( hr ) )                                                                             \
    {                                                                                              \
        std::ostringstream ss;                                                                     \
        ss << call << " returned: " << rsutils::hresult::hr_to_string( hr );                       \
        std::string descr = ss.str();                                                              \
        LOG_DEBUG(descr);                                                                          \
        throw std::runtime_error( descr );                                                         \
    }


#define LOG_HR_STR( call, hr )                                                                     \
    if( FAILED( hr ) )                                                                             \
    {                                                                                              \
        std::ostringstream ss;                                                                     \
        ss << call << " returned: " << rsutils::hresult::hr_to_string( hr );                       \
        std::string descr = ss.str();                                                              \
        LOG_DEBUG(descr);                                                                          \
    }

#define CHECK_HR( x ) CHECK_HR_STR( #x, x )
#define LOG_HR( x ) LOG_HR_STR( #x, x )

}
}
