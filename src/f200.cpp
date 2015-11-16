#include "f200.h"
#include "f200-private.h"
#include "image.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <limits>

namespace rsimpl
{
    #pragma pack(push, 1)
    struct inzi_pixel { uint16_t value; uint16_t empty; };
    #pragma pack(pop)

    void unpack_inzi_to_z16_and_y16(void * dest[], const void * frame, const subdevice_mode & mode)
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

    void unpack_inzi_to_z16_and_y8(void * dest[], const void * frame, const subdevice_mode & mode)
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
            intrin.ppx -= 1.0f/6;
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

    enum { COLOR_1080P, COLOR_720P, COLOR_540P, COLOR_480P, COLOR_VGA, COLOR_360P, COLOR_240P, COLOR_QVGA, COLOR_180P, DEPTH_VGA, DEPTH_HVGA, NUM_INTRINSICS };
    struct f200_mode { int w, h, intrin; std::vector<int> fps; };
    static const f200_mode f200_color_modes[] = {
        {1920, 1080, COLOR_1080P, {2,5,15,30}},
        {1280,  720, COLOR_720P,  {2,5,15,30}},
        { 960,  540, COLOR_540P,  {2,5,15,30,60}},
        { 848,  480, COLOR_480P,  {2,5,15,30,60}},
        { 640,  480, COLOR_VGA,   {2,5,15,30,60}},
        { 640,  360, COLOR_360P,  {2,5,15,30,60}},
        { 424,  240, COLOR_240P,  {2,5,15,30,60}},
        { 320,  240, COLOR_QVGA,  {2,5,15,30,60}},
        { 320,  180, COLOR_180P,  {2,5,15,30,60}}
    };
    static const f200_mode f200_depth_modes[] = {
        {640, 480, DEPTH_VGA,  {2,5,15,30,60}}, 
        {640, 240, DEPTH_HVGA, {2,5,15,30,60,110}}
    };
    static const f200_mode f200_ir_only_modes[] = {
        {640, 480, DEPTH_VGA,  {30,60,120,240,300}}, 
        {640, 240, DEPTH_HVGA, {30,60,120,240,300}}        
    };
    
    static static_device_info get_f200_info(const f200::CameraCalibrationParameters & c)
    {
        static_device_info info;
        info.name = {"Intel RealSense F200"};

        // Color modes on subdevice 0
        info.stream_subdevices[RS_STREAM_COLOR] = 0;
        for(auto & m : f200_color_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({0, m.w, m.h, &pf_yuy2, fps, {{RS_STREAM_COLOR, m.w, m.h, RS_FORMAT_YUYV, fps, m.intrin}}, &unpack_subrect, &decode_ivcam_frame_number});
            }
        }

