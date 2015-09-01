#include "r200.h"
#include "r200-private.h"

namespace rsimpl
{
    int decode_dinghy_frame_number(const subdevice_mode & mode, const void * frame)
    {
        auto data = reinterpret_cast<const uint8_t *>(frame);
        const r200::Dinghy * dinghy = nullptr;
        switch(mode.format)
        {
        case uvc::frame_format::Y8:   dinghy = reinterpret_cast<const r200::Dinghy *>(data +  mode.width * (mode.height-1)   ); break;
        case uvc::frame_format::Y12I: dinghy = reinterpret_cast<const r200::Dinghy *>(data + (mode.width * (mode.height-1))*3); break;
        case uvc::frame_format::Z16:  dinghy = reinterpret_cast<const r200::Dinghy *>(data + (mode.width * (mode.height-1))*2); break;
        }
        assert(dinghy);
        // Todo: check dinghy->magicNumber against 0x08070605 (IR), 0x4030201 (Z), 0x8A8B8C8D (Third)
        return dinghy->frameCount;
    }

    int decode_yuy2_frame_number(const subdevice_mode & mode, const void * frame)
    {
        assert(mode.format == uvc::frame_format::YUYV);

        auto data = reinterpret_cast<const uint8_t *>(frame) + ((mode.width * mode.height) - 32) * 2;
        int number = 0;
        for(int i = 0; i < 32; ++i)
        {
            number |= ((*data & 1) << (i & 1 ? 32 - i : 30 - i));
            data += 2;
        }
        return number;
    }

    enum { LR_FULL, LR_BIG, Z_FULL, Z_BIG, THIRD_HD, THIRD_VGA, NUM_INTRINSICS };
    static static_camera_info get_r200_info()
    {
        static_camera_info info;
        for(auto fps : {30, 60, 90})
        {
            auto uvcFps = fps == 60 ? 59 : fps; // UVC sees the 60 fps mode as 59 fps

            info.stream_subdevices[RS_STREAM_INFRARED  ] = 0;
            info.stream_subdevices[RS_STREAM_INFRARED_2] = 0;
            info.subdevice_modes.push_back({0,  640, 481, uvc::frame_format::Y8,   uvcFps, {{RS_STREAM_INFRARED,   640,  480, RS_FORMAT_Y8, fps, LR_FULL}}, &unpack_strided_image, &decode_dinghy_frame_number});
            info.subdevice_modes.push_back({0,  640, 481, uvc::frame_format::Y12I, uvcFps, {{RS_STREAM_INFRARED,   640,  480, RS_FORMAT_Y8, fps, LR_FULL},
                                                                                            {RS_STREAM_INFRARED_2, 640,  480, RS_FORMAT_Y8, fps, LR_FULL}}, &unpack_rly12_to_y8, &decode_dinghy_frame_number});
            info.subdevice_modes.push_back({0,  640, 373, uvc::frame_format::Y8,   uvcFps, {{RS_STREAM_INFRARED,   492,  372, RS_FORMAT_Y8, fps, LR_BIG }}, &unpack_strided_image, &decode_dinghy_frame_number});
            info.subdevice_modes.push_back({0,  640, 373, uvc::frame_format::Y12I, uvcFps, {{RS_STREAM_INFRARED,   492,  372, RS_FORMAT_Y8, fps, LR_BIG },
                                                                                            {RS_STREAM_INFRARED_2, 492,  372, RS_FORMAT_Y8, fps, LR_BIG }}, &unpack_rly12_to_y8, &decode_dinghy_frame_number});

            info.stream_subdevices[RS_STREAM_DEPTH] = 1;
            info.subdevice_modes.push_back({1,  628, 469, uvc::frame_format::Z16,  uvcFps, {{RS_STREAM_DEPTH,      628,  468, RS_FORMAT_Z16, fps, Z_FULL}}, &unpack_strided_image, &decode_dinghy_frame_number});
            info.subdevice_modes.push_back({1,  628, 361, uvc::frame_format::Z16,  uvcFps, {{RS_STREAM_DEPTH,      480,  360, RS_FORMAT_Z16, fps, Z_BIG }}, &unpack_strided_image, &decode_dinghy_frame_number});

            if(fps == 90) continue;
            info.stream_subdevices[RS_STREAM_COLOR] = 2;
            info.subdevice_modes.push_back({2, 1920, 1080, uvc::frame_format::YUYV, uvcFps, {{RS_STREAM_COLOR,    1920, 1080, RS_FORMAT_RGB8, fps, THIRD_HD }}, &unpack_yuyv_to_rgb, &decode_yuy2_frame_number});
            info.subdevice_modes.push_back({2,  640,  480, uvc::frame_format::YUYV, uvcFps, {{RS_STREAM_COLOR,     640,  480, RS_FORMAT_RGB8, fps, THIRD_VGA}}, &unpack_yuyv_to_rgb, &decode_yuy2_frame_number});
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

        for(int i = RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED; i <= RS_OPTION_R200_DISPARITY_SHIFT; ++i) info.option_supported[i] = true;
        return info;
    }

    r200_camera::r200_camera(uvc::device device) : rs_camera(device, get_r200_info())
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
        if(first_handle)
        {
            uint8_t streamIntent = 0;
            if(subdevices[0]) streamIntent |= STATUS_BIT_LR_STREAMING;
            if(subdevices[1]) streamIntent |= STATUS_BIT_Z_STREAMING;
            if(subdevices[2]) streamIntent |= STATUS_BIT_WEB_STREAMING;
            r200::set_stream_intent(first_handle, streamIntent);
        }
    }

