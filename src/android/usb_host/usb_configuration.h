/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
//
// Created by daniel on 10/16/2018.
//

#pragma once

#include "usbhost.h"
#include "usb_interface.h"
#include "usb_interface_association.h"
#include <vector>

namespace librealsense
{
    namespace usb_host
    {
        class usb_configuration {
        protected:
            usb_configuration() {}

            std::vector<usb_interface> _interfaces;
            std::vector<usb_interface_association> _interface_assocs;
            const usb_config_descriptor *_desc;

        public:
            usb_configuration(usb_config_descriptor *pDescriptor) {
                _desc = pDescriptor;
            }

            void add_interface(const usb_interface &usbInterface) {
                _interfaces.push_back(usbInterface);
            }

            int get_interfaces_count() { return _interfaces.size(); }

            const usb_interface &get_interface(int interface_index) const {
                return _interfaces.at(interface_index);
            }

            void add_interface_association(const usb_interface_association &ia) {
                _interface_assocs.push_back(ia);
            }

            int get_interfaces_associations_count() { return _interface_assocs.size(); }

            const usb_interface_association &get_interface_association(int ia_index) const {
                return _interface_assocs.at(ia_index);
            }

            const usb_config_descriptor *get_descriptor() const { return _desc; }
        };
    }
}
