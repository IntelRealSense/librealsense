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

    enum { COLOR_VGA, COLOR_HD, DEPTH_VGA, DEPTH_QVGA, NUM_INTRINSICS };
    static static_device_info get_f200_info(const f200::CameraCalibrationParameters & c)
    {
        struct mode { int w, h, intrin; };
        static const mode color_modes[] = {{640, 480, COLOR_VGA}, {1920, 1080, COLOR_HD}};
        static const mode depth_modes[] = {{640, 480, DEPTH_VGA}, {320, 240, DEPTH_QVGA}};        

        static_device_info info;
        info.name = {"Intel RealSense F200"};

        // Color modes on subdevice 0
        info.stream_subdevices[RS_STREAM_COLOR] = 0;
        for(auto & m : color_modes) info.subdevice_modes.push_back({0, m.w, m.h, 'YUY2', 60, {{RS_STREAM_COLOR, m.w, m.h, RS_FORMAT_YUYV, 60, m.intrin}}, &unpack_subrect, &decode_ivcam_frame_number});

        // Depth and IR modes on subdevice 1
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;        
        for(auto & m : depth_modes)
        {
            info.subdevice_modes.push_back({1, 640, m.h, 'INVR', 60, {{RS_STREAM_DEPTH,    m.w, m.h, RS_FORMAT_Z16, 60, m.intrin}}, &unpack_subrect, &decode_ivcam_frame_number});
            info.subdevice_modes.push_back({1, 640, m.h, 'INVI', 60, {{RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y8,  60, m.intrin}}, &unpack_subrect, &decode_ivcam_frame_number});            
            info.subdevice_modes.push_back({1, 640, m.h, 'INVI', 60, {{RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y16, 60, m.intrin}}, &unpack_y16_from_y8, &decode_ivcam_frame_number});            
            info.subdevice_modes.push_back({1, 640, m.h, 'INRI', 60, {{RS_STREAM_DEPTH,    m.w, m.h, RS_FORMAT_Z16, 60, m.intrin},
                                                                      {RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y8,  60, m.intrin}}, &unpack_z16_y8_from_inri, &decode_ivcam_frame_number});;
            info.subdevice_modes.push_back({1, 640, m.h, 'INRI', 60, {{RS_STREAM_DEPTH,    m.w, m.h, RS_FORMAT_Z16, 60, m.intrin},
                                                                      {RS_STREAM_INFRARED, m.w, m.h, RS_FORMAT_Y16, 60, m.intrin}}, &unpack_z16_y16_from_inri, &decode_ivcam_frame_number});;
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

        for(int i = RS_OPTION_F200_LASER_POWER; i <= RS_OPTION_F200_CONFIDENCE_THRESHOLD; ++i) info.option_supported[i] = true;

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

    static static_device_info get_sr300_info(const f200::CameraCalibrationParameters & c)
    {
        static_device_info info;
        info.name = {"Intel RealSense SR300"};
        
        info.stream_subdevices[RS_STREAM_COLOR] = 0;
        info.subdevice_modes.push_back({0, 640, 480, 'YUY2', 60, {{RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60, COLOR_VGA}}, &unpack_subrect, &decode_ivcam_frame_number});

        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        info.subdevice_modes.push_back({1, 640, 480, 'INVZ', 60, {{RS_STREAM_DEPTH,    640, 480, RS_FORMAT_Z16, 60, DEPTH_VGA}}, &unpack_subrect, &decode_ivcam_frame_number});
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

        for(int i = RS_OPTION_F200_LASER_POWER; i <= RS_OPTION_F200_DYNAMIC_FPS; ++i) info.option_supported[i] = true;

        info.intrinsics.resize(NUM_INTRINSICS);
        info.intrinsics[COLOR_VGA] = MakeColorIntrinsics(c, 640, 480);
        info.intrinsics[DEPTH_VGA] = MakeDepthIntrinsics(c, 640, 480);
        info.stream_poses[RS_STREAM_DEPTH] = info.stream_poses[RS_STREAM_INFRARED] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_COLOR] = {transpose((const float3x3 &)c.Rt), (const float3 &)c.Tt * 0.001f}; // convert mm to m
        info.depth_scale = (c.Rmax / 0xFFFF) * 0.001f; // convert mm to m*/
        return info;
    }

    f200_camera::f200_camera(uvc::device device, bool sr300) : rs_device(device), last_temperature_delta(std::numeric_limits<float>::infinity())
    {
        f200::claim_ivcam_interface(device);

        if(sr300)
        {
            std::tie(base_calibration, base_temperature_data, thermal_loop_params) = f200::read_sr300_calibration(device, usbMutex);
            device_info = add_standard_unpackers(get_sr300_info(base_calibration));
        }
        else
        {
            std::tie(base_calibration, base_temperature_data, thermal_loop_params) = f200::read_f200_calibration(device, usbMutex);
            device_info = add_standard_unpackers(get_f200_info(base_calibration));
        }

        compensated_calibration = base_calibration;
        f200::enable_timestamp(device, usbMutex, true, true); // Dimitri: debugging dangerously (assume we are pulling color + depth)

        // If thermal control loop requested, start up thread to handle it
		if(thermal_loop_params.IRThermalLoopEnable)
        {
            runTemperatureThread = true;
            temperatureThread = std::thread(&f200_camera::temperature_control_loop, this);
        }
    }

    f200_camera::~f200_camera()
    {
        // Shut down thermal control loop thread
        runTemperatureThread = false;
        temperatureCv.notify_one();
        if (temperatureThread.joinable())
            temperatureThread.join();        
    }

    void f200_camera::temperature_control_loop()
    {
        const float FcxSlope = base_calibration.Kc[0][0] * thermal_loop_params.FcxSlopeA + thermal_loop_params.FcxSlopeB;
        const float UxSlope = base_calibration.Kc[0][2] * thermal_loop_params.UxSlopeA + base_calibration.Kc[0][0] * thermal_loop_params.UxSlopeB + thermal_loop_params.UxSlopeC;
        const float tempFromHFOV = (tan(thermal_loop_params.HFOVsensitivity*M_PI/360)*(1 + base_calibration.Kc[0][0]*base_calibration.Kc[0][0]))/(FcxSlope * (1 + base_calibration.Kc[0][0] * tan(thermal_loop_params.HFOVsensitivity * M_PI/360)));
        float TempThreshold = thermal_loop_params.TempThreshold; //celcius degrees, the temperatures delta that above should be fixed;
        if (TempThreshold <= 0) TempThreshold = tempFromHFOV;
        if (TempThreshold > tempFromHFOV) TempThreshold = tempFromHFOV;

        std::unique_lock<std::mutex> lock(temperatureMutex);
        while (runTemperatureThread) 
        {
            //@tofix, this will throw if bad, but might periodically fail anyway. try/catch
            try
            {
                float IRTemp = (float)f200::read_ir_temp(device, usbMutex);
                float LiguriaTemp = f200::read_mems_temp(device, usbMutex);

                double IrBaseTemperature = base_temperature_data.IRTemp; //should be taken from the parameters
                double liguriaBaseTemperature = base_temperature_data.LiguriaTemp; //should be taken from the parameters

                // calculate deltas from the calibration and last fix
                double IrTempDelta = IRTemp - IrBaseTemperature;
                double liguriaTempDelta = LiguriaTemp - liguriaBaseTemperature;
                double weightedTempDelta = liguriaTempDelta * thermal_loop_params.LiguriaTempWeight + IrTempDelta * thermal_loop_params.IrTempWeight;
                double tempDetaFromLastFix = abs(weightedTempDelta - last_temperature_delta);

                //read intrinsic from the calibration working point
                double Kc11 = base_calibration.Kc[0][0];
                double Kc13 = base_calibration.Kc[0][2];

                // Apply model
                if (tempDetaFromLastFix >= TempThreshold)
                {
                    //if we are during a transition, fix for after the transition
                    double tempDeltaToUse = weightedTempDelta;
                    if (tempDeltaToUse > 0 && tempDeltaToUse < thermal_loop_params.TransitionTemp)
                    {
                        tempDeltaToUse = thermal_loop_params.TransitionTemp;
                    }

                    //calculate fixed values
                    double fixed_Kc11 = Kc11 + (FcxSlope * tempDeltaToUse) + thermal_loop_params.FcxOffset;
                    double fixed_Kc13 = Kc13 + (UxSlope * tempDeltaToUse) + thermal_loop_params.UxOffset;

                    //write back to intrinsic hfov and vfov
                    compensated_calibration = base_calibration;
                    compensated_calibration.Kc[0][0] = (float) fixed_Kc11;
                    compensated_calibration.Kc[1][1] = base_calibration.Kc[1][1] * (float)(fixed_Kc11/Kc11);
                    compensated_calibration.Kc[0][2] = (float) fixed_Kc13;
                    last_temperature_delta = weightedTempDelta;
                    // *******

                    //@tofix, qRes mode
                    DEBUG_OUT("updating asic with new temperature calibration coefficients");
                    update_asic_coefficients(device, usbMutex, compensated_calibration);
                }
            }
            catch(const std::exception & e) { DEBUG_ERR("TemperatureControlLoop: " << e.what()); }
            
            temperatureCv.wait_for(lock, std::chrono::seconds(10));
        }
    }

    void f200_camera::set_option(rs_option option, int value)
    {
        // TODO: Range check value before write
        auto val = static_cast<uint8_t>(value);
        switch(option)
        {
        case RS_OPTION_F200_LASER_POWER:          f200::set_laser_power(device, val); break;
        case RS_OPTION_F200_ACCURACY:             f200::set_accuracy(device, val); break;
        case RS_OPTION_F200_MOTION_RANGE:         f200::set_motion_range(device, val); break;
        case RS_OPTION_F200_FILTER_OPTION:        f200::set_filter_option(device, val); break;
        case RS_OPTION_F200_CONFIDENCE_THRESHOLD: f200::set_confidence_threshold(device, val); break;
        case RS_OPTION_F200_DYNAMIC_FPS:          f200::set_dynamic_fps(device, val); break; // IVCAM 1.5 Only
        }
    }

    int f200_camera::get_option(rs_option option) const
    {
        uint8_t value = 0;
        switch(option)
        {
        case RS_OPTION_F200_LASER_POWER:          f200::get_laser_power(device, value); break;
        case RS_OPTION_F200_ACCURACY:             f200::get_accuracy(device, value); break;
        case RS_OPTION_F200_MOTION_RANGE:         f200::get_motion_range(device, value); break;
        case RS_OPTION_F200_FILTER_OPTION:        f200::get_filter_option(device, value); break;
        case RS_OPTION_F200_CONFIDENCE_THRESHOLD: f200::get_confidence_threshold(device, value); break;
        case RS_OPTION_F200_DYNAMIC_FPS:          f200::get_dynamic_fps(device, value); break; // IVCAM 1.5 Only
        }
        return value;
    }

} // namespace rsimpl::f200

