//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/os/os.h>

#if defined __linux__
#include <unistd.h>
#include <memory>
#include <functional>
#endif

namespace rsutils {
namespace os {
    bool is_platform_jetson()
    {
#ifdef __linux__
        // RAII to handle exceptions
        std::unique_ptr<int, std::function<void(int*)> > fd(
            new int(open("/etc/nv_tegra_release", O_RDONLY)),
            [](int* d) { if (d && (*d)) { ::close(*d); } delete d; });
        return (fd >= 0);
#else
        return false;
#endif
    }
}  // namespace os
}  // namespace rsutils
