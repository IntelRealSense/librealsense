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

        // Enumerate devices
        std::shared_ptr<context> create_context();
        std::vector<std::shared_ptr<device>> query_devices(std::shared_ptr<context> context);

        // Static device properties
        int get_vendor_id(const device & device);
        int get_product_id(const device & device);

        // Direct USB controls
        void claim_interface(device & device, const guid & interface_guid, int interface_number);
        void bulk_transfer(device & device, unsigned char endpoint, void * data, int length, int *actual_length, unsigned int timeout);

        // Access CT and PU controls
        inline bool is_pu_control(rs_option option) { return option >= RS_OPTION_COLOR_BACKLIGHT_COMPENSATION && option <= RS_OPTION_COLOR_WHITE_BALANCE; }
        void get_pu_control_range(const device & device, int subdevice, rs_option option, int * min, int * max);
        void set_pu_control(device & device, int subdevice, rs_option option, int value);
        int get_pu_control(const device & device, int subdevice, rs_option option);

        // Access XU controls
        void init_controls(device & device, int subdevice, const guid & xu_guid);
        void set_control(device & device, int subdevice, uint8_t ctrl, void * data, int len);
        void get_control(const device & device, int subdevice, uint8_t ctrl, void * data, int len);

        // Control streaming
        void set_subdevice_mode(device & device, int subdevice_index, int width, int height, uint32_t fourcc, int fps, std::function<void(const void * frame)> callback);
        void start_streaming(device & device, int num_transfer_bufs);
        void stop_streaming(device & device);
    }
}

#endif
