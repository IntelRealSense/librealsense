/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
//
// Created by daniel on 10/16/2018.
//

#pragma once


#include <vector>
#include "usb_endpoint.h"

namespace librealsense
{
    namespace usb_host
    {
        class usb_interface
        {
            usb_interface_descriptor _desc;
            std::vector<usb_endpoint> _endpoints;
        public:
            usb_interface(usb_interface_descriptor descriptor) {
                _desc = descriptor;
            }

            virtual ~usb_interface() {}

            void add_endpoint(const usb_endpoint &usb_endpoint) {
                _endpoints.push_back(usb_endpoint);
            }

            int get_Endpoint_count() const { return _endpoints.size(); }

            const usb_endpoint &get_endpoint(int endpoint_index) const {
                return _endpoints.at(endpoint_index);
            }

            const usb_interface_descriptor &get_descriptor() const { return _desc; }
        };
    }
}



