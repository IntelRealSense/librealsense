// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#if defined __linux__
#include <fcntl.h>
#endif

namespace rsutils {
namespace os {

    bool is_platform_jetson();

} // namespace os
} // namespace rsutils
