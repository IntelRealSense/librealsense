#include "f200.h"
#include "f200-private.h"

#include <iostream>

namespace rsimpl
{
    #pragma pack(push, 1)
    struct inri_pixel { uint16_t depth; uint8_t ir; };
    struct inzi_pixel { uint16_t value; uint16_t empty; };
    #pragma pack(pop)

    void unpack_inri_to_z16_and_y8(void * dest[], const subdevice_mode & mode, const void * frame)
    {
        auto in = reinterpret_cast<const inri_pixel *>(frame);
        auto out_depth = reinterpret_cast<uint16_t *>(dest[0]);
        auto out_ir = reinterpret_cast<uint8_t *>(dest[1]);
        for(int y=0; y<mode.height; ++y)
        {
            for(int x=0; x<mode.streams[0].width; ++x)
            {
                *out_depth++ = in->depth;
                *out_ir++ = in->ir;
                ++in;
            }
            in += (mode.width - mode.streams[0].width);
        }
    }

    void unpack_inzi_to_z16_and_y16(void * dest[], const subdevice_mode & mode, const void * frame)
    {
        auto in = reinterpret_cast<const inzi_pixel *>(frame);
        auto out_depth = reinterpret_cast<uint16_t *>(dest[0]);
        auto out_ir = reinterpret_cast<uint16_t *>(dest[1]);
        for(int y=0; y<mode.height; ++y)
        {
            for(int x=0; x<mode.streams[0].width; x+=2)
            {
                *out_ir++ = in->value << 6;
                *out_ir++ = in->value << 6;
                ++in;
            }
            //in += (mode.width - mode.streams[0].width);
        }
        for(int y=0; y<mode.height; ++y)
        {
            for(int x=0; x<mode.streams[0].width; x+=2)
            {
                *out_depth++ = in->value;
                *out_depth++ = in->value;
                ++in;
            }
            //in += (mode.width - mode.streams[0].width);
        }
    }

    void unpack_inzi_to_z16_and_y8(void * dest[], const subdevice_mode & mode, const void * frame)
    {
        auto in = reinterpret_cast<const inzi_pixel *>(frame);
        auto out_depth = reinterpret_cast<uint16_t *>(dest[0]);
        auto out_ir = reinterpret_cast<uint8_t *>(dest[1]);
        for(int y=0; y<mode.height; ++y)
        {
            for(int x=0; x<mode.streams[0].width; x+=2)
            {
                *out_ir++ = in->value >> 2;
                *out_ir++ = in->value >> 2;
                ++in;
            }
            //in += (mode.width - mode.streams[0].width);
        }
        for(int y=0; y<mode.height; ++y)
        {
            for(int x=0; x<mode.streams[0].width; x+=2)
            {
                *out_depth++ = in->value;
                *out_depth++ = in->value;
                ++in;
            }
            //in += (mode.width - mode.streams[0].width);
        }
    }

    static rs_intrinsics MakeDepthIntrinsics(const f200::CameraCalibrationParameters & c, int w, int h)
    {
        return {w, h, (c.Kc[0][2]*0.5f + 0.5f) * w, (c.Kc[1][2]*0.5f + 0.5f) * h, c.Kc[0][0]*0.5f * w, c.Kc[1][1]*0.5f * h,
            RS_DISTORTION_INVERSE_BROWN_CONRADY, {c.Invdistc[0], c.Invdistc[1], c.Invdistc[2], c.Invdistc[3], c.Invdistc[4]}};
    }

    static rs_intrinsics MakeColorIntrinsics(const f200::CameraCalibrationParameters & c, int w, int h)
    {
        rs_intrinsics intrin = {w, h, c.Kt[0][2]*0.5f + 0.5f, c.Kt[1][2]*0.5f + 0.5f, c.Kt[0][0]*0.5f, c.Kt[1][1]*0.5f, RS_DISTORTION_NONE};
        if(w*3 == h*4) // If using a 4:3 aspect ratio, adjust intrinsics (defaults to 16:9)
        {
            intrin.fx *= 4.0f/3;
            intrin.ppx *= 4.0f/3;
            intrin.ppy -= 1.0f/6;
        }
        intrin.fx *= w;
        intrin.fy *= h;
        intrin.ppx *= w;
        intrin.ppy *= h;
        return intrin;
    }

