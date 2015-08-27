#include "r200.h"
#include "r200-private.h"

namespace rsimpl
{
    enum { LR_FULL, LR_BIG, Z_FULL, Z_BIG, THIRD_HD, THIRD_VGA, NUM_INTRINSICS };
    static static_camera_info get_r200_info()
    {
        static_camera_info info;
        for(auto fps : {30, 60, 90})
        {
            auto uvcFps = fps == 60 ? 59 : fps; // UVC sees the 60 fps mode as 59 fps

            info.stream_subdevices[RS_STREAM_INFRARED  ] = 0;
            info.stream_subdevices[RS_STREAM_INFRARED_2] = 0;
            info.subdevice_modes.push_back({0,  640, 481, UVC_FRAME_FORMAT_Y8,   uvcFps, {{RS_STREAM_INFRARED,   640,  480, RS_FORMAT_Y8, fps, LR_FULL}}, &unpack_strided_image});
            info.subdevice_modes.push_back({0,  640, 481, UVC_FRAME_FORMAT_Y12I, uvcFps, {{RS_STREAM_INFRARED,   640,  480, RS_FORMAT_Y8, fps, LR_FULL},
                                                                                          {RS_STREAM_INFRARED_2, 640,  480, RS_FORMAT_Y8, fps, LR_FULL}}, &unpack_rly12_to_y8});
            info.subdevice_modes.push_back({0,  640, 373, UVC_FRAME_FORMAT_Y8,   uvcFps, {{RS_STREAM_INFRARED,   492,  372, RS_FORMAT_Y8, fps, LR_BIG }}, &unpack_strided_image});
            info.subdevice_modes.push_back({0,  640, 373, UVC_FRAME_FORMAT_Y12I, uvcFps, {{RS_STREAM_INFRARED,   492,  372, RS_FORMAT_Y8, fps, LR_BIG },
                                                                                          {RS_STREAM_INFRARED_2, 492,  372, RS_FORMAT_Y8, fps, LR_BIG }}, &unpack_rly12_to_y8});

            info.stream_subdevices[RS_STREAM_DEPTH] = 1;
            info.subdevice_modes.push_back({1,  628, 469, UVC_FRAME_FORMAT_Z16,  uvcFps, {{RS_STREAM_DEPTH,      628,  468, RS_FORMAT_Z16, fps, Z_FULL}}, &unpack_strided_image});
            info.subdevice_modes.push_back({1,  628, 361, UVC_FRAME_FORMAT_Z16,  uvcFps, {{RS_STREAM_DEPTH,      480,  360, RS_FORMAT_Z16, fps, Z_BIG }}, &unpack_strided_image});

            if(fps == 90) continue;
            info.stream_subdevices[RS_STREAM_COLOR] = 2;
            info.subdevice_modes.push_back({2, 1920, 1080, UVC_FRAME_FORMAT_YUYV, uvcFps, {{RS_STREAM_COLOR,    1920, 1080, RS_FORMAT_RGB8, fps, THIRD_HD }}, &unpack_yuyv_to_rgb});
            info.subdevice_modes.push_back({2,  640,  480, UVC_FRAME_FORMAT_YUYV, uvcFps, {{RS_STREAM_COLOR,     640,  480, RS_FORMAT_RGB8, fps, THIRD_VGA}}, &unpack_yuyv_to_rgb});
        }

        info.presets[RS_STREAM_INFRARED][RS_PRESET_BEST_QUALITY] = {true, 492, 372, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_BEST_QUALITY] = {true, 480, 360, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_LARGEST_IMAGE] = {true,  628,  468, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_LARGEST_IMAGE] = {true, 1920, 1080, RS_FORMAT_RGB8, 30};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_HIGHEST_FRAMERATE] = {true, 492, 372, RS_FORMAT_Y8,   90};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 480, 360, RS_FORMAT_Z16,  90};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        for(int i=0; i<RS_PRESET_NUM; ++i) info.presets[RS_STREAM_INFRARED_2][i] = info.presets[RS_STREAM_INFRARED][i];

        return info;
    }

    r200_camera::r200_camera(uvc_context_t * ctx, uvc_device_t * device) : rs_camera(ctx, device, get_r200_info())
    {

    }
    
    r200_camera::~r200_camera()
    {
        if(first_handle) r200::force_firmware_reset(first_handle);
    }

    static rs_intrinsics MakeLeftRightIntrinsics(const r200::RectifiedIntrinsics & i)
    {
        return {{(int)i.rw, (int)i.rh}, {i.rfx, i.rfy}, {i.rpx, i.rpy}, {0,0,0,0,0}, RS_DISTORTION_NONE};
    }

    static rs_intrinsics MakeDepthIntrinsics(const r200::RectifiedIntrinsics & i)
    {
        return {{(int)i.rw-12, (int)i.rh-12}, {i.rfx, i.rfy}, {i.rpx-6, i.rpy-6}, {0,0,0,0,0}, RS_DISTORTION_NONE};
    }

    static rs_intrinsics MakeColorIntrinsics(const r200::UnrectifiedIntrinsics & i)
    {
        return {{(int)i.w, (int)i.h}, {i.fx,i.fy}, {i.px,i.py}, {i.k[0],i.k[1],i.k[2],i.k[3],i.k[4]}, RS_DISTORTION_GORDON_BROWN_CONRADY};
    }

    calibration_info r200_camera::retrieve_calibration()
    {
        r200::CameraCalibrationParameters calib;
        r200::CameraHeaderInfo header;
        r200::read_camera_info(first_handle, calib, header);

        calibration_info c;
        c.intrinsics.resize(NUM_INTRINSICS);
        c.intrinsics[LR_FULL] = MakeLeftRightIntrinsics(calib.modesLR[0]);
        c.intrinsics[LR_BIG] = MakeLeftRightIntrinsics(calib.modesLR[1]);
        c.intrinsics[Z_FULL] = MakeDepthIntrinsics(calib.modesLR[0]);
        c.intrinsics[Z_BIG] = MakeDepthIntrinsics(calib.modesLR[1]);
        c.intrinsics[THIRD_HD] = MakeColorIntrinsics(calib.intrinsicsThird[0]);
        c.intrinsics[THIRD_VGA] = MakeColorIntrinsics(calib.intrinsicsThird[1]);
        c.stream_poses[RS_STREAM_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        c.stream_poses[RS_STREAM_INFRARED] = c.stream_poses[RS_STREAM_DEPTH];
        c.stream_poses[RS_STREAM_INFRARED_2] = c.stream_poses[RS_STREAM_DEPTH]; // TODO: Figure out the correct translation vector to put here
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) c.stream_poses[RS_STREAM_COLOR].orientation(i,j) = calib.Rthird[0][i*3+j];
        for(int i=0; i<3; ++i) c.stream_poses[RS_STREAM_COLOR].position[i] = calib.T[0][i] * 0.001f;
        c.stream_poses[RS_STREAM_COLOR].position = c.stream_poses[RS_STREAM_COLOR].orientation * c.stream_poses[RS_STREAM_COLOR].position;
        c.depth_scale = 0.001f;
        return c;
    }

    void r200_camera::set_stream_intent()
    {
        uint8_t streamIntent = 0;
        if(subdevices[0]) streamIntent |= STATUS_BIT_LR_STREAMING;
        if(subdevices[1]) streamIntent |= STATUS_BIT_Z_STREAMING;
        if(subdevices[2]) streamIntent |= STATUS_BIT_WEB_STREAMING;

        if(first_handle)
        {
            if (!r200::set_stream_intent(first_handle, streamIntent)) throw std::runtime_error("could not set stream intent");
        }
    }
}

