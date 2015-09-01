#pragma once
#ifndef LIBREALSENSE_F200_H
#define LIBREALSENSE_F200_H

#include "camera.h"

namespace rsimpl
{
    namespace f200 { class IVCAMHardwareIO; }

    class f200_camera : public rs_camera
    {
        std::unique_ptr<f200::IVCAMHardwareIO> hardware_io;

    public:
        
        f200_camera(uvc::context * ctx, uvc::device device);
        ~f200_camera();

        calibration_info retrieve_calibration() override final;
        void set_stream_intent() override final {}
        void set_option(rs_option option, int value) override final;
        int get_option(rs_option option) override final;
    };
}

#endif