    int decode_ivcam_frame_number(const subdevice_mode & mode, const void * frame)
    {
        return *reinterpret_cast<const int *>(frame);
    }

    enum { COLOR_VGA, COLOR_HD, DEPTH_VGA, DEPTH_QVGA, NUM_INTRINSICS };
    static static_device_info get_f200_info(f200::IVCAMHardwareIO & io)
    {
        static_device_info info;
        info.name = {"Intel RealSense F200"};

        // Color modes on subdevice 0
        info.stream_subdevices[RS_STREAM_COLOR] = 0;
        info.subdevice_modes.push_back({0,  640,  480, 'YUY2', 60, {{RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60, COLOR_VGA}}, &unpack_strided_image, &decode_ivcam_frame_number});
        info.subdevice_modes.push_back({0, 1920, 1080, 'YUY2', 60, {{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 60, COLOR_HD}}, &unpack_strided_image, &decode_ivcam_frame_number});

        // Depth and IR modes on subdevice 1
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        info.subdevice_modes.push_back({1, 640, 480, 'INVR', 60, {{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60, DEPTH_VGA}}, &unpack_strided_image, &decode_ivcam_frame_number});
        info.subdevice_modes.push_back({1, 640, 240, 'INVR', 60, {{RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60, DEPTH_QVGA}}, &unpack_strided_image, &decode_ivcam_frame_number});

        info.subdevice_modes.push_back({1, 640, 480, 'INVI', 60, {{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60, DEPTH_VGA}}, &unpack_strided_image, &decode_ivcam_frame_number});
        info.subdevice_modes.push_back({1, 640, 240, 'INVI', 60, {{RS_STREAM_INFRARED, 320, 240, RS_FORMAT_Y8, 60, DEPTH_QVGA}}, &unpack_strided_image, &decode_ivcam_frame_number});

        info.subdevice_modes.push_back({1, 640, 480, 'INRI', 60, {{RS_STREAM_DEPTH,    640, 480, RS_FORMAT_Z16, 60, DEPTH_VGA},
                                                                  {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8,  60, DEPTH_VGA}}, &unpack_inri_to_z16_and_y8, &decode_ivcam_frame_number});
        info.subdevice_modes.push_back({1, 640, 240, 'INRI', 60, {{RS_STREAM_DEPTH,    320, 240, RS_FORMAT_Z16, 60, DEPTH_QVGA},
                                                                  {RS_STREAM_INFRARED, 320, 240, RS_FORMAT_Y8,  60, DEPTH_QVGA}}, &unpack_inri_to_z16_and_y8, &decode_ivcam_frame_number});

        info.presets[RS_STREAM_INFRARED][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_LARGEST_IMAGE] = {true, 1920, 1080, RS_FORMAT_RGB8, 60};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        for(int i = RS_OPTION_F200_LASER_POWER; i <= RS_OPTION_F200_CONFIDENCE_THRESHOLD; ++i) info.option_supported[i] = true;

        const f200::CameraCalibrationParameters & c = io.GetParameters();
        info.intrinsics.resize(NUM_INTRINSICS);
        info.intrinsics[COLOR_VGA] = MakeColorIntrinsics(c, 640, 480);
        info.intrinsics[COLOR_HD] = MakeColorIntrinsics(c, 1920, 1080);
        info.intrinsics[DEPTH_VGA] = MakeDepthIntrinsics(c, 640, 480);
        info.intrinsics[DEPTH_QVGA] = MakeDepthIntrinsics(c, 320, 240);
        info.stream_poses[RS_STREAM_DEPTH] = info.stream_poses[RS_STREAM_INFRARED] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_COLOR] = {transpose((const float3x3 &)c.Rt), (const float3 &)c.Tt * 0.001f}; // convert mm to m
        info.depth_scale = (c.Rmax / 0xFFFF) * 0.001f; // convert mm to m

