/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

#include "r200.h"
#include "r200-private.h"
#include "image.h"

#include <cstring>
#include <climits>
#include <algorithm>

namespace rsimpl
{
    template<void (*UNPACKER)(byte * const dest[], const byte * source, const subdevice_mode & mode)> 
    void crop_unpack(byte * const dest[], const byte * source, const subdevice_mode & mode)
    {
        UNPACKER(dest, source + mode.pf->get_crop_offset(mode.width, 6), mode);
    }

    static int amount = 0;

    template<void (*UNPACKER)(byte * const dest[], const byte * source, const subdevice_mode & mode)> 
    void pad_unpack(byte * const dest[], const byte * source, const subdevice_mode & mode)
    {
        assert(mode.streams.size() == 1);
        byte * out = dest[0] + get_image_size(mode.streams[0].width, 6, mode.streams[0].format) + get_image_size(6, 1, mode.streams[0].format);
        UNPACKER(&out, source, mode);

        // Erase Dinghy, which will get copied over when blitting into a padded buffer
        memset(out + get_image_size(mode.streams[0].width, mode.height-1, mode.streams[0].format), 0, get_image_size(mode.width, 1, mode.streams[0].format));
    }

    template<unsigned MAGIC_NUMBER>
    int decode_dinghy_frame_number(const subdevice_mode & mode, const void * frame)
    {
        auto dinghy = reinterpret_cast<const r200::Dinghy *>(reinterpret_cast<const uint8_t *>(frame) + mode.pf->get_image_size(mode.width, mode.height-1));
        return dinghy->magicNumber == MAGIC_NUMBER ? dinghy->frameCount : 0;
    }

    int decode_yuy2_frame_number(const subdevice_mode & mode, const void * frame)
    {
        assert(mode.pf == &pf_yuy2);

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

    static rs_intrinsics MakeColorIntrinsics(const r200::UnrectifiedIntrinsics & i, int denom)
    {
        return {(int)i.w/denom, (int)i.h/denom, i.px/denom, i.py/denom, i.fx/denom, i.fy/denom, RS_DISTORTION_MODIFIED_BROWN_CONRADY, {i.k[0],i.k[1],i.k[2],i.k[3],i.k[4]}};
    }

    static rs_intrinsics MakeColorIntrinsics(const r200::RectifiedIntrinsics & i, int denom)
    {
        return {(int)i.rw/denom, (int)i.rh/denom, i.rpx/denom, i.rpy/denom, i.rfx/denom, i.rfy/denom, RS_DISTORTION_NONE, {0,0,0,0,0}};
    }

    r200_camera::r200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, std::vector<rs_intrinsics> intrinsics, std::vector<rs_intrinsics> rect_intrinsics) : rs_device(device, info, intrinsics, rect_intrinsics)
    {
        //config.intrinsics.set(intrinsics, rect_intrinsics);
        on_update_depth_units(get_xu_option(RS_OPTION_R200_DEPTH_UNITS));
    }
    
    r200_camera::~r200_camera()
    {

    }

    std::shared_ptr<rs_device> make_r200_device(std::shared_ptr<uvc::device> device)
    {
		// Retrieve the extension unit for the R200's left/right infrared camera
        // This XU is used for all commands related to retrieving calibration information and setting depth generation settings
        const uvc::guid R200_LEFT_RIGHT_XU = {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}};
        init_controls(*device, 0, R200_LEFT_RIGHT_XU);

