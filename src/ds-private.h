// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DS_PRIVATE_H
#define LIBREALSENSE_DS_PRIVATE_H

#include "uvc.h"
#include <algorithm>

namespace rsimpl
{
    namespace ds
    {
        const uvc::extension_unit lr_xu = {0, 2, 1, {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}}};

        const int STATUS_BIT_Z_STREAMING = 1 << 0;
        const int STATUS_BIT_LR_STREAMING = 1 << 1;
        const int STATUS_BIT_WEB_STREAMING = 1 << 2;

        struct ds_calibration
        {
            int version;
            uint32_t serial_number;
            rs_intrinsics modesLR[3];
            rs_intrinsics intrinsicsThird[2];
            rs_intrinsics modesThird[2][2];
            float Rthird[9], T[3], B;
        };

        ds_calibration read_camera_info(uvc::device & device);
        std::string read_firmware_version(uvc::device & device);

        ///////////////////////////////
        //// Extension unit controls //
        ///////////////////////////////

        enum class control // UVC extension control codes
        {
            command_response           = 1,
            fisheye_xu_strobe          = 1,
            iffley                     = 2,
            fisheye_xu_ext_trig        = 2,
            stream_intent              = 3,
            fisheye_exposure           = 3,
            depth_units                = 4,
            min_max                    = 5,
            disparity                  = 6,
            rectification              = 7,
            emitter                    = 8,
            temperature                = 9,
            depth_params               = 10,
            last_error                 = 12,
            embedded_count             = 13,
            lr_exposure                = 14,
            lr_autoexposure_parameters = 15,
            sw_reset                   = 16,
            lr_gain                    = 17,
            lr_exposure_mode           = 18,
            disparity_shift            = 19,
            status                     = 20,
            lr_exposure_discovery      = 21,
            lr_gain_discovery          = 22,
            hw_timestamp               = 23,
        };

        void xu_read(const uvc::device & device, uvc::extension_unit xu, control xu_ctrl, void * buffer, uint32_t length);
        void xu_write(uvc::device & device, uvc::extension_unit xu, control xu_ctrl, void * buffer, uint32_t length);

        template<class T> T xu_read(const uvc::device & dev, uvc::extension_unit xu, control ctrl) { T val; xu_read(dev, xu, ctrl, &val, sizeof(val)); return val; }
        template<class T> void xu_write(uvc::device & dev, uvc::extension_unit xu, control ctrl, const T & value) { T val = value; xu_write(dev, xu, ctrl, &val, sizeof(val)); }

        #pragma pack(push, 1)
        struct ae_params // Auto-exposure algorithm parameters
        {
            float mean_intensity_set_point;
            float bright_ratio_set_point;
            float kp_gain;
            float kp_exposure;
            float kp_dark_threshold;
            uint16_t exposure_top_edge;
            uint16_t exposure_bottom_edge;
            uint16_t exposure_left_edge;
            uint16_t exposure_right_edge;
        };

        struct dc_params // Depth control algorithm parameters
        {
            uint32_t robbins_munroe_minus_inc;
            uint32_t robbins_munroe_plus_inc;
            uint32_t median_thresh;
            uint32_t score_min_thresh;
            uint32_t score_max_thresh;
            uint32_t texture_count_thresh;
            uint32_t texture_diff_thresh;
            uint32_t second_peak_thresh;
            uint32_t neighbor_thresh;
            uint32_t lr_thresh;

            enum { MAX_PRESETS = 6 };
            static const dc_params presets[MAX_PRESETS];
        };

        class time_pad
        {
            std::chrono::high_resolution_clock::duration _duration;
            std::chrono::high_resolution_clock::time_point _start_time;

        public:
            time_pad(std::chrono::high_resolution_clock::duration duration) : _duration(duration) {}
            void start()
            {
                _start_time = std::chrono::high_resolution_clock::now();
            }

