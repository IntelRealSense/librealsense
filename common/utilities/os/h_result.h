// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once
#include "../../src/types.h"
#include <comdef.h>



namespace utilities {
namespace h_result {

    using namespace librealsense;
    inline std::string win_to_utf(const WCHAR * s)
        {
            auto len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
            if (len == 0)
                throw std::runtime_error(to_string() << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError());

            std::string buffer(len - 1, ' ');
            len = WideCharToMultiByte(CP_UTF8, 0, s, -1, &buffer[0], static_cast<int>(buffer.size()) + 1, nullptr, nullptr);
            if (len == 0)
                throw std::runtime_error(to_string() << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError());

            return buffer;
        }

    inline std::string hr_to_string(HRESULT hr)
    {
        _com_error err(hr);
        std::wstring errorMessage = (err.ErrorMessage()) ? err.ErrorMessage() : L"";
        std::stringstream ss;
        ss << "HResult 0x" << std::hex << hr << ": \"" << win_to_utf(errorMessage.data()) << "\"";
        return ss.str();
    }

    inline bool check(const char * call, HRESULT hr, bool to_throw = true)
    {
        if (FAILED(hr))
        {
            std::string descr = to_string() << call << " returned: " << hr_to_string(hr);
            if (to_throw)
                throw librealsense::windows_backend_exception(descr);
            else
                LOG_DEBUG(descr);

            return false;
        }
        return true;
    }

#define CHECK_HR(x) check(#x, x);
#define LOG_HR(x) check(#x, x, false);

}
}