        // Depth and IR modes on subdevice 1
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        for(auto & m : f200_ir_only_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.w, m.h, &pf_f200_invi, fps, {{RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y8,  fps, m.intrin}}, &unpack_subrect, &decode_ivcam_frame_number});
                info.subdevice_modes.push_back({1, m.w, m.h, &pf_f200_invi, fps, {{RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y16, fps, m.intrin}}, &unpack_y16_from_y8, &decode_ivcam_frame_number});    
            }
        }
        for(auto & m : f200_depth_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.w, m.h, &pf_invz, fps, {{RS_STREAM_DEPTH,    m.w, m.h, RS_FORMAT_Z16, fps, m.intrin}}, &unpack_subrect, &decode_ivcam_frame_number});       
                info.subdevice_modes.push_back({1, m.w, m.h, &pf_f200_inzi, fps, {{RS_STREAM_DEPTH,    m.w, m.h, RS_FORMAT_Z16, fps, m.intrin},
                                                                                  {RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y8,  fps, m.intrin}}, &unpack_z16_y8_from_inri, &decode_ivcam_frame_number});
                info.subdevice_modes.push_back({1, m.w, m.h, &pf_f200_inzi, fps, {{RS_STREAM_DEPTH,    m.w, m.h, RS_FORMAT_Z16, fps, m.intrin},
                                                                                  {RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y16, fps, m.intrin}}, &unpack_z16_y16_from_inri, &decode_ivcam_frame_number});
            }
        }

        info.presets[RS_STREAM_INFRARED][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_LARGEST_IMAGE] = {true, 1920, 1080, RS_FORMAT_RGB8, 60};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        for(int i = RS_OPTION_F200_LASER_POWER; i <= RS_OPTION_F200_CONFIDENCE_THRESHOLD; ++i)
            info.option_supported[i] = true;

        info.stream_poses[RS_STREAM_DEPTH] = info.stream_poses[RS_STREAM_INFRARED] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_COLOR] = {transpose((const float3x3 &)c.Rt), (const float3 &)c.Tt * 0.001f}; // convert mm to m
        info.depth_scale = (c.Rmax / 0xFFFF) * 0.001f; // convert mm to m
        info.num_libuvc_transfer_buffers = 1;

        return info;
    }

    static const f200_mode sr300_color_modes[] = {
        {1920, 1080, COLOR_1080P, {5,15,30}},
        {1280,  720, COLOR_720P,  {5,15,30,60}},
        { 960,  540, COLOR_540P,  {5,15,30,60}},
        { 848,  480, COLOR_480P,  {5,15,30,60}},
        { 640,  480, COLOR_VGA,   {5,15,30,60}},
        { 640,  360, COLOR_360P,  {5,15,30,60}},
        { 424,  240, COLOR_240P,  {5,15,30,60}},
        { 320,  240, COLOR_QVGA,  {5,15,30,60}},
        { 320,  180, COLOR_180P,  {5,15,30,60}}
    };
    static const f200_mode sr300_depth_modes[] = {
        {640, 480, DEPTH_VGA,  {5,15,30,60}}, 
        {640, 240, DEPTH_HVGA, {5,15,30,60,110}}
    };
    static const f200_mode sr300_ir_only_modes[] = {
        {640, 480, DEPTH_VGA,  {30,60,120,200}}      
    };    

    static static_device_info get_sr300_info(const f200::CameraCalibrationParameters & c)
    {
        static_device_info info;
        info.name = {"Intel RealSense SR300"};
        
        // Color modes on subdevice 0
        info.stream_subdevices[RS_STREAM_COLOR] = 0;
        for(auto & m : sr300_color_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({0, m.w, m.h, &pf_yuy2, fps, {{RS_STREAM_COLOR, m.w, m.h, RS_FORMAT_YUYV, fps, m.intrin}}, &unpack_subrect, &decode_ivcam_frame_number});
            }
        }

        // Depth and IR modes on subdevice 1
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        for(auto & m : sr300_ir_only_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.w, m.h, &pf_sr300_invi, fps, {{RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y8,  fps, m.intrin}}, &unpack_y8_from_y16_10, &decode_ivcam_frame_number});    
                info.subdevice_modes.push_back({1, m.w, m.h, &pf_sr300_invi, fps, {{RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y16, fps, m.intrin}}, &unpack_y16_from_y16_10, &decode_ivcam_frame_number});                
            }
        }
        for(auto & m : sr300_depth_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.w, m.h, &pf_invz, fps, {{RS_STREAM_DEPTH,    m.w, m.h, RS_FORMAT_Z16, fps, m.intrin}}, &unpack_subrect, &decode_ivcam_frame_number});       
                info.subdevice_modes.push_back({1, m.w, m.h, &pf_sr300_inzi, fps, {{RS_STREAM_DEPTH,    m.w, m.h, RS_FORMAT_Z16, fps, m.intrin},
                                                                                   {RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y8,  fps, m.intrin}}, &unpack_inzi_to_z16_and_y8, &decode_ivcam_frame_number});
                info.subdevice_modes.push_back({1, m.w, m.h, &pf_sr300_inzi, fps, {{RS_STREAM_DEPTH,    m.w, m.h, RS_FORMAT_Z16, fps, m.intrin},
                                                                                   {RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y16, fps, m.intrin}}, &unpack_inzi_to_z16_and_y16, &decode_ivcam_frame_number});
            }
        }

        for(int i=0; i<RS_PRESET_COUNT; ++i)
        {
            info.presets[RS_STREAM_COLOR   ][i] = {true, 640, 480, RS_FORMAT_RGB8, 60};
            info.presets[RS_STREAM_DEPTH   ][i] = {true, 640, 480, RS_FORMAT_Z16, 60};
            info.presets[RS_STREAM_INFRARED][i] = {true, 640, 480, RS_FORMAT_Y16, 60};
        }

        for(int i = RS_OPTION_F200_LASER_POWER; i <= RS_OPTION_F200_DYNAMIC_FPS; ++i) info.option_supported[i] = true;

        info.stream_poses[RS_STREAM_DEPTH] = info.stream_poses[RS_STREAM_INFRARED] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_COLOR] = {transpose((const float3x3 &)c.Rt), (const float3 &)c.Tt * 0.001f}; // convert mm to m
        info.depth_scale = (c.Rmax / 0xFFFF) * 0.001f; // convert mm to m
        info.num_libuvc_transfer_buffers = 1;

        return info;
    }

    std::vector<rs_intrinsics> compute_intrinsics(const f200::CameraCalibrationParameters & calibration)
    {
        std::vector<rs_intrinsics> intrinsics(NUM_INTRINSICS);
        for(auto & m : f200_color_modes) intrinsics[m.intrin] = MakeColorIntrinsics(calibration, m.w, m.h);
        for(auto & m : f200_depth_modes) intrinsics[m.intrin] = MakeDepthIntrinsics(calibration, m.w, m.h);
        return intrinsics;
    }

    f200_camera::f200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const f200::CameraCalibrationParameters & calib, const f200::IVCAMTemperatureData & temp, const f200::IVCAMThermalLoopParams & params) :
        rs_device(device, info), base_calibration(calib), base_temperature_data(temp), thermal_loop_params(params), last_temperature_delta(std::numeric_limits<float>::infinity())
    {
        set_intrinsics_thread_safe(compute_intrinsics(base_calibration));

        // If thermal control loop requested, start up thread to handle it
		if(thermal_loop_params.IRThermalLoopEnable)
        {
            runTemperatureThread = true;
            temperatureThread = std::thread(&f200_camera::temperature_control_loop, this);
        }
    }

    const uvc::guid IVCAM_DEPTH_XU = {0xA55751A1,0xF3C5,0x4A5E,{0x8D,0x5A,0x68,0x54,0xB8,0xFA,0x27,0x16}};
    const uvc::guid IVCAM_COLOR_XU = {0xB8EC416E,0xA3AC,0x4580,{0x8D,0x5C,0x0B,0xEE,0x15,0x97,0xE4,0x3D}};

    std::shared_ptr<rs_device> make_f200_device(std::shared_ptr<uvc::device> device)
    {
        init_controls(*device, 1, IVCAM_DEPTH_XU);
        std::timed_mutex mutex;
        f200::claim_ivcam_interface(*device);
        auto calib = f200::read_f200_calibration(*device, mutex);
        f200::enable_timestamp(*device, mutex, true, true);

        auto info = get_f200_info(std::get<0>(calib));
        f200::get_module_serial_string(*device, mutex, info.serial, 96);
        f200::get_firmware_version_string(*device, mutex, info.firmware_version);

        return std::make_shared<f200_camera>(device, info, std::get<0>(calib), std::get<1>(calib), std::get<2>(calib));
    }

    std::shared_ptr<rs_device> make_sr300_device(std::shared_ptr<uvc::device> device)
    {
        init_controls(*device, 1, IVCAM_DEPTH_XU);
        std::timed_mutex mutex;
        f200::claim_ivcam_interface(*device);
        auto calib = f200::read_sr300_calibration(*device, mutex);
        f200::enable_timestamp(*device, mutex, true, true);

        uvc::set_pu_control(*device, 0, rs_option::RS_OPTION_COLOR_BACKLIGHT_COMPENSATION, 0);
        uvc::set_pu_control(*device, 0, rs_option::RS_OPTION_COLOR_BRIGHTNESS, 0);
        uvc::set_pu_control(*device, 0, rs_option::RS_OPTION_COLOR_CONTRAST, 50);
        uvc::set_pu_control(*device, 0, rs_option::RS_OPTION_COLOR_GAMMA, 300);
        uvc::set_pu_control(*device, 0, rs_option::RS_OPTION_COLOR_HUE, 0);
        uvc::set_pu_control(*device, 0, rs_option::RS_OPTION_COLOR_SATURATION, 64);
        uvc::set_pu_control(*device, 0, rs_option::RS_OPTION_COLOR_SHARPNESS, 50);
        uvc::set_pu_control(*device, 0, rs_option::RS_OPTION_COLOR_GAIN, 64);
        //uvc::set_pu_control(*device, 0, rs_option::RS_OPTION_COLOR_WHITE_BALANCE, 4600); // auto
        //uvc::set_pu_control(*device, 0, rs_option::RS_OPTION_COLOR_EXPOSURE, -6); // auto

        auto info = get_sr300_info(std::get<0>(calib));

        f200::get_module_serial_string(*device, mutex, info.serial, 132);
        f200::get_firmware_version_string(*device, mutex, info.firmware_version);

        return std::make_shared<f200_camera>(device, info, std::get<0>(calib), std::get<1>(calib), std::get<2>(calib));
    }

    f200_camera::~f200_camera()
    {
        // Shut down thermal control loop thread
        runTemperatureThread = false;
        temperatureCv.notify_one();
        if (temperatureThread.joinable())
            temperatureThread.join();        
    }

    void f200_camera::on_before_start(const std::vector<subdevice_mode> & selected_modes)
    {

    }

    void f200_camera::temperature_control_loop()
    {
        const float FcxSlope = base_calibration.Kc[0][0] * thermal_loop_params.FcxSlopeA + thermal_loop_params.FcxSlopeB;
        const float UxSlope = base_calibration.Kc[0][2] * thermal_loop_params.UxSlopeA + base_calibration.Kc[0][0] * thermal_loop_params.UxSlopeB + thermal_loop_params.UxSlopeC;
        const float tempFromHFOV = (tan(thermal_loop_params.HFOVsensitivity*(float)M_PI/360)*(1 + base_calibration.Kc[0][0]*base_calibration.Kc[0][0]))/(FcxSlope * (1 + base_calibration.Kc[0][0] * tan(thermal_loop_params.HFOVsensitivity * (float)M_PI/360)));
        float TempThreshold = thermal_loop_params.TempThreshold; //celcius degrees, the temperatures delta that above should be fixed;
        if (TempThreshold <= 0) TempThreshold = tempFromHFOV;
        if (TempThreshold > tempFromHFOV) TempThreshold = tempFromHFOV;

        std::unique_lock<std::mutex> lock(temperatureMutex);
        while (runTemperatureThread) 
        {
            temperatureCv.wait_for(lock, std::chrono::seconds(10));

            // todo - this will throw if bad, but might periodically fail anyway. try/catch
            try
            {
                float IRTemp = (float)f200::read_ir_temp(get_device(), usbMutex);
                float LiguriaTemp = f200::read_mems_temp(get_device(), usbMutex);

                double IrBaseTemperature = base_temperature_data.IRTemp; //should be taken from the parameters
                double liguriaBaseTemperature = base_temperature_data.LiguriaTemp; //should be taken from the parameters

                // calculate deltas from the calibration and last fix
                double IrTempDelta = IRTemp - IrBaseTemperature;
                double liguriaTempDelta = LiguriaTemp - liguriaBaseTemperature;
                double weightedTempDelta = liguriaTempDelta * thermal_loop_params.LiguriaTempWeight + IrTempDelta * thermal_loop_params.IrTempWeight;
                double tempDetaFromLastFix = abs(weightedTempDelta - last_temperature_delta);

                // read intrinsic from the calibration working point
                double Kc11 = base_calibration.Kc[0][0];
                double Kc13 = base_calibration.Kc[0][2];

                // Apply model
                if (tempDetaFromLastFix >= TempThreshold)
                {
                    // if we are during a transition, fix for after the transition
                    double tempDeltaToUse = weightedTempDelta;
                    if (tempDeltaToUse > 0 && tempDeltaToUse < thermal_loop_params.TransitionTemp)
                    {
                        tempDeltaToUse = thermal_loop_params.TransitionTemp;
                    }

                    // calculate fixed values
                    double fixed_Kc11 = Kc11 + (FcxSlope * tempDeltaToUse) + thermal_loop_params.FcxOffset;
                    double fixed_Kc13 = Kc13 + (UxSlope * tempDeltaToUse) + thermal_loop_params.UxOffset;

                    // write back to intrinsic hfov and vfov
                    auto compensated_calibration = base_calibration;
                    compensated_calibration.Kc[0][0] = (float) fixed_Kc11;
                    compensated_calibration.Kc[1][1] = base_calibration.Kc[1][1] * (float)(fixed_Kc11/Kc11);
                    compensated_calibration.Kc[0][2] = (float) fixed_Kc13;

                    // todo - Pass the current resolution into update_asic_coefficients
                    DEBUG_OUT("updating asic with new temperature calibration coefficients");
                    update_asic_coefficients(get_device(), usbMutex, compensated_calibration);
                    set_intrinsics_thread_safe(compute_intrinsics(compensated_calibration));
                    last_temperature_delta = (float)weightedTempDelta;
                }
            }
            catch(const std::exception & e) { DEBUG_ERR("TemperatureControlLoop: " << e.what()); }
        }
    }

    void f200_camera::set_xu_option(rs_option option, int value)
    {
        // todo - Range check value before write
        auto val = static_cast<uint8_t>(value);
        switch(option)
        {
        case RS_OPTION_F200_LASER_POWER:          f200::set_laser_power(get_device(), val); break;
        case RS_OPTION_F200_ACCURACY:             f200::set_accuracy(get_device(), val); break;
        case RS_OPTION_F200_MOTION_RANGE:         f200::set_motion_range(get_device(), val); break;
        case RS_OPTION_F200_FILTER_OPTION:        f200::set_filter_option(get_device(), val); break;
        case RS_OPTION_F200_CONFIDENCE_THRESHOLD: f200::set_confidence_threshold(get_device(), val); break;
        case RS_OPTION_F200_DYNAMIC_FPS:          f200::set_dynamic_fps(get_device(), val); break; // IVCAM 1.5 Only
        }
    }

    int f200_camera::get_xu_option(rs_option option) const
    {
        uint8_t value = 0;
        switch(option)
        {
        case RS_OPTION_F200_LASER_POWER:          f200::get_laser_power(get_device(), value); break;
        case RS_OPTION_F200_ACCURACY:             f200::get_accuracy(get_device(), value); break;
        case RS_OPTION_F200_MOTION_RANGE:         f200::get_motion_range(get_device(), value); break;
        case RS_OPTION_F200_FILTER_OPTION:        f200::get_filter_option(get_device(), value); break;
        case RS_OPTION_F200_CONFIDENCE_THRESHOLD: f200::get_confidence_threshold(get_device(), value); break;
        case RS_OPTION_F200_DYNAMIC_FPS:          f200::get_dynamic_fps(get_device(), value); break; // IVCAM 1.5 Only
        }
        return value;
    }

} // namespace rsimpl::f200

