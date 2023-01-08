//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/os/os.h>
#include <mutex>

#if defined __linux__
#include <unistd.h>
#include <memory>
#include <functional>
#include <fcntl.h>
#endif

namespace rsutils {
    namespace os {
        std::mutex jetson_mtx;

        // Checking if platform is jetson by checking if the file /etc/nv_tegra_release exists
        bool is_platform_jetson()
        {
            std::lock_guard<std::mutex> lock(jetson_mtx);
            static bool is_jetson = false;
            static bool is_first_time = true;
            if (is_first_time)
            {
                is_first_time = false;
    #ifdef __linux__
                // RAII to handle exceptions
                std::unique_ptr<int, std::function<void(int*)> > fd(
                    new int(open("/etc/nv_tegra_release", O_RDONLY)),
                    [](int* d) { if (d && (*d)) { ::close(*d); } delete d; });
                is_jetson = (fd >= 0);
    #else
                is_jetson = false;
    #endif
            }
            return is_jetson;
        }
    }  // namespace os
}  // namespace rsutils