        return info;
    }

    static static_device_info get_sr300_info(f200::IVCAMHardwareIO & io)
    {
        static_device_info info;
        info.name = {"Intel RealSense SR300"};
		
		info.stream_subdevices[RS_STREAM_COLOR] = 0;
		info.subdevice_modes.push_back({0, 640, 480, 'YUY2', 60, {{RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60, COLOR_VGA}}, &unpack_strided_image, &decode_ivcam_frame_number});

		info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        info.subdevice_modes.push_back({1, 640, 480, 'INVZ', 60, {{RS_STREAM_DEPTH,    640, 480, RS_FORMAT_Z16, 60, DEPTH_VGA}}, &unpack_strided_image, &decode_ivcam_frame_number});
        info.subdevice_modes.push_back({1, 640, 480, 'INZI', 60, {{RS_STREAM_DEPTH,    640, 480, RS_FORMAT_Z16, 60, DEPTH_VGA},
                                                                  {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60, DEPTH_VGA}}, &unpack_inzi_to_z16_and_y16, &decode_ivcam_frame_number});
        info.subdevice_modes.push_back({1, 640, 480, 'INZI', 60, {{RS_STREAM_DEPTH,    640, 480, RS_FORMAT_Z16, 60, DEPTH_VGA},
                                                                  {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8,  60, DEPTH_VGA}}, &unpack_inzi_to_z16_and_y8, &decode_ivcam_frame_number});

		for(int i=0; i<RS_PRESET_COUNT; ++i)
		{
			info.presets[RS_STREAM_COLOR   ][i] = {true, 640, 480, RS_FORMAT_RGB8, 60};
			info.presets[RS_STREAM_DEPTH   ][i] = {true, 640, 480, RS_FORMAT_Z16, 60};
            info.presets[RS_STREAM_INFRARED][i] = {true, 640, 480, RS_FORMAT_Y16, 60};
		}

		const f200::CameraCalibrationParameters & c = io.GetParameters();
        info.intrinsics.resize(NUM_INTRINSICS);
        info.intrinsics[COLOR_VGA] = MakeColorIntrinsics(c, 640, 480);
        info.intrinsics[DEPTH_VGA] = MakeDepthIntrinsics(c, 640, 480);
        info.stream_poses[RS_STREAM_DEPTH] = info.stream_poses[RS_STREAM_INFRARED] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_COLOR] = {transpose((const float3x3 &)c.Rt), (const float3 &)c.Tt * 0.001f}; // convert mm to m
        info.depth_scale = (c.Rmax / 0xFFFF) * 0.001f; // convert mm to m*/
        return info;
    }

    f200_camera::f200_camera(uvc::device device, bool sr300) : rs_device(device)
    {
		hardware_io.reset(new f200::IVCAMHardwareIO(device, sr300));
        if(sr300) device_info = add_standard_unpackers(get_sr300_info(*hardware_io));
		else device_info = add_standard_unpackers(get_f200_info(*hardware_io));
    }

    f200_camera::~f200_camera()
    {
        
    }
    
    // N.B. f200 xu_read and xu_write hard code the xu interface to the depth suvdevice. There is only a
    // single *potentially* useful XU on the color device, so let's ignore it for now.
    void xu_read(const uvc::device & device, uint64_t xu_ctrl, void * buffer, uint32_t length)
    {
        device.get_control(6, xu_ctrl, buffer, length);
    }
    
    void xu_write(uvc::device & device, uint64_t xu_ctrl, void * buffer, uint32_t length)
    {
        device.set_control(6, xu_ctrl, buffer, length);
    }
    
    void get_laser_power(const uvc::device & device, uint8_t & laser_power)
    {
        xu_read(device, IVCAM_DEPTH_LASER_POWER, &laser_power, sizeof(laser_power));
    }
    
    void set_laser_power(uvc::device & device, uint8_t laser_power)
    {
        xu_write(device, IVCAM_DEPTH_LASER_POWER, &laser_power, sizeof(laser_power));
    }
    
    void get_accuracy(const uvc::device & device, uint8_t & accuracy)
    {
        xu_read(device, IVCAM_DEPTH_ACCURACY, &accuracy, sizeof(accuracy));
    }
    
    void set_accuracy(uvc::device & device, uint8_t accuracy)
    {
        xu_write(device, IVCAM_DEPTH_ACCURACY, &accuracy, sizeof(accuracy));
    }
    
    void get_motion_range(const uvc::device & device, uint8_t & motion_range)
    {
        xu_read(device, IVCAM_DEPTH_MOTION_RANGE, &motion_range, sizeof(motion_range));
    }
    
    void set_motion_range(uvc::device & device, uint8_t motion_range)
    {
        xu_write(device, IVCAM_DEPTH_MOTION_RANGE, &motion_range, sizeof(motion_range));
    }
    
    void get_filter_option(const uvc::device & device, uint8_t & filter_option)
    {
        xu_read(device, IVCAM_DEPTH_FILTER_OPTION, &filter_option, sizeof(filter_option));
    }
    
    void set_filter_option(uvc::device & device, uint8_t filter_option)
    {
        xu_write(device, IVCAM_DEPTH_FILTER_OPTION, &filter_option, sizeof(filter_option));
    }
    
    void get_confidence_threshold(const uvc::device & device, uint8_t & conf_thresh)
    {
        xu_read(device, IVCAM_DEPTH_CONFIDENCE_THRESH, &conf_thresh, sizeof(conf_thresh));
    }
    
    void set_confidence_threshold(uvc::device & device, uint8_t conf_thresh)
    {
        xu_write(device, IVCAM_DEPTH_CONFIDENCE_THRESH, &conf_thresh, sizeof(conf_thresh));
    }
    
    void get_dynamic_fps(const uvc::device & device, uint8_t & dynamic_fps)
    {
        return xu_read(device, IVCAM_DEPTH_DYNAMIC_FPS, &dynamic_fps, sizeof(dynamic_fps));
    }
    
    void set_dynamic_fps(uvc::device & device, uint8_t dynamic_fps)
    {
        return xu_write(device, IVCAM_DEPTH_DYNAMIC_FPS, &dynamic_fps, sizeof(dynamic_fps));
    }

    void f200_camera::set_option(rs_option option, int value)
    {
        // TODO: Range check value before write
        auto val = static_cast<uint8_t>(value);
        switch(option)
        {
        case RS_OPTION_F200_LASER_POWER:          set_laser_power(device, val); break;
        case RS_OPTION_F200_ACCURACY:             set_accuracy(device, val); break;
        case RS_OPTION_F200_MOTION_RANGE:         set_motion_range(device, val); break;
        case RS_OPTION_F200_FILTER_OPTION:        set_filter_option(device, val); break;
        case RS_OPTION_F200_CONFIDENCE_THRESHOLD: set_confidence_threshold(device, val); break;
        //case RS_OPTION_F200_DYNAMIC_FPS:        set_dynamic_fps(device, val); break; // IVCAM 1.5 Only
        }
    }

    int f200_camera::get_option(rs_option option) const
    {
        uint8_t value = 0;
        switch(option)
        {
        case RS_OPTION_F200_LASER_POWER:          get_laser_power(device, value); break;
        case RS_OPTION_F200_ACCURACY:             get_accuracy(device, value); break;
        case RS_OPTION_F200_MOTION_RANGE:         get_motion_range(device, value); break;
        case RS_OPTION_F200_FILTER_OPTION:        get_filter_option(device, value); break;
        case RS_OPTION_F200_CONFIDENCE_THRESHOLD: get_confidence_threshold(device, value); break;
        //case RS_OPTION_F200_DYNAMIC_FPS:        get_dynamic_fps(device, value); break; // IVCAM 1.5 Only
        }
        return value;
    }

} // namespace rsimpl::f200

