// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_NET_HPP
#define LIBREALSENSE_RS2_NET_HPP

#include <librealsense2/rs.hpp>
#include "rs_net.h"

#include <memory>

namespace rs2
{
        class net_device : public rs2::device
        {
        public:
            net_device(const std::string& address) : rs2::device(init(address)) { }

            /**
            * Add network device to existing context.
            * Any future queries on the context will return this device.
            * This operation cannot be undone (except for destroying the context)
            *
            * \param[in] ctx   context to add the device to
            */
            void add_to(context& ctx)
            {
                rs2_error* e = nullptr;
                rs2_context_add_software_device(((std::shared_ptr<rs2_context>)ctx).get(), _dev.get(), &e);
                error::handle(e);
            }


        private:
            std::shared_ptr<rs2_device> init(const std::string& address)
            {
                rs2_error* e = nullptr;
                auto dev = std::shared_ptr<rs2_device>(
                    rs2_create_net_device(RS2_API_VERSION, address.c_str(), &e),
                    rs2_delete_device);
                error::handle(e);

                return dev;
            }
        };
}
#endif // LIBREALSENSE_RS2_NET_HPP
