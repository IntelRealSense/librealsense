#include "r200.h"
#include "r200-private.h"
#include "image.h"

#include <cstring>
#include <algorithm>

namespace rsimpl
{
    int decode_dinghy_frame_number(const subdevice_mode & mode, const void * frame)
    {
        auto data = reinterpret_cast<const uint8_t *>(frame);
        const r200::Dinghy * dinghy = nullptr;
        switch(mode.fourcc)
        {
        case 'Y8  ': dinghy = reinterpret_cast<const r200::Dinghy *>(data +  mode.width * (mode.height-1)   ); break;
        case 'Y12I': dinghy = reinterpret_cast<const r200::Dinghy *>(data + (mode.width * (mode.height-1))*3); break;
        case 'Z16 ': dinghy = reinterpret_cast<const r200::Dinghy *>(data + (mode.width * (mode.height-1))*2); break;
        }
        assert(dinghy);
        // Todo: check dinghy->magicNumber against 0x08070605 (IR), 0x4030201 (Z), 0x8A8B8C8D (Third)
        return dinghy->frameCount;
    }

    int decode_yuy2_frame_number(const subdevice_mode & mode, const void * frame)
    {
        assert(mode.fourcc == 'YUY2');

        auto data = reinterpret_cast<const uint8_t *>(frame) + ((mode.width * mode.height) - 32) * 2;
        int number = 0;
        for(int i = 0; i < 32; ++i)
        {
            number |= ((*data & 1) << (i & 1 ? 32 - i : 30 - i));
            data += 2;
        }
        return number;
    }

    static rs_intrinsics MakeLeftRightIntrinsics(const r200::RectifiedIntrinsics & i)
    {
        return {(int)i.rw, (int)i.rh, i.rpx, i.rpy, i.rfx, i.rfy, RS_DISTORTION_NONE, {0,0,0,0,0}};
    }

    static rs_intrinsics MakeDepthIntrinsics(const r200::RectifiedIntrinsics & i)
    {
        return {(int)i.rw-12, (int)i.rh-12, i.rpx-6, i.rpy-6, i.rfx, i.rfy, RS_DISTORTION_NONE, {0,0,0,0,0}};
    }

    static rs_intrinsics MakeColorIntrinsics(const r200::UnrectifiedIntrinsics & i)
    {
        return {(int)i.w, (int)i.h, i.px, i.py, i.fx, i.fy, RS_DISTORTION_MODIFIED_BROWN_CONRADY, {i.k[0],i.k[1],i.k[2],i.k[3],i.k[4]}};
    }