    void r200_camera::set_option(rs_option option, int value)
    {
        if(!first_handle) throw std::runtime_error("cannot call before rs_start_capture(...)");

        //r200::auto_exposure_params aep;
        //r200::depth_params dp;
        r200::disparity_mode dm;
        uint32_t u32[2];
        uint16_t u16[2];

        // TODO: Range check value before write
        switch(option)
        {
        case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED:
            r200::set_lr_exposure_mode(first_handle, value);
            break;
        case RS_OPTION_R200_LR_GAIN:
            r200::get_lr_gain(first_handle, u32[0], u32[1]);
            r200::set_lr_gain(first_handle, u32[0], value);
            break;
        case RS_OPTION_R200_LR_EXPOSURE:
            r200::get_lr_exposure(first_handle, u32[0], u32[1]);
            r200::set_lr_exposure(first_handle, u32[0], value);
            break;
        case RS_OPTION_R200_EMITTER_ENABLED:
            r200::set_emitter_state(first_handle, !!value);
            break;
        case RS_OPTION_R200_DEPTH_CONTROL_PRESET:
            r200::set_depth_params(first_handle, r200::depth_params::presets[value]);
            break;
        case RS_OPTION_R200_DEPTH_UNITS:
            r200::set_depth_units(first_handle, value);
            break;
        case RS_OPTION_R200_DEPTH_CLAMP_MIN:
            r200::get_min_max_depth(first_handle, u16[0], u16[1]);
            r200::set_min_max_depth(first_handle, value, u16[1]);
            break;
        case RS_OPTION_R200_DEPTH_CLAMP_MAX:
            r200::get_min_max_depth(first_handle, u16[0], u16[1]);
            r200::set_min_max_depth(first_handle, u16[0], value);
            break;
        case RS_OPTION_R200_DISPARITY_MODE_ENABLED:
            r200::get_disparity_mode(first_handle, dm);
            dm.format = value ? r200::range_format::RANGE_FORMAT_DISPARITY : r200::range_format::RANGE_FORMAT_DISTANCE;
            r200::set_disparity_mode(first_handle, dm);
            break;
        case RS_OPTION_R200_DISPARITY_MULTIPLIER:
            r200::get_disparity_mode(first_handle, dm);
            dm.multiplier = value;
            r200::set_disparity_mode(first_handle, dm);
            break;
        case RS_OPTION_R200_DISPARITY_SHIFT:
            r200::set_disparity_shift(first_handle, value);
            break;
        }
    }

    int r200_camera::get_option(rs_option option)
    {
        if(!first_handle) throw std::runtime_error("cannot call before rs_start_capture(...)");

        //r200::auto_exposure_params aep;
        r200::depth_params dp;
        r200::disparity_mode dm;
        uint32_t u32[2];
        uint16_t u16[2];
        bool b;

        int value = 0;
        switch(option)
        {
        case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED: r200::get_lr_exposure_mode(first_handle, u32[0]);         value = u32[0]; break;
        case RS_OPTION_R200_LR_GAIN:                  r200::get_lr_gain         (first_handle, u32[0], u32[1]); value = u32[1]; break;
        case RS_OPTION_R200_LR_EXPOSURE:              r200::get_lr_exposure     (first_handle, u32[0], u32[1]); value = u32[1]; break;
        case RS_OPTION_R200_EMITTER_ENABLED:          r200::get_emitter_state   (first_handle, b);              value = b; break;
        case RS_OPTION_R200_DEPTH_UNITS:              r200::get_depth_units     (first_handle, u32[0]);         value = u32[0]; break;
        case RS_OPTION_R200_DEPTH_CLAMP_MIN:          r200::get_min_max_depth   (first_handle, u16[0], u16[1]); value = u16[0]; break;
        case RS_OPTION_R200_DEPTH_CLAMP_MAX:          r200::get_min_max_depth   (first_handle, u16[0], u16[1]); value = u16[1]; break;
        case RS_OPTION_R200_DISPARITY_MODE_ENABLED:   r200::get_disparity_mode  (first_handle, dm);             value = dm.format == r200::range_format::RANGE_FORMAT_DISPARITY; break;
        case RS_OPTION_R200_DISPARITY_MULTIPLIER:     r200::get_disparity_mode  (first_handle, dm);             value = static_cast<int>(dm.multiplier); break;
        case RS_OPTION_R200_DISPARITY_SHIFT:          r200::get_disparity_shift (first_handle, u32[0]);         value = u32[0]; break;
        case RS_OPTION_R200_DEPTH_CONTROL_PRESET:
            r200::get_depth_params(first_handle, dp);
            for(int i=0; i<r200::depth_params::MAX_PRESETS; ++i)
            {
                if(memcmp(&dp, &r200::depth_params::presets[i], sizeof(dp)) == 0)
                {
                    return i;
                }
            }
            break;
        }
        return value;
    }
    
}