        enum { LR_FULL, LR_BIG, LR_QRES, Z_FULL, Z_BIG, Z_QRES, THIRD_HD, THIRD_VGA, THIRD_QRES, NUM_INTRINSICS };
        const static struct { int w, h, uvc_w, uvc_h, lr_intrin, z_intrin; } lrz_modes[] = {
            {640, 480,   640, 481,  LR_FULL, Z_FULL},
            {492, 372,   640, 373,  LR_BIG,  Z_BIG },
            {332, 252,   640, 254,  LR_QRES, Z_QRES}
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
                info.subdevice_modes.push_back({1, m.uvc_w-12, m.uvc_h-12, &pf_z16, fps, m.lr_intrin, {{RS_STREAM_DEPTH, m.w, m.h, RS_FORMAT_Z16}}, &pad_unpack<unpack_subrect>, &decode_dinghy_frame_number<0x4030201>});
                info.subdevice_modes.push_back({1, m.uvc_w-12, m.uvc_h-12, &pf_z16, fps, m.z_intrin,  {{RS_STREAM_DEPTH, m.w-12, m.h-12, RS_FORMAT_Z16}}, &unpack_subrect, &decode_dinghy_frame_number<0x4030201>});
                info.subdevice_modes.push_back({1, m.uvc_w-12, m.uvc_h-12, &pf_z16, fps, m.lr_intrin, {{RS_STREAM_DEPTH, m.w, m.h, RS_FORMAT_DISPARITY16}}, &pad_unpack<unpack_subrect>, &decode_dinghy_frame_number<0x4030201>});
                info.subdevice_modes.push_back({1, m.uvc_w-12, m.uvc_h-12, &pf_z16, fps, m.z_intrin,  {{RS_STREAM_DEPTH, m.w-12, m.h-12, RS_FORMAT_DISPARITY16}}, &unpack_subrect, &decode_dinghy_frame_number<0x4030201>});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, &pf_y8 , fps, m.lr_intrin, {{RS_STREAM_INFRARED,  m.w, m.h, RS_FORMAT_Y8}}, &unpack_subrect, &decode_dinghy_frame_number<0x08070605>});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, &pf_y8 , fps, m.z_intrin,  {{RS_STREAM_INFRARED,  m.w-12, m.h-12, RS_FORMAT_Y8}}, &crop_unpack<unpack_subrect>, &decode_dinghy_frame_number<0x08070605>});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, &pf_y8i, fps, m.lr_intrin, {{RS_STREAM_INFRARED,  m.w, m.h, RS_FORMAT_Y8},
                                                                                                 {RS_STREAM_INFRARED2, m.w, m.h, RS_FORMAT_Y8}}, &unpack_y8_y8_from_y8i, &decode_dinghy_frame_number<0x08070605>});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, &pf_y8i, fps, m.z_intrin,  {{RS_STREAM_INFRARED,  m.w-12, m.h-12, RS_FORMAT_Y8},
                                                                                                 {RS_STREAM_INFRARED2, m.w-12, m.h-12, RS_FORMAT_Y8}}, &crop_unpack<unpack_y8_y8_from_y8i>, &decode_dinghy_frame_number<0x08070605>});
                                                                     
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, &pf_y16, fps, m.lr_intrin, {{RS_STREAM_INFRARED,  m.w, m.h, RS_FORMAT_Y16}}, &unpack_y16_from_y16_10, &decode_dinghy_frame_number<0x08070605>});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, &pf_y16, fps, m.z_intrin,  {{RS_STREAM_INFRARED,  m.w-12, m.h-12, RS_FORMAT_Y16}}, &crop_unpack<unpack_y16_from_y16_10>, &decode_dinghy_frame_number<0x08070605>});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, &pf_y12i, fps, m.lr_intrin, {{RS_STREAM_INFRARED,  m.w, m.h, RS_FORMAT_Y16},
                                                                                                  {RS_STREAM_INFRARED2, m.w, m.h, RS_FORMAT_Y16}}, &unpack_y16_y16_from_y12i_10, &decode_dinghy_frame_number<0x08070605>});
                info.subdevice_modes.push_back({0, m.uvc_w, m.uvc_h, &pf_y12i, fps, m.z_intrin, {{RS_STREAM_INFRARED,  m.w-12, m.h-12, RS_FORMAT_Y16},
                                                                                                 {RS_STREAM_INFRARED2, m.w-12, m.h-12, RS_FORMAT_Y16}}, &crop_unpack<unpack_y16_y16_from_y12i_10>, &decode_dinghy_frame_number<0x08070605>});
            }
        }

        // Set up modes for third images
		info.subdevice_modes.push_back({2,  320,  240, &pf_yuy2, 60, THIRD_QRES, {{RS_STREAM_COLOR,  320,  240, RS_FORMAT_YUYV}}, &unpack_subrect, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2,  320,  240, &pf_yuy2, 30, THIRD_QRES, {{RS_STREAM_COLOR,  320,  240, RS_FORMAT_YUYV}}, &unpack_subrect, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2,  640,  480, &pf_yuy2, 60, THIRD_VGA, {{RS_STREAM_COLOR,  640,  480, RS_FORMAT_YUYV}}, &unpack_subrect, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2,  640,  480, &pf_yuy2, 30, THIRD_VGA, {{RS_STREAM_COLOR,  640,  480, RS_FORMAT_YUYV}}, &unpack_subrect, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2, 1920, 1080, &pf_yuy2, 30, THIRD_HD, {{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV}}, &unpack_subrect, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back(subdevice_mode{2, 2400, 1081, &pf_rw10, 30, THIRD_HD, {{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_RAW10}}, &unpack_subrect, &decode_dinghy_frame_number<0x8A8B8C8D>, true});
		// todo - add 15 fps modes

        // Set up interstream rules for left/right/z images
        for(auto ir : {RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
        {
            info.interstream_rules.push_back({RS_STREAM_DEPTH, ir, &stream_request::width, 0, 12});
            info.interstream_rules.push_back({RS_STREAM_DEPTH, ir, &stream_request::height, 0, 12});
            info.interstream_rules.push_back({RS_STREAM_DEPTH, ir, &stream_request::fps, 0, 0});
        }

        info.presets[RS_STREAM_INFRARED][RS_PRESET_BEST_QUALITY] = {true, 480, 360, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_BEST_QUALITY] = {true, 480, 360, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_LARGEST_IMAGE] = {true, 1920, 1080, RS_FORMAT_RGB8, 30};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_HIGHEST_FRAMERATE] = {true, 320, 240, RS_FORMAT_Y8,   90};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 320, 240, RS_FORMAT_Z16,  90};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        for(int i=0; i<RS_PRESET_COUNT; ++i) 
			info.presets[RS_STREAM_INFRARED2][i] = info.presets[RS_STREAM_INFRARED][i];

        for(int i = RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED; i <= RS_OPTION_R200_DISPARITY_SHIFT; ++i)
			info.option_supported[i] = true;

        r200::CameraCalibrationParameters c;
        r200::CameraHeaderInfo h;
        r200::read_camera_info(*device, c, h);
        
        if (c.metadata.versionNumber != 2)
            throw std::runtime_error("only supported calibration struct is version 2. got (" + std::to_string(c.metadata.versionNumber) + ").");

        std::vector<rs_intrinsics> intrinsics(NUM_INTRINSICS), rect_intrinsics(NUM_INTRINSICS);
        rect_intrinsics[LR_FULL] = intrinsics[LR_FULL] = MakeLeftRightIntrinsics(c.modesLR[0][0]);
        rect_intrinsics[LR_BIG ] = intrinsics[LR_BIG ] = MakeLeftRightIntrinsics(c.modesLR[0][1]);
        rect_intrinsics[LR_QRES] = intrinsics[LR_QRES] = MakeLeftRightIntrinsics(c.modesLR[0][2]);
        rect_intrinsics[Z_FULL ] = intrinsics[Z_FULL ] = MakeDepthIntrinsics(c.modesLR[0][0]);
        rect_intrinsics[Z_BIG  ] = intrinsics[Z_BIG  ] = MakeDepthIntrinsics(c.modesLR[0][1]);
        rect_intrinsics[Z_QRES ] = intrinsics[Z_QRES ] = MakeDepthIntrinsics(c.modesLR[0][2]);
        intrinsics[THIRD_HD  ] = MakeColorIntrinsics(c.intrinsicsThird[0],1);
        intrinsics[THIRD_VGA ] = MakeColorIntrinsics(c.intrinsicsThird[1],1);
        intrinsics[THIRD_QRES] = MakeColorIntrinsics(c.intrinsicsThird[1],2);
        rect_intrinsics[THIRD_HD  ] = MakeColorIntrinsics(c.modesThird[0][0][0],1);
        rect_intrinsics[THIRD_VGA ] = MakeColorIntrinsics(c.modesThird[0][1][0],1);
        rect_intrinsics[THIRD_QRES] = MakeColorIntrinsics(c.modesThird[0][1][0],2);

        // We select the depth/left infrared camera's viewpoint to be the origin
        info.stream_poses[RS_STREAM_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_INFRARED] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};

        // The right infrared camera is offset along the +x axis by the baseline (B)
        info.stream_poses[RS_STREAM_INFRARED2] = {{{1,0,0},{0,1,0},{0,0,1}}, {c.B[0] * 0.001f, 0, 0}}; // Sterling comment

		// The transformation between the depth camera and third camera is described by a translation vector (T), followed by rotation matrix (Rthird)
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) 
			info.stream_poses[RS_STREAM_COLOR].orientation(i,j) = c.Rthird[0][i*3+j];
        for(int i=0; i<3; ++i) 
			info.stream_poses[RS_STREAM_COLOR].position[i] = c.T[0][i] * 0.001f;

        // Our position is added AFTER orientation is applied, not before, so we must multiply Rthird * T to compute it
        info.stream_poses[RS_STREAM_COLOR].position = info.stream_poses[RS_STREAM_COLOR].orientation * info.stream_poses[RS_STREAM_COLOR].position;
        info.nominal_depth_scale = 0.001f;
        info.serial = std::to_string(h.serialNumber);
        info.firmware_version = r200::read_firmware_version(*device);

		// On LibUVC backends, the R200 should use four transfer buffers
        info.num_libuvc_transfer_buffers = 4;

        return std::make_shared<r200_camera>(device, info, intrinsics, rect_intrinsics);
    }

    bool r200_camera::is_disparity_mode_enabled() const
    {
        auto & depth = get_stream_interface(RS_STREAM_DEPTH);
        return depth.is_enabled() && depth.get_format() == RS_FORMAT_DISPARITY16;
    }

    void r200_camera::on_update_depth_units(int units)
    {
        if(is_disparity_mode_enabled()) return;
        config.depth_scale = (float)units / 1000000; // Convert from micrometers to meters
    }

    void r200_camera::on_update_disparity_multiplier(float multiplier)
    {
        if(!is_disparity_mode_enabled()) return;
        auto & depth = get_stream_interface(RS_STREAM_DEPTH);
        float baseline = get_stream_interface(RS_STREAM_INFRARED2).get_extrinsics_to(depth).translation[0];
        config.depth_scale = depth.get_intrinsics().fx * baseline * multiplier;
    }

    void r200_camera::on_before_start(const std::vector<subdevice_mode_selection> & selected_modes)
    {
        uint8_t streamIntent = 0;
        for(const auto & m : selected_modes)
        {
            switch(m.mode->subdevice)
            {
            case 0: streamIntent |= r200::STATUS_BIT_LR_STREAMING; break;
            case 2: streamIntent |= r200::STATUS_BIT_WEB_STREAMING; break;
            case 1: 
                streamIntent |= r200::STATUS_BIT_Z_STREAMING; 
                r200::disparity_mode dm;        
                r200::get_disparity_mode(get_device(), dm);
                switch(m.mode->streams[0].format)
                {
                default: throw std::logic_error("unsupported R200 depth format");
                case RS_FORMAT_Z16: 
                    dm.is_disparity_enabled = 0;
                    on_update_depth_units(get_xu_option(RS_OPTION_R200_DEPTH_UNITS));
                    break;
                case RS_FORMAT_DISPARITY16: 
                    dm.is_disparity_enabled = 1;
                    on_update_disparity_multiplier(dm.disparity_multiplier);
                    break;
                }
                r200::set_disparity_mode(get_device(), dm);
                break;
            }
        }
        r200::set_stream_intent(get_device(), streamIntent);
    }

    int r200_camera::convert_timestamp(int64_t timestamp) const
    { 
        int max_fps = 0;
        for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
        {
            if(get_stream_interface((rs_stream)i).is_enabled())
            {
                max_fps = std::max(max_fps, get_stream_interface((rs_stream)i).get_framerate());
            }
        }
        return static_cast<int>(timestamp * 1000 / max_fps);
    }

    int r200_camera::get_lr_framerate() const
    {
        for(auto s : {RS_STREAM_DEPTH, RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
        {
            auto & stream = get_stream_interface(s);
            if(stream.is_enabled()) return stream.get_framerate();
        }
        return 30;
    }

    void r200_camera::set_xu_option(rs_option option, int value)
    {
        if(is_capturing())
        {
            switch(option)
            {
            case RS_OPTION_R200_DEPTH_UNITS:
            case RS_OPTION_R200_DEPTH_CLAMP_MIN:
            case RS_OPTION_R200_DEPTH_CLAMP_MAX:
            case RS_OPTION_R200_DISPARITY_MULTIPLIER:
            case RS_OPTION_R200_DISPARITY_SHIFT:
                throw std::runtime_error("cannot set this option after rs_start_capture(...)");
            }
        }
        r200::disparity_mode dm;
        uint32_t u32[2];
        uint16_t u16[2];

        // todo - Range check value before write
        switch(option)
        {
        case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED:
            r200::set_lr_exposure_mode(get_device(), value);
            break;
        case RS_OPTION_R200_LR_GAIN:
            r200::set_lr_gain(get_device(), get_lr_framerate(), value); // TODO: May need to set this on start if framerate changes
            break;
        case RS_OPTION_R200_LR_EXPOSURE:
            r200::set_lr_exposure(get_device(), get_lr_framerate(), value); // TODO: May need to set this on start if framerate changes
            break;
        case RS_OPTION_R200_EMITTER_ENABLED:
            r200::set_emitter_state(get_device(), !!value);
            break;
        case RS_OPTION_R200_DEPTH_CONTROL_PRESET:
            r200::set_depth_params(get_device(), r200::depth_params::presets[value]);
            break;
        case RS_OPTION_R200_DEPTH_UNITS:
            r200::set_depth_units(get_device(), value);
            on_update_depth_units(value);
            break;
        case RS_OPTION_R200_DEPTH_CLAMP_MIN:
            r200::get_min_max_depth(get_device(), u16[0], u16[1]);
            r200::set_min_max_depth(get_device(), value, u16[1]);
            break;
        case RS_OPTION_R200_DEPTH_CLAMP_MAX:
            r200::get_min_max_depth(get_device(), u16[0], u16[1]);
            r200::set_min_max_depth(get_device(), u16[0], value);
            break;
        case RS_OPTION_R200_DISPARITY_MULTIPLIER:
            r200::get_disparity_mode(get_device(), dm);
            dm.disparity_multiplier = value;
            r200::set_disparity_mode(get_device(), dm);
            on_update_disparity_multiplier(value);
            break;
        case RS_OPTION_R200_DISPARITY_SHIFT:
            r200::set_disparity_shift(get_device(), value);
            break;
        }
    }

    void r200_camera::get_xu_range(rs_option option, int * min, int * max) const
    {
        int max_fps = 30;
        for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
        {
            if(get_stream_interface((rs_stream)i).is_enabled())
            {
                max_fps = std::max(max_fps, get_stream_interface((rs_stream)i).get_framerate());
            }
        }
        const struct { rs_option option; int min, max; } ranges[] = {
            {RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, 0, 1},
            {RS_OPTION_R200_LR_GAIN, 100, 1600},
            {RS_OPTION_R200_LR_EXPOSURE, 0, 10000/max_fps},
            {RS_OPTION_R200_EMITTER_ENABLED, 0, 1},
            {RS_OPTION_R200_DEPTH_CONTROL_PRESET, 0, 5},
            {RS_OPTION_R200_DEPTH_UNITS, 1, INT_MAX}, // What is the real range?
            {RS_OPTION_R200_DEPTH_CLAMP_MIN, 0, USHRT_MAX},
            {RS_OPTION_R200_DEPTH_CLAMP_MAX, 0, USHRT_MAX},
            {RS_OPTION_R200_DISPARITY_MULTIPLIER, 1, 1000},
            {RS_OPTION_R200_DISPARITY_SHIFT, 0, 0},
        };
        for(auto & r : ranges)
        {
            if(option != r.option) continue;
            if(min) *min = r.min;
            if(max) *max = r.max;
            return;
        }
        throw std::logic_error("range not specified");
    }

    int r200_camera::get_xu_option(rs_option option) const
    {
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
        case RS_OPTION_R200_EMITTER_ENABLED:          r200::get_emitter_state   (get_device(), is_capturing(), get_stream_interface(RS_STREAM_DEPTH).is_enabled(), b); value = b; break;
        case RS_OPTION_R200_DEPTH_UNITS:              r200::get_depth_units     (get_device(), u32[0]);         value = u32[0]; break;
        case RS_OPTION_R200_DEPTH_CLAMP_MIN:          r200::get_min_max_depth   (get_device(), u16[0], u16[1]); value = u16[0]; break;
        case RS_OPTION_R200_DEPTH_CLAMP_MAX:          r200::get_min_max_depth   (get_device(), u16[0], u16[1]); value = u16[1]; break;
        case RS_OPTION_R200_DISPARITY_MULTIPLIER:     r200::get_disparity_mode  (get_device(), dm);             value = static_cast<int>(dm.disparity_multiplier); break;
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