    static static_device_info get_r200_info(uvc::device device)
    {
        enum { LR_FULL, LR_BIG, Z_FULL, Z_BIG, THIRD_HD, THIRD_VGA, NUM_INTRINSICS };
        static_device_info info;
        info.name = {"Intel RealSense R200"};
        for(auto fps : {30, 60, 90})
        {
            info.stream_subdevices[RS_STREAM_INFRARED ] = 0;
            info.stream_subdevices[RS_STREAM_INFRARED2] = 0;
            info.subdevice_modes.push_back({0,  640, 481, 'Y8  ', fps, {{RS_STREAM_INFRARED,  640,  480, RS_FORMAT_Y8,  fps, LR_FULL}}, &unpack_subrect, &decode_dinghy_frame_number});
            info.subdevice_modes.push_back({0,  640, 481, 'Y12I', fps, {{RS_STREAM_INFRARED,  640,  480, RS_FORMAT_Y8,  fps, LR_FULL},
                                                                        {RS_STREAM_INFRARED2, 640,  480, RS_FORMAT_Y8,  fps, LR_FULL}}, &unpack_y8_y8_from_y12i, &decode_dinghy_frame_number});
            info.subdevice_modes.push_back({0,  640, 481, 'Y12I', fps, {{RS_STREAM_INFRARED,  640,  480, RS_FORMAT_Y16, fps, LR_FULL},
                                                                        {RS_STREAM_INFRARED2, 640,  480, RS_FORMAT_Y16, fps, LR_FULL}}, &unpack_y16_y16_from_y12i, &decode_dinghy_frame_number});
            info.subdevice_modes.push_back({0,  640, 373, 'Y8  ', fps, {{RS_STREAM_INFRARED,  492,  372, RS_FORMAT_Y8,  fps, LR_BIG }}, &unpack_subrect, &decode_dinghy_frame_number});
            info.subdevice_modes.push_back({0,  640, 373, 'Y12I', fps, {{RS_STREAM_INFRARED,  492,  372, RS_FORMAT_Y8,  fps, LR_BIG },
                                                                        {RS_STREAM_INFRARED2, 492,  372, RS_FORMAT_Y8,  fps, LR_BIG }}, &unpack_y8_y8_from_y12i, &decode_dinghy_frame_number});
            info.subdevice_modes.push_back({0,  640, 373, 'Y12I', fps, {{RS_STREAM_INFRARED,  492,  372, RS_FORMAT_Y16, fps, LR_BIG },
                                                                        {RS_STREAM_INFRARED2, 492,  372, RS_FORMAT_Y16, fps, LR_BIG }}, &unpack_y16_y16_from_y12i, &decode_dinghy_frame_number});

            info.stream_subdevices[RS_STREAM_DEPTH] = 1;
            info.subdevice_modes.push_back({1,  628, 469, 'Z16 ', fps, {{RS_STREAM_DEPTH,      628,  468, RS_FORMAT_Z16, fps, Z_FULL}}, &unpack_subrect, &decode_dinghy_frame_number});
            info.subdevice_modes.push_back({1,  628, 361, 'Z16 ', fps, {{RS_STREAM_DEPTH,      480,  360, RS_FORMAT_Z16, fps, Z_BIG }}, &unpack_subrect, &decode_dinghy_frame_number});

            if(fps == 90) continue;
            info.stream_subdevices[RS_STREAM_COLOR] = 2;
            info.subdevice_modes.push_back({2, 1920, 1080, 'YUY2', fps, {{RS_STREAM_COLOR,    1920, 1080, RS_FORMAT_YUYV, fps, THIRD_HD }}, &unpack_subrect, &decode_yuy2_frame_number, true});
            info.subdevice_modes.push_back({2,  640,  480, 'YUY2', fps, {{RS_STREAM_COLOR,     640,  480, RS_FORMAT_YUYV, fps, THIRD_VGA}}, &unpack_subrect, &decode_yuy2_frame_number, true});
        }

        for(auto ir : {RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
        {
            info.interstream_rules.push_back({RS_STREAM_DEPTH, ir, &stream_request::width, 12});
            info.interstream_rules.push_back({RS_STREAM_DEPTH, ir, &stream_request::height, 12});
            info.interstream_rules.push_back({RS_STREAM_DEPTH, ir, &stream_request::fps, 0});
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

        for(int i=0; i<RS_PRESET_COUNT; ++i) info.presets[RS_STREAM_INFRARED2][i] = info.presets[RS_STREAM_INFRARED][i];

        for(int i = RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED; i <= RS_OPTION_R200_DISPARITY_SHIFT; ++i) info.option_supported[i] = true;

        r200::CameraCalibrationParameters c;
        r200::CameraHeaderInfo h;
        r200::read_camera_info(device, c, h);

        info.intrinsics.resize(NUM_INTRINSICS);
        info.intrinsics[LR_FULL] = MakeLeftRightIntrinsics(c.modesLR[0]);
        info.intrinsics[LR_BIG] = MakeLeftRightIntrinsics(c.modesLR[1]);
        info.intrinsics[Z_FULL] = MakeDepthIntrinsics(c.modesLR[0]);
        info.intrinsics[Z_BIG] = MakeDepthIntrinsics(c.modesLR[1]);
        info.intrinsics[THIRD_HD] = MakeColorIntrinsics(c.intrinsicsThird[0]);
        info.intrinsics[THIRD_VGA] = MakeColorIntrinsics(c.intrinsicsThird[1]);
        info.stream_poses[RS_STREAM_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_INFRARED] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_INFRARED2] = {{{1,0,0},{0,1,0},{0,0,1}}, {c.B[0] * 0.001f, 0, 0}};
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) info.stream_poses[RS_STREAM_COLOR].orientation(i,j) = c.Rthird[0][i*3+j];
        for(int i=0; i<3; ++i) info.stream_poses[RS_STREAM_COLOR].position[i] = c.T[0][i] * 0.001f;
        info.stream_poses[RS_STREAM_COLOR].position = info.stream_poses[RS_STREAM_COLOR].orientation * info.stream_poses[RS_STREAM_COLOR].position;
        info.depth_scale = 0.001f;

        return info;
    }

    r200_camera::r200_camera(uvc::device device) : rs_device(device)
    {
		device_info = add_standard_unpackers(get_r200_info(device));
    }
    
    r200_camera::~r200_camera()
    {
        r200::force_firmware_reset(device);
    }

    void r200_camera::on_before_start(const std::vector<subdevice_mode> & selected_modes)
    {
        uint8_t streamIntent = 0;
        for(const auto & m : selected_modes)
        {
            switch(m.subdevice)
            {
            case 0: streamIntent |= r200::STATUS_BIT_LR_STREAMING; break;
            case 1: streamIntent |= r200::STATUS_BIT_Z_STREAMING; break;
            case 2: streamIntent |= r200::STATUS_BIT_WEB_STREAMING; break;
            }
        }
        r200::set_stream_intent(device, streamIntent);
    }

    int r200_camera::convert_timestamp(int64_t timestamp) const
    { 
        int max_fps = 0;
        for(auto & req : requests) if(req.enabled) max_fps = std::max(max_fps, req.fps);
        return static_cast<int>(timestamp * 1000 / max_fps);
    }

    void r200_camera::set_option(rs_option option, int value)
    {
        //r200::auto_exposure_params aep;
        //r200::depth_params dp;
        r200::disparity_mode dm;
        uint32_t u32[2];
        uint16_t u16[2];

        // TODO: Range check value before write
        switch(option)
        {
        case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED:
            r200::set_lr_exposure_mode(device, value);
            break;
        case RS_OPTION_R200_LR_GAIN:
            r200::get_lr_gain(device, u32[0], u32[1]);
            r200::set_lr_gain(device, u32[0], value);
            break;
        case RS_OPTION_R200_LR_EXPOSURE:
            r200::get_lr_exposure(device, u32[0], u32[1]);
            r200::set_lr_exposure(device, u32[0], value);
            break;
        case RS_OPTION_R200_EMITTER_ENABLED:
            r200::set_emitter_state(device, !!value);
            break;
        case RS_OPTION_R200_DEPTH_CONTROL_PRESET:
            r200::set_depth_params(device, r200::depth_params::presets[value]);
            break;
        case RS_OPTION_R200_DEPTH_UNITS:
            r200::set_depth_units(device, value);
            break;
        case RS_OPTION_R200_DEPTH_CLAMP_MIN:
            r200::get_min_max_depth(device, u16[0], u16[1]);
            r200::set_min_max_depth(device, value, u16[1]);
            break;
        case RS_OPTION_R200_DEPTH_CLAMP_MAX:
            r200::get_min_max_depth(device, u16[0], u16[1]);
            r200::set_min_max_depth(device, u16[0], value);
            break;
        case RS_OPTION_R200_DISPARITY_MODE_ENABLED:
            r200::get_disparity_mode(device, dm);
            dm.format = value ? r200::range_format::RANGE_FORMAT_DISPARITY : r200::range_format::RANGE_FORMAT_DISTANCE;
            r200::set_disparity_mode(device, dm);
            break;
        case RS_OPTION_R200_DISPARITY_MULTIPLIER:
            r200::get_disparity_mode(device, dm);
            dm.multiplier = value;
            r200::set_disparity_mode(device, dm);
            break;
        case RS_OPTION_R200_DISPARITY_SHIFT:
            r200::set_disparity_shift(device, value);
            break;
        }
    }

    int r200_camera::get_option(rs_option option) const
    {
        if(!device) throw std::runtime_error("cannot call before rs_start_capture(...)");

        //r200::auto_exposure_params aep;
        r200::depth_params dp;
        r200::disparity_mode dm;
        uint32_t u32[2];
        uint16_t u16[2];
        bool b;

        int value = 0;
        switch(option)
        {
        case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED: r200::get_lr_exposure_mode(device, u32[0]);         value = u32[0]; break;
        case RS_OPTION_R200_LR_GAIN:                  r200::get_lr_gain         (device, u32[0], u32[1]); value = u32[1]; break;
        case RS_OPTION_R200_LR_EXPOSURE:              r200::get_lr_exposure     (device, u32[0], u32[1]); value = u32[1]; break;
        case RS_OPTION_R200_EMITTER_ENABLED:          r200::get_emitter_state   (device, b);              value = b; break;
        case RS_OPTION_R200_DEPTH_UNITS:              r200::get_depth_units     (device, u32[0]);         value = u32[0]; break;
        case RS_OPTION_R200_DEPTH_CLAMP_MIN:          r200::get_min_max_depth   (device, u16[0], u16[1]); value = u16[0]; break;
        case RS_OPTION_R200_DEPTH_CLAMP_MAX:          r200::get_min_max_depth   (device, u16[0], u16[1]); value = u16[1]; break;
        case RS_OPTION_R200_DISPARITY_MODE_ENABLED:   r200::get_disparity_mode  (device, dm);             value = dm.format == r200::range_format::RANGE_FORMAT_DISPARITY; break;
        case RS_OPTION_R200_DISPARITY_MULTIPLIER:     r200::get_disparity_mode  (device, dm);             value = static_cast<int>(dm.multiplier); break;
        case RS_OPTION_R200_DISPARITY_SHIFT:          r200::get_disparity_shift (device, u32[0]);         value = u32[0]; break;
        case RS_OPTION_R200_DEPTH_CONTROL_PRESET:
            r200::get_depth_params(device, dp);
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
