#include "F200.h"
#include "HardwareIO.h"
#include "../../include/librealsense/rsutil.h"

#ifndef WIN32

using namespace rs;

namespace f200
{
    
    F200Camera::F200Camera(uvc_context_t * ctx, uvc_device_t * device) : UVCCamera(ctx, device)
    {
        subdevices.resize(2);
    }

    F200Camera::~F200Camera()
    {
        
    }

    static SubdeviceMode MakeDepthMode(const CameraCalibrationParameters & c, int w, int h)
    {
        rs_intrinsics intrin = {{w,h}};
        intrin.focal_length[0] = c.Kc[0][0]*0.5f * w;
        intrin.focal_length[1] = c.Kc[1][1]*0.5f * h;
        intrin.principal_point[0] = (c.Kc[0][2]*0.5f + 0.5f) * w;
        intrin.principal_point[1] = (c.Kc[1][2]*0.5f + 0.5f) * h;
        for(int i=0; i<5; ++i) intrin.distortion_coeff[i] = c.Invdistc[i];
        intrin.distortion_model = RS_INVERSE_BROWN_CONRADY_DISTORTION;
        return {1, w, h, UVC_FRAME_FORMAT_INVR, 60, {{RS_DEPTH, intrin, RS_Z16, 60}}, &rs::unpack_strided_image};
    }

    static SubdeviceMode MakeColorMode(const CameraCalibrationParameters & c, int w, int h)
    {
        rs_intrinsics intrin = {{w,h}};
        intrin.focal_length[0] = c.Kt[0][0]*0.5f;
        intrin.focal_length[1] = c.Kt[1][1]*0.5f;
        intrin.principal_point[0] = c.Kt[0][2]*0.5f + 0.5f;
        intrin.principal_point[1] = c.Kt[1][2]*0.5f + 0.5f;
        if(w*3 == h*4) // If using a 4:3 aspect ratio, adjust intrinsics (defaults to 16:9)
        {
            intrin.focal_length[0] *= 4.0f/3;
            intrin.principal_point[0] *= 4.0f/3;
            intrin.principal_point[0] -= 1.0f/6;
        }
        intrin.focal_length[0] *= w;
        intrin.focal_length[1] *= h;
        intrin.principal_point[0] *= w;
        intrin.principal_point[1] *= h;
        intrin.distortion_model = RS_NO_DISTORTION;
        return {0, w, h, UVC_FRAME_FORMAT_YUYV, 60, {{RS_COLOR, intrin, RS_RGB8, 60}}, &rs::unpack_yuyv_to_rgb};
    }

    CalibrationInfo F200Camera::RetrieveCalibration()
    {
        if(!hardware_io) hardware_io.reset(new IVCAMHardwareIO(context));
        const CameraCalibrationParameters & calib = hardware_io->GetParameters();

        rs::CalibrationInfo c;
        c.modes.push_back(MakeDepthMode(calib, 640, 480));
        c.modes.push_back(MakeColorMode(calib, 1920, 1080));
        c.modes.push_back(MakeColorMode(calib, 640, 480));
        c.stream_poses[RS_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        c.stream_poses[RS_COLOR] = {transpose((const float3x3 &)calib.Rt), (const float3 &)calib.Tt * 0.001f}; // convert mm to m
        c.depth_scale = (calib.Rmax / 0xFFFF) * 0.001f; // convert mm to m
        return c;
    }

} // end f200

#endif
