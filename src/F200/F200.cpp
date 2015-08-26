#include "F200.h"
#include "HardwareIO.h"
#include "../../include/librealsense/rsutil.h"

namespace rsimpl { namespace f200
{
    enum { COLOR_480P, COLOR_1080P, DEPTH_480P, NUM_INTRINSICS };
    static StaticCameraInfo get_f200_info()
    {
        StaticCameraInfo info;
        // Color modes on subdevice 0
        info.stream_subdevices[RS_STREAM_COLOR] = 0;
        info.subdevice_modes.push_back({0, 640, 480, UVC_FRAME_FORMAT_YUYV, 60, {{RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60, COLOR_480P}}, &unpack_yuyv_to_rgb});
        info.subdevice_modes.push_back({0, 1920, 1080, UVC_FRAME_FORMAT_YUYV, 60, {{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_RGB8, 60, COLOR_1080P}}, &unpack_yuyv_to_rgb});
        // Depth modes on subdevice 1
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.subdevice_modes.push_back({1, 640, 480, UVC_FRAME_FORMAT_INVR, 60, {{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60, DEPTH_480P}}, &unpack_strided_image});
        return info;
    }

    F200Camera::F200Camera(uvc_context_t * ctx, uvc_device_t * device) : rs_camera(ctx, device, get_f200_info())
    {

    }

    F200Camera::~F200Camera()
    {
        
    }

    static rs_intrinsics MakeDepthIntrinsics(const CameraCalibrationParameters & c, int w, int h)
    {
        rs_intrinsics intrin = {{w,h}};
        intrin.focal_length[0] = c.Kc[0][0]*0.5f * w;
        intrin.focal_length[1] = c.Kc[1][1]*0.5f * h;
        intrin.principal_point[0] = (c.Kc[0][2]*0.5f + 0.5f) * w;
        intrin.principal_point[1] = (c.Kc[1][2]*0.5f + 0.5f) * h;
        for(int i=0; i<5; ++i) intrin.distortion_coeff[i] = c.Invdistc[i];
        intrin.distortion_model = RS_DISTORTION_INVERSE_BROWN_CONRADY;
        return intrin;
    }

    static rs_intrinsics MakeColorIntrinsics(const CameraCalibrationParameters & c, int w, int h)
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
        intrin.distortion_model = RS_DISTORTION_NONE;
        return intrin;
    }

    CalibrationInfo F200Camera::RetrieveCalibration()
    {
        if(!hardware_io) hardware_io.reset(new IVCAMHardwareIO(context));
        const CameraCalibrationParameters & calib = hardware_io->GetParameters();

        CalibrationInfo c;
        c.intrinsics.resize(NUM_INTRINSICS);
        c.intrinsics[COLOR_480P] = MakeColorIntrinsics(calib, 640, 480);
        c.intrinsics[COLOR_1080P] = MakeColorIntrinsics(calib, 1920, 1080);
        c.intrinsics[DEPTH_480P] = MakeDepthIntrinsics(calib, 640, 480);
        c.stream_poses[RS_STREAM_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        c.stream_poses[RS_STREAM_COLOR] = {transpose((const float3x3 &)calib.Rt), (const float3 &)calib.Tt * 0.001f}; // convert mm to m
        c.depth_scale = (calib.Rmax / 0xFFFF) * 0.001f; // convert mm to m
        return c;
    }

} } // namespace rsimpl::f200

