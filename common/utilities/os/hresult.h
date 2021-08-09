// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <Windows.h>
#include <functional>
#include <comdef.h>
#include <sstream>

namespace utilities {
namespace hresult {

      inline std::string win_to_utf(const WCHAR * s)
      {
          auto len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
          std::stringstream ss;

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

      inline std::string hr_to_string(HRESULT hr)
      {
          _com_error err(hr);
          std::wstring errorMessage = (err.ErrorMessage()) ? err.ErrorMessage() : L"";
          std::ostringstream ss;
          ss << "HResult 0x" << std::hex << hr << ": \"" << win_to_utf(errorMessage.data()) << "\"";
          return ss.str();
      }

      inline bool check(const char * call, HRESULT hr, std::function<void(std::string descr, bool to_throw)> do_on_failure, bool to_throw = true)
      {
          if (FAILED(hr))
          {
              std::stringstream ss;
              ss << call << " returned: " << hr_to_string(hr);
              std::string descr = ss.str();
              do_on_failure(descr, to_throw);

              return false;
          }
          return true;
      }

#define CHECK_HR_THROWS(x) check(#x, x, [&](std::string descr, bool to_throw)\
{\
        throw std::runtime_error(descr);\
})

}
}
