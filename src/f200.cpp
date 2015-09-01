#include "f200.h"
#include "f200-private.h"

#include <iostream>

namespace rsimpl
{
    #pragma pack(push, 1)
    struct inzi_pixel { uint16_t depth; uint8_t ir; };
    #pragma pack(pop)

    void unpack_inzi_to_z16_and_y8(void * dest[], const subdevice_mode & mode, const void * frame)
    {
        auto in = reinterpret_cast<const inzi_pixel *>(frame);
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

    enum { COLOR_VGA, COLOR_HD, DEPTH_VGA, DEPTH_QVGA, NUM_INTRINSICS };
    static static_camera_info get_f200_info()
    {
        static_camera_info info;

        // Color modes on subdevice 0
        info.stream_subdevices[RS_STREAM_COLOR] = 0;
        info.subdevice_modes.push_back({0,  640,  480, uvc::frame_format::YUYV, 60, {{RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60, COLOR_VGA}}, &unpack_yuyv_to_rgb});
        info.subdevice_modes.push_back({0, 1920, 1080, uvc::frame_format::YUYV, 60, {{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_RGB8, 60, COLOR_HD}}, &unpack_yuyv_to_rgb});

        // Depth and IR modes on subdevice 1
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        info.subdevice_modes.push_back({1, 640, 480, uvc::frame_format::INVR, 60, {{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60, DEPTH_VGA}}, &unpack_strided_image});
        info.subdevice_modes.push_back({1, 640, 240, uvc::frame_format::INVR, 60, {{RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60, DEPTH_QVGA}}, &unpack_strided_image});

        info.subdevice_modes.push_back({1, 640, 480, uvc::frame_format::INVI, 60, {{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60, DEPTH_VGA}}, &unpack_strided_image});
        info.subdevice_modes.push_back({1, 640, 240, uvc::frame_format::INVI, 60, {{RS_STREAM_INFRARED, 320, 240, RS_FORMAT_Y8, 60, DEPTH_QVGA}}, &unpack_strided_image});

        info.subdevice_modes.push_back({1, 640, 480, uvc::frame_format::INRI, 60, {{RS_STREAM_DEPTH,    640, 480, RS_FORMAT_Z16, 60, DEPTH_VGA},
                                                                                   {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8,  60, DEPTH_VGA}}, &unpack_inzi_to_z16_and_y8});
        info.subdevice_modes.push_back({1, 640, 240, uvc::frame_format::INRI, 60, {{RS_STREAM_DEPTH,    320, 240, RS_FORMAT_Z16, 60, DEPTH_QVGA},
                                                                                   {RS_STREAM_INFRARED, 320, 240, RS_FORMAT_Y8,  60, DEPTH_QVGA}}, &unpack_inzi_to_z16_and_y8});

        info.presets[RS_STREAM_INFRARED][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_LARGEST_IMAGE] = {true, 1920, 1080, RS_FORMAT_RGB8, 60};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        for(int i = RS_OPTION_F200_LASER_POWER; i <= RS_OPTION_F200_DYNAMIC_FPS; ++i) info.option_supported[i] = true;
        return info;
    }

    f200_camera::f200_camera(uvc::device device) : rs_camera(device, get_f200_info())
    {

    }

    f200_camera::~f200_camera()
    {
        
    }

    static rs_intrinsics MakeDepthIntrinsics(const f200::CameraCalibrationParameters & c, int w, int h)
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

    static rs_intrinsics MakeColorIntrinsics(const f200::CameraCalibrationParameters & c, int w, int h)
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

    calibration_info f200_camera::retrieve_calibration()
    {
        if(!hardware_io) hardware_io.reset(new f200::IVCAMHardwareIO(*first_handle));
        const f200::CameraCalibrationParameters & calib = hardware_io->GetParameters();

        calibration_info c;
        c.intrinsics.resize(NUM_INTRINSICS);
        c.intrinsics[COLOR_VGA] = MakeColorIntrinsics(calib, 640, 480);
        c.intrinsics[COLOR_HD] = MakeColorIntrinsics(calib, 1920, 1080);
        c.intrinsics[DEPTH_VGA] = MakeDepthIntrinsics(calib, 640, 480);
        c.intrinsics[DEPTH_QVGA] = MakeDepthIntrinsics(calib, 320, 240);
        c.stream_poses[RS_STREAM_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        c.stream_poses[RS_STREAM_COLOR] = {transpose((const float3x3 &)calib.Rt), (const float3 &)calib.Tt * 0.001f}; // convert mm to m
        c.depth_scale = (calib.Rmax / 0xFFFF) * 0.001f; // convert mm to m
        return c;
    }
    
    // N.B. f200 xu_read and xu_write hard code the xu interface to the depth suvdevice. There is only a
    // single *potentially* useful XU on the color device, so let's ignore it for now.
    bool xu_read(uvc::device_handle & device, uint64_t xu_ctrl, void * buffer, uint32_t length)
    {
        auto status = device.get_ctrl(6, xu_ctrl, buffer, length);
        if (status < 0)
        {
            std::cerr << "xu read - uvc_get_ctrl - " << usb::error_name(status) << std::endl;
            return false;
        }
        return true;
    }
    
    bool xu_write(uvc::device_handle & device, uint64_t xu_ctrl, void * buffer, uint32_t length)
    {
        auto status = device.set_ctrl(6, xu_ctrl, buffer, length);
        if (status < 0)
        {
            std::cerr << "xu write - uvc_get_ctrl - " << usb::error_name(status) << std::endl;
            return false;
        }
        return true;
    }
    
    bool get_laser_power(uvc::device_handle & device, uint8_t & laser_power)
    {
        return xu_read(device, IVCAM_DEPTH_LASER_POWER, &laser_power, sizeof(laser_power));
    }
    
    bool set_laser_power(uvc::device_handle & device, uint8_t laser_power)
    {
        return xu_write(device, IVCAM_DEPTH_LASER_POWER, &laser_power, sizeof(laser_power));
    }
    
    bool get_accuracy(uvc::device_handle & device, uint8_t & accuracy)
    {
        return xu_read(device, IVCAM_DEPTH_ACCURACY, &accuracy, sizeof(accuracy));
    }
    
    bool set_accuracy(uvc::device_handle & device, uint8_t accuracy)
    {
        return xu_write(device, IVCAM_DEPTH_ACCURACY, &accuracy, sizeof(accuracy));
    }
    
    bool get_motion_range(uvc::device_handle & device, uint8_t & motion_range)
    {
        return xu_read(device, IVCAM_DEPTH_MOTION_RANGE, &motion_range, sizeof(motion_range));
    }
    
    bool set_motion_range(uvc::device_handle & device, uint8_t motion_range)
    {
        return xu_write(device, IVCAM_DEPTH_MOTION_RANGE, &motion_range, sizeof(motion_range));
    }
    
    bool get_filter_option(uvc::device_handle & device, uint8_t & filter_option)
    {
        return xu_read(device, IVCAM_DEPTH_FILTER_OPTION, &filter_option, sizeof(filter_option));
    }
    
    bool set_filter_option(uvc::device_handle & device, uint8_t filter_option)
    {
        return xu_write(device, IVCAM_DEPTH_FILTER_OPTION, &filter_option, sizeof(filter_option));
    }
    
    bool get_confidence_threshold(uvc::device_handle & device, uint8_t & conf_thresh)
    {
        return xu_read(device, IVCAM_DEPTH_CONFIDENCE_THRESH, &conf_thresh, sizeof(conf_thresh));
    }
    
    bool set_confidence_threshold(uvc::device_handle & device, uint8_t conf_thresh)
    {
        return xu_write(device, IVCAM_DEPTH_CONFIDENCE_THRESH, &conf_thresh, sizeof(conf_thresh));
    }
    
    bool get_dynamic_fps(uvc::device_handle & device, uint8_t & dynamic_fps)
    {
        return xu_read(device, IVCAM_DEPTH_DYNAMIC_FPS, &dynamic_fps, sizeof(dynamic_fps));
    }
    
    bool set_dynamic_fps(uvc::device_handle & device, uint8_t dynamic_fps)
    {
        return xu_write(device, IVCAM_DEPTH_DYNAMIC_FPS, &dynamic_fps, sizeof(dynamic_fps));
    }

    void f200_camera::set_option(rs_option option, int value)
    {
        // TODO: Range check value before write
        auto val = static_cast<uint8_t>(value);
        bool result = false;
        switch(option)
        {
        case RS_OPTION_F200_LASER_POWER:          result = set_laser_power(*first_handle, val); break;
        case RS_OPTION_F200_ACCURACY:             result = set_accuracy(*first_handle, val); break;
        case RS_OPTION_F200_MOTION_RANGE:         result = set_motion_range(*first_handle, val); break;
        case RS_OPTION_F200_FILTER_OPTION:        result = set_filter_option(*first_handle, val); break;
        case RS_OPTION_F200_CONFIDENCE_THRESHOLD: result = set_confidence_threshold(*first_handle, val); break;
        //case RS_OPTION_F200_DYNAMIC_FPS:          result = set_dynamic_fps(*first_handle, val); break; // IVCAM 1.5 Only
        }
        if(!result) throw std::runtime_error("failed to write option to device");
    }

    int f200_camera::get_option(rs_option option)
    {
        uint8_t value = 0;
        bool result = false;
        switch(option)
        {
        case RS_OPTION_F200_LASER_POWER:          result = get_laser_power(*first_handle, value); break;
        case RS_OPTION_F200_ACCURACY:             result = get_accuracy(*first_handle, value); break;
        case RS_OPTION_F200_MOTION_RANGE:         result = get_motion_range(*first_handle, value); break;
        case RS_OPTION_F200_FILTER_OPTION:        result = get_filter_option(*first_handle, value); break;
        case RS_OPTION_F200_CONFIDENCE_THRESHOLD: result = get_confidence_threshold(*first_handle, value); break;
        //case RS_OPTION_F200_DYNAMIC_FPS:          result = get_dynamic_fps(*first_handle, value); break; // IVCAM 1.5 Only
        }
        if(!result) throw std::runtime_error("failed to read option from device");
        return value;
    }

} // namespace rsimpl::f200

