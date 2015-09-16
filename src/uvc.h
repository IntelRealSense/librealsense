#pragma once
#ifndef LIBREALSENSE_UVC_H
#define LIBREALSENSE_UVC_H

#include "types.h"

#include <functional>

namespace rsimpl
{
    namespace uvc
    {
        struct guid { uint32_t data1; uint16_t data2, data3; uint8_t data4[8]; };
        struct context; // Opaque type representing access to the underlying UVC implementation
        struct device; // Opaque type representing access to a specific UVC device

        std::shared_ptr<context> create_context();
        std::vector<std::shared_ptr<device>> query_devices(std::shared_ptr<context> context);

        int get_vendor_id(const device & device);
        int get_product_id(const device & device);

        void claim_interface(device & device, const guid & interface_guid, int interface_number);
        void bulk_transfer(device & device, unsigned char endpoint, void * data, int length, int *actual_length, unsigned int timeout);

        void get_control(const device & device, uint8_t unit, uint8_t ctrl, void * data, int len);
        void set_control(device & device, uint8_t unit, uint8_t ctrl, void * data, int len);

        void set_subdevice_mode(device & device, int subdevice_index, int width, int height, uint32_t fourcc, int fps, std::function<void(const void * frame)> callback);
        void start_streaming(device & device);
        void stop_streaming(device & device);
    }
}

#endif
