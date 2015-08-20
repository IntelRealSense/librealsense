#include "../Common.h"
#include "F200.h"
#include "Projection.h"
#include "../../include/librealsense/rsutil.h"

#ifndef WIN32

using namespace rs;

namespace f200
{
    
    F200Camera::F200Camera(uvc_context_t * ctx, uvc_device_t * device) : UVCCamera(ctx, device)
    {
        
    }

    F200Camera::~F200Camera()
    {
        
    }

    int F200Camera::GetStreamSubdeviceNumber(int stream) const
    {
        switch(stream)
        {
        case RS_COLOR: return 0;
        case RS_DEPTH: return 1;
        default: throw std::runtime_error("invalid stream");
        }
    }

    static ResolutionMode MakeDepthMode(const CameraCalibrationParameters & c, int w, int h)
    {
        rs_intrinsics intrin = {{w,h}};
        intrin.focal_length[0] = c.Kc[0][0]*0.5f * w;
        intrin.focal_length[1] = c.Kc[1][1]*0.5f * h;
        intrin.principal_point[0] = (c.Kc[0][2]*0.5f + 0.5f) * w;
        intrin.principal_point[1] = (c.Kc[1][2]*0.5f + 0.5f) * h;
        for(int i=0; i<5; ++i) intrin.distortion_coeff[i] = c.Invdistc[i];
        intrin.distortion_model = RS_INVERSE_BROWN_CONRADY_DISTORTION;
        return {RS_DEPTH, w,h,60,RS_Z16, w,h,60,UVC_FRAME_FORMAT_INVR, intrin};
    }

    static ResolutionMode MakeColorMode(const CameraCalibrationParameters & c, int w, int h)
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
        return {RS_COLOR, w,h,60,RS_RGB, w,h,60,UVC_FRAME_FORMAT_YUYV, intrin};
    }

    void F200Camera::RetrieveCalibration()
    {
        if(!hardware_io)
        {
            hardware_io.reset(new IVCAMHardwareIO(internalContext));
            const CameraCalibrationParameters & calib = hardware_io->GetParameters();

            modes.push_back(MakeDepthMode(calib, 640, 480));
            modes.push_back(MakeColorMode(calib, 1920, 1080));
            modes.push_back(MakeColorMode(calib, 640, 480));
        }
    }

    rs_extrinsics F200Camera::GetStreamExtrinsics(int from, int to)
    {
        if(from == RS_DEPTH && to == RS_COLOR)
        {
            const CameraCalibrationParameters & calib = hardware_io->GetParameters();
            return {{calib.Rt[0][0], calib.Rt[0][1], calib.Rt[0][2],
                     calib.Rt[1][0], calib.Rt[1][1], calib.Rt[1][2],
                     calib.Rt[2][0], calib.Rt[2][1], calib.Rt[2][2]},
                    {calib.Tt[0], calib.Tt[1], calib.Tt[2]}};
        }
        else throw std::runtime_error("unsupported streams");
    }

    float F200Camera::GetDepthScale() const
    {
        auto inst = Projection::GetInstance();
        if (!inst->m_calibration) throw std::runtime_error("calibration not initialized");
        const CameraCalibrationParameters & p = inst->m_calibration.params;
        return p.Rmax / 0xFFFF;
    }

} // end f200

#endif
