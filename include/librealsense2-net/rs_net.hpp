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

        class net_server 
        {
        public:
            net_server(rs2_device* dev, rs_server_params params)
                : m_params(params)
            {
                rs2_error* e = nullptr;
                m_server = rs2_create_server(RS2_API_VERSION, dev, m_params, &e);
                error::handle(e);
            }

            ~net_server()
            {
                rs2_error* e = nullptr;
                rs2_destroy_server(RS2_API_VERSION, m_server, &e);
                error::handle(e);
            }
            
            int start()
            {
                rs2_error* e = nullptr;
                int result = rs2_start_server(RS2_API_VERSION, m_server, &e);
                error::handle(e);

                return result;
            }

            int stop()
            {
                rs2_error* e = nullptr;
                int result = rs2_stop_server(RS2_API_VERSION, m_server, &e);
                error::handle(e);

                return result;
            }

        private:
            rs_server*       m_server;
            rs_server_params m_params;
        };
}
#endif // LIBREALSENSE_RS2_NET_HPP
