#pragma once
#ifndef LIBREALSENSE_F200_H
#define LIBREALSENSE_F200_H

#include "device.h"

namespace rsimpl
{
    namespace f200 { class IVCAMHardwareIO; }

    class f200_camera : public rs_device
    {
        std::unique_ptr<f200::IVCAMHardwareIO> hardware_io;
    public:      
        f200_camera(uvc::device device, bool sr300);
        ~f200_camera();

        void set_option(rs_option option, int value) override final;
        int get_option(rs_option option) const override final;
        int convert_timestamp(int64_t timestamp) const override final { return static_cast<int>(timestamp / 100000); }
    };
}

#endif
