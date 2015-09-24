#include "r200.h"
#include "r200-private.h"
#include "image.h"

#include <cstring>
#include <algorithm>

namespace rsimpl
{
    int decode_dinghy_frame_number(const subdevice_mode & mode, const void * frame)
    {
        // Todo: check dinghy->magicNumber against 0x08070605 (IR), 0x4030201 (Z), 0x8A8B8C8D (Third)
        return reinterpret_cast<const r200::Dinghy *>(reinterpret_cast<const uint8_t *>(frame) + get_image_size(mode.width, mode.streams[0].height, mode.fourcc))->frameCount;
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

    r200_camera::r200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, std::vector<rs_intrinsics> intrinsics) : rs_device(device, info)
    {
        set_intrinsics_thread_safe(intrinsics);
    }
    
    r200_camera::~r200_camera()
    {
        r200::force_firmware_reset(get_device());
    }

    std::shared_ptr<rs_device> make_r200_device(std::shared_ptr<uvc::device> device)
    {
        const uvc::guid DS_LEFT_RIGHT_XU = {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}};
        init_controls(*device, 0, DS_LEFT_RIGHT_XU);

        enum { LR_FULL, LR_BIG, LR_QRES, Z_FULL, Z_BIG, Z_QRES, THIRD_HD, THIRD_VGA, NUM_INTRINSICS };
        const static struct { int w, h, uvc_w, uvc_h, lr_intrin, z_intrin; } lrz_modes[] = {
            {640, 480,   640, 481,  LR_FULL, Z_FULL},
            {492, 372,   640, 373,  LR_BIG,  Z_BIG },
            // {332, 252,   640, 254,  LR_QRES, Z_QRES} // TODO: Fix QRES
        };