            void stop()
            {
                auto elapsed = std::chrono::high_resolution_clock::now() - _start_time;
                if (elapsed < _duration)
                {
                    auto left = _duration - elapsed;
                    std::this_thread::sleep_for(left);
                }
            }
        };
        
        struct range { uint16_t min, max; };
        struct disp_mode { uint32_t is_disparity_enabled; double disparity_multiplier; };
        struct rate_value { uint32_t rate, value; }; // Framerate dependent value, such as exposure or gain
        struct temperature { int8_t current, min, max, min_fault; };
        struct discovery { uint32_t fps, min, max, default_value, resolution; }; // Fields other than fps are in same units as field (exposure in tenths of a millisecond, gain as percent)
        #pragma pack(pop)

        void set_stream_intent(uvc::device & device, uint8_t & intent);
        void get_stream_status(const uvc::device & device, uint32_t & status);
        void force_firmware_reset(uvc::device & device);
        bool get_emitter_state(const uvc::device & device, bool is_streaming, bool is_depth_enabled);
        void set_emitter_state(uvc::device & device, bool state);

        void get_register_value(uvc::device & device, uint32_t reg, uint32_t & value);
        void set_register_value(uvc::device & device, uint32_t reg, uint32_t value);


        inline uint32_t     get_depth_units             (const uvc::device & device) { return xu_read<uint32_t   >(device, lr_xu, control::depth_units); }
        inline range        get_min_max_depth           (const uvc::device & device) { return xu_read<range      >(device, lr_xu, control::min_max); }
        inline disp_mode    get_disparity_mode          (const uvc::device & device) { return xu_read<disp_mode  >(device, lr_xu, control::disparity); }
        inline temperature  get_temperature             (const uvc::device & device) { return xu_read<temperature>(device, lr_xu, control::temperature); }
        inline dc_params    get_depth_params            (const uvc::device & device) { return xu_read<dc_params  >(device, lr_xu, control::depth_params); }
        inline uint8_t      get_last_error              (const uvc::device & device) { return xu_read<uint8_t    >(device, lr_xu, control::last_error); }
        inline rate_value   get_lr_exposure             (const uvc::device & device) { return xu_read<rate_value >(device, lr_xu, control::lr_exposure); }
        inline ae_params    get_lr_auto_exposure_params(const uvc::device & device, std::vector<supported_option> ae_vec) {
            auto ret_val = xu_read<ae_params  >(device, lr_xu, control::lr_autoexposure_parameters);

            for (auto& elem : ae_vec)
            {
                switch (elem.option)
                {
                case RS_OPTION_R200_AUTO_EXPOSURE_TOP_EDGE:
                    ret_val.exposure_top_edge = std::min((uint16_t)elem.max, ret_val.exposure_top_edge);
                    break;

                case RS_OPTION_R200_AUTO_EXPOSURE_BOTTOM_EDGE:
                    ret_val.exposure_bottom_edge = std::min((uint16_t)elem.max, ret_val.exposure_bottom_edge);
                    break;

                case RS_OPTION_R200_AUTO_EXPOSURE_LEFT_EDGE:
                    ret_val.exposure_left_edge = std::min((uint16_t)elem.max, ret_val.exposure_left_edge);
                    break;

                case RS_OPTION_R200_AUTO_EXPOSURE_RIGHT_EDGE:
                    ret_val.exposure_right_edge = std::min((uint16_t)elem.max, ret_val.exposure_right_edge);
                    break;
                }
            }
            return ret_val;
        }
        inline rate_value   get_lr_gain                 (const uvc::device & device) { return xu_read<rate_value >(device, lr_xu, control::lr_gain); }
        inline uint8_t      get_lr_exposure_mode        (const uvc::device & device) { return xu_read<uint8_t    >(device, lr_xu, control::lr_exposure_mode); }
        inline uint32_t     get_disparity_shift         (const uvc::device & device) { return xu_read<uint32_t   >(device, lr_xu, control::disparity_shift); }
        inline discovery    get_lr_exposure_discovery   (const uvc::device & device) { return xu_read<discovery  >(device, lr_xu, control::lr_exposure_discovery); }
        inline discovery    get_lr_gain_discovery       (const uvc::device & device) { return xu_read<discovery  >(device, lr_xu, control::lr_gain_discovery); }

