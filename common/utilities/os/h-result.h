// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <Windows.h>

namespace utilities {
namespace h_result {

      std::string win_to_utf(const WCHAR * s);
      std::string hr_to_string(HRESULT hr);
      bool check(const char * call, HRESULT hr, bool to_throw = true);

#define CHECK_HR(x) check(#x, x);
#define LOG_HR(x) check(#x, x, false);
}
}
