#include "R200.h"

#ifdef USE_UVC_DEVICES
#include "HardwareIO.h"
#include "../../include/librealsense/rsutil.h"
#include <cassert>

using namespace rs;

namespace r200
{
    R200Camera::R200Camera(uvc_context_t * ctx, uvc_device_t * device) : UVCCamera(ctx, device)
    {

    }

    static ResolutionMode MakeDepthMode(int w, int h, const RectifiedIntrinsics & i)
    {
        assert(i.rw == w+12 && i.rh == h+12);
        return {RS_DEPTH, w,h,0,RS_Z16, 628,h+1,0,UVC_FRAME_FORMAT_Z16, {{w,h}, {i.rfx,i.rfy}, {i.rpx-6,i.rpy-6}, {0,0,0,0,0}, RS_NO_DISTORTION}};
    }

    static ResolutionMode MakeColorMode(const UnrectifiedIntrinsics & i)
    {
        return {RS_COLOR, i.w,i.h,60,RS_RGB, i.w,i.h,59,UVC_FRAME_FORMAT_YUYV, {{i.w,i.h}, {i.fx,i.fy}, {i.px,i.py}, {i.k[0],i.k[1],i.k[2],i.k[3],i.k[4]}, RS_GORDON_BROWN_CONRADY_DISTORTION}};
    }

    int R200Camera::GetStreamSubdeviceNumber(int stream) const
    {
        switch(stream)
        {
        case RS_DEPTH: return 1;
        case RS_COLOR: return 2;
        default: throw std::runtime_error("invalid stream");
        }
    }

    void R200Camera::EnableStreamPreset(int streamIdentifier, int preset)
    {
        switch(streamIdentifier)
        {
        case RS_DEPTH: EnableStream(RS_DEPTH, 480, 360, 0, RS_Z16); break;
        case RS_COLOR: EnableStream(RS_COLOR, 640, 480, 60, RS_RGB); break;
        default: throw std::runtime_error("unsupported stream");
        }
    }

    void R200Camera::RetrieveCalibration()
    {
        if(modes.empty())
        {
            uvc_device_handle_t * handle = GetHandleToAnyStream();
            if(!handle) throw std::runtime_error("RetrieveCalibration() failed as no stream interfaces were open");
            CameraCalibrationParameters calib;
            CameraHeaderInfo header;
            read_camera_info(handle, calib, header);

            modes.push_back(MakeDepthMode(628, 468, calib.modesLR[0]));
            modes.push_back(MakeDepthMode(480, 360, calib.modesLR[1]));
            //modes.push_back(MakeDepthMode(320, 240, calib.modesLR[2])); // NOTE: QRES oddness
            modes.push_back(MakeColorMode(calib.intrinsicsThird[0]));
            modes.push_back(MakeColorMode(calib.intrinsicsThird[1]));

            stream_poses[RS_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
            stream_poses[RS_COLOR].orientation = transpose((const float3x3 &)calib.Rthird[0]);
            stream_poses[RS_COLOR].position = stream_poses[RS_COLOR].orientation * (const float3 &)calib.T[0] * 0.001f;
        }
    }

    void R200Camera::SetStreamIntent(bool depth, bool color)
    {
        uint8_t streamIntent = 0;
        if(depth) streamIntent |= STATUS_BIT_Z_STREAMING;
        if(color) streamIntent |= STATUS_BIT_WEB_STREAMING;
        if(uvc_device_handle_t * handle = GetHandleToAnyStream())
        {
            if(!xu_write(handle, CONTROL_STREAM_INTENT, &streamIntent, sizeof(streamIntent)))
            {
                throw std::runtime_error("xu_write failed");
            }
        }
    }
}
#endif
