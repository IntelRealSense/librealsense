/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_RS2_LEGACY_ADAPTOR_HPP
#define LIBREALSENSE_RS2_LEGACY_ADAPTOR_HPP

#include "librealsense2/rs.hpp"
#include "legacy_adaptor.h"

namespace rs2 {
    namespace legacy {
        class legacy_device : public rs2::software_device {
            std::shared_ptr<rs2_device> create_device_ptr(int idx)
            {
                rs2_error* e = nullptr;
                std::shared_ptr<rs2_device> dev(
                    rs2_create_legacy_adaptor_device(idx, &e),
                    rs2_delete_device);
                rs2::error::handle(e);
                return dev;
            }
        public:
            /**
             * Create a librealsense2 software device that wraps a legacy librealsense1 device
             * \param[in]  idx    which legacy device to create, 0-indexed
             */
            legacy_device(int idx)
                : rs2::software_device(create_device_ptr(idx))
            {}
        };
    }
}

#endif