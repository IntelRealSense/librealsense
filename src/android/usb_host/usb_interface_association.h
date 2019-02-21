/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
//
// Created by daniel on 10/22/2018.
//

#pragma once

namespace librealsense
{
    namespace usb_host
    {
        class usb_interface_association
        {
            int _mi;

        private:
            usb_interface_assoc_descriptor *_desc;
            std::vector< unsigned char > _interfaces;

        public:
            usb_interface_association(int interface_index,usb_interface_assoc_descriptor *pDescriptor) {
                _desc = pDescriptor;
                _mi=interface_index;
            }

            const usb_interface_assoc_descriptor * get_descriptor() const { return _desc; }

            int get_mi() const { return _mi; }
        };
    }
}