        inline void         set_depth_units             (uvc::device & device, uint32_t units)      { xu_write(device, lr_xu, control::depth_units, units); }
        inline void         set_min_max_depth           (uvc::device & device, range min_max)       { xu_write(device, lr_xu, control::min_max, min_max); }
        inline void         set_disparity_mode          (uvc::device & device, disp_mode mode)      { xu_write(device, lr_xu, control::disparity, mode); }
        inline void         set_temperature             (uvc::device & device, temperature temp)    { xu_write(device, lr_xu, control::temperature, temp); }
        inline void         set_depth_params            (uvc::device & device, dc_params params)    { xu_write(device, lr_xu, control::depth_params, params); }        
        inline void         set_lr_auto_exposure_params (uvc::device & device, ae_params params)    {
            if (params.exposure_top_edge >= params.exposure_bottom_edge || params.exposure_left_edge >= params.exposure_right_edge)
                throw std::logic_error("set_lr_auto_exposure_params failed.");
            xu_write(device, lr_xu, control::lr_autoexposure_parameters, params); }        
        inline void         set_lr_exposure_mode        (uvc::device & device, uint8_t mode)        { xu_write(device, lr_xu, control::lr_exposure_mode, mode); }
        inline void         set_disparity_shift         (uvc::device & device, uint32_t shift)      { xu_write(device, lr_xu, control::disparity_shift, shift); }
        inline void         set_lr_exposure_discovery   (uvc::device & device, discovery disc)      { xu_write(device, lr_xu, control::lr_exposure_discovery, disc); }
        inline void         set_lr_gain_discovery       (uvc::device & device, discovery disc)      { xu_write(device, lr_xu, control::lr_gain_discovery, disc); }
        inline void         set_lr_exposure             (uvc::device & device, rate_value exposure) { if (get_lr_exposure_mode(device)) set_lr_exposure_mode(device,0); xu_write(device, lr_xu, control::lr_exposure, exposure);}
        inline void         set_lr_gain                 (uvc::device & device, rate_value gain)     { if (get_lr_exposure_mode(device)) set_lr_exposure_mode(device,0); xu_write(device, lr_xu, control::lr_gain, gain);}


        #pragma pack(push, 1)
        struct dinghy
        {
            uint32_t magicNumber;
            uint32_t frameCount;
            uint32_t frameStatus;
            uint32_t exposureLeftSum;
            uint32_t exposureLeftDarkCount;
            uint32_t exposureLeftBrightCount;
            uint32_t exposureRightSum;
            uint32_t exposureRightDarkCount;
            uint32_t exposureRightBrightCount;
            uint32_t CAMmoduleStatus;
            uint32_t pad0;
            uint32_t pad1;
            uint32_t pad2;
            uint32_t pad3;
            uint32_t VDFerrorStatus;
            uint32_t pad4;
        };
        #pragma pack(pop)
    }

    namespace zr300
    {
        const uvc::extension_unit fisheye_xu = { 3, 3, 2,{ 0xf6c3c3d1, 0x5cde, 0x4477,{ 0xad, 0xf0, 0x41, 0x33, 0xf5, 0x8d, 0xa6, 0xf4 } } };

        // Claim USB interface used for motion module device
        void claim_motion_module_interface(uvc::device & device);

        uint8_t get_fisheye_strobe(const uvc::device & device);
        void set_fisheye_strobe(uvc::device & device, uint8_t strobe);
        uint8_t get_fisheye_external_trigger(const uvc::device & device);
        void set_fisheye_external_trigger(uvc::device & device, uint8_t ext_trig);
        uint16_t get_fisheye_exposure(const uvc::device & device);
        void set_fisheye_exposure(uvc::device & device, uint16_t exposure);

    }
}

#endif // DS_PRIVATE_H