        static_device_info info;
        info.name = {"Intel RealSense R200"};
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_COLOR] = 2;
        info.stream_subdevices[RS_STREAM_INFRARED ] = 0;
        info.stream_subdevices[RS_STREAM_INFRARED2] = 0;            

        // Set up modes for left/right/z images
        for(auto m : lrz_modes)
        {
            for(auto fps : {30, 60, 90})
            {
                info.subdevice_modes.push_back({1, m.uvc_w-12, m.uvc_h-12, 'Z16 ', fps, {{RS_STREAM_DEPTH, m.w-12, m.h-12, RS_FORMAT_Z16, fps, m.z_intrin}}, &unpack_subrect, &decode_dinghy_frame_number});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, 'Y8  ', fps, {{RS_STREAM_INFRARED,  m.w, m.h, RS_FORMAT_Y8,  fps, m.lr_intrin}}, &unpack_subrect, &decode_dinghy_frame_number});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, 'Y8I ', fps, {{RS_STREAM_INFRARED,  m.w, m.h, RS_FORMAT_Y8,  fps, m.lr_intrin},
                                                                                   {RS_STREAM_INFRARED2, m.w, m.h, RS_FORMAT_Y8,  fps, m.lr_intrin}}, &unpack_y8_y8_from_y8i, &decode_dinghy_frame_number});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, 'Y16 ', fps, {{RS_STREAM_INFRARED,  m.w, m.h, RS_FORMAT_Y16, fps, m.lr_intrin}}, &unpack_y16_from_y16_10, &decode_dinghy_frame_number});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, 'Y12I', fps, {{RS_STREAM_INFRARED,  m.w, m.h, RS_FORMAT_Y16, fps, m.lr_intrin},
                                                                                   {RS_STREAM_INFRARED2, m.w, m.h, RS_FORMAT_Y16, fps, m.lr_intrin}}, &unpack_y16_y16_from_y12i_10, &decode_dinghy_frame_number});
            }
        }

        // Set up modes for third images (TODO: 15?)
        info.subdevice_modes.push_back({2,  640,  480, 'YUY2', 60, {{RS_STREAM_COLOR,  640,  480, RS_FORMAT_YUYV, 60, THIRD_VGA}}, &unpack_subrect, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2,  640,  480, 'YUY2', 30, {{RS_STREAM_COLOR,  640,  480, RS_FORMAT_YUYV, 30, THIRD_VGA}}, &unpack_subrect, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2, 1920, 1080, 'YUY2', 30, {{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30, THIRD_HD }}, &unpack_subrect, &decode_yuy2_frame_number, true});

        // Set up interstream rules for left/right/z images
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
        r200::read_camera_info(*device, c, h);
        
        if (c.metadata.versionNumber != 2)
            throw std::runtime_error("only supported calibration struct is version 2. got (" + std::to_string(c.metadata.versionNumber) + ").");

        std::vector<rs_intrinsics> intrinsics(NUM_INTRINSICS);
        intrinsics[LR_FULL] = MakeLeftRightIntrinsics(c.modesLR[0]);
        intrinsics[LR_BIG] = MakeLeftRightIntrinsics(c.modesLR[1]);
        intrinsics[LR_QRES] = MakeLeftRightIntrinsics(c.modesLR[2]);
        intrinsics[Z_FULL] = MakeDepthIntrinsics(c.modesLR[0]);
        intrinsics[Z_BIG] = MakeDepthIntrinsics(c.modesLR[1]);
        intrinsics[Z_QRES] = MakeDepthIntrinsics(c.modesLR[2]);
        intrinsics[THIRD_HD] = MakeColorIntrinsics(c.intrinsicsThird[0]);
        intrinsics[THIRD_VGA] = MakeColorIntrinsics(c.intrinsicsThird[1]);
        info.stream_poses[RS_STREAM_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_INFRARED] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_INFRARED2] = {{{1,0,0},{0,1,0},{0,0,1}}, {c.B[0] * 0.001f, 0, 0}};
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) info.stream_poses[RS_STREAM_COLOR].orientation(i,j) = c.Rthird[0][i*3+j];
        for(int i=0; i<3; ++i) info.stream_poses[RS_STREAM_COLOR].position[i] = c.T[0][i] * 0.001f;
        info.stream_poses[RS_STREAM_COLOR].position = info.stream_poses[RS_STREAM_COLOR].orientation * info.stream_poses[RS_STREAM_COLOR].position;
        info.depth_scale = 0.001f;

        return std::make_shared<r200_camera>(device, info, intrinsics);
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
        r200::set_stream_intent(get_device(), streamIntent);
    }

    int r200_camera::convert_timestamp(int64_t timestamp) const
    { 
        int max_fps = 0;
        for(int i=0; i<RS_STREAM_COUNT; ++i)
        {
            if(is_stream_enabled((rs_stream)i))
            {
                max_fps = std::max(max_fps, get_stream_framerate((rs_stream)i));
            }
        }
        return static_cast<int>(timestamp * 1000 / max_fps);
    }

    void r200_camera::set_xu_option(rs_option option, int value)
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
            r200::set_lr_exposure_mode(get_device(), value);
            break;
        case RS_OPTION_R200_LR_GAIN:
            r200::get_lr_gain(get_device(), u32[0], u32[1]);
            r200::set_lr_gain(get_device(), u32[0], value);
            break;
        case RS_OPTION_R200_LR_EXPOSURE:
            r200::get_lr_exposure(get_device(), u32[0], u32[1]);
            r200::set_lr_exposure(get_device(), u32[0], value);
            break;
        case RS_OPTION_R200_EMITTER_ENABLED:
            r200::set_emitter_state(get_device(), !!value);
            break;
        case RS_OPTION_R200_DEPTH_CONTROL_PRESET:
            r200::set_depth_params(get_device(), r200::depth_params::presets[value]);
            break;
        case RS_OPTION_R200_DEPTH_UNITS:
            r200::set_depth_units(get_device(), value);
            break;
        case RS_OPTION_R200_DEPTH_CLAMP_MIN:
            r200::get_min_max_depth(get_device(), u16[0], u16[1]);
            r200::set_min_max_depth(get_device(), value, u16[1]);
            break;
        case RS_OPTION_R200_DEPTH_CLAMP_MAX:
            r200::get_min_max_depth(get_device(), u16[0], u16[1]);
            r200::set_min_max_depth(get_device(), u16[0], value);
            break;
        case RS_OPTION_R200_DISPARITY_MODE_ENABLED:
            r200::get_disparity_mode(get_device(), dm);
            dm.format = value ? r200::range_format::RANGE_FORMAT_DISPARITY : r200::range_format::RANGE_FORMAT_DISTANCE;
            r200::set_disparity_mode(get_device(), dm);
            break;
        case RS_OPTION_R200_DISPARITY_MULTIPLIER:
            r200::get_disparity_mode(get_device(), dm);
            dm.multiplier = value;
            r200::set_disparity_mode(get_device(), dm);
            break;
        case RS_OPTION_R200_DISPARITY_SHIFT:
            r200::set_disparity_shift(get_device(), value);
            break;
        }
    }

    int r200_camera::get_xu_option(rs_option option) const
    {
        if(!is_capturing()) throw std::runtime_error("cannot call before rs_start_capture(...)");

        //r200::auto_exposure_params aep;
        r200::depth_params dp;
        r200::disparity_mode dm;
        uint32_t u32[2];
        uint16_t u16[2];
        bool b;

        int value = 0;
        switch(option)
        {
        case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED: r200::get_lr_exposure_mode(get_device(), u32[0]);         value = u32[0]; break;
        case RS_OPTION_R200_LR_GAIN:                  r200::get_lr_gain         (get_device(), u32[0], u32[1]); value = u32[1]; break;
        case RS_OPTION_R200_LR_EXPOSURE:              r200::get_lr_exposure     (get_device(), u32[0], u32[1]); value = u32[1]; break;
        case RS_OPTION_R200_EMITTER_ENABLED:          r200::get_emitter_state   (get_device(), b);              value = b; break;
        case RS_OPTION_R200_DEPTH_UNITS:              r200::get_depth_units     (get_device(), u32[0]);         value = u32[0]; break;
        case RS_OPTION_R200_DEPTH_CLAMP_MIN:          r200::get_min_max_depth   (get_device(), u16[0], u16[1]); value = u16[0]; break;
        case RS_OPTION_R200_DEPTH_CLAMP_MAX:          r200::get_min_max_depth   (get_device(), u16[0], u16[1]); value = u16[1]; break;
        case RS_OPTION_R200_DISPARITY_MODE_ENABLED:   r200::get_disparity_mode  (get_device(), dm);             value = dm.format == r200::range_format::RANGE_FORMAT_DISPARITY; break;
        case RS_OPTION_R200_DISPARITY_MULTIPLIER:     r200::get_disparity_mode  (get_device(), dm);             value = static_cast<int>(dm.multiplier); break;
        case RS_OPTION_R200_DISPARITY_SHIFT:          r200::get_disparity_shift (get_device(), u32[0]);         value = u32[0]; break;
        case RS_OPTION_R200_DEPTH_CONTROL_PRESET:
            r200::get_depth_params(get_device(), dp);
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
