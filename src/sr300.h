// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>

#include "device.h"
#include "context.h"
#include "backend.h"
#include "ivcam-private.h"
#include "hw-monitor.h"
#include "image.h"

namespace rsimpl
{
    const uint16_t SR300_PID = 0x0aa5;

    class sr300_camera;

    class sr300_info : public device_info
    {
    public:
        std::shared_ptr<device> create(const uvc::backend& backend) const override;

        std::shared_ptr<device_info> clone() const override
        {
            return std::make_shared<sr300_info>(*this);
        }

        sr300_info(uvc::uvc_device_info color,
            uvc::uvc_device_info depth,
            uvc::usb_device_info hwm);

    private:
        uvc::uvc_device_info _color;
        uvc::uvc_device_info _depth;
        uvc::usb_device_info _hwm;
    };

    std::vector<std::shared_ptr<device_info>> pick_sr300_devices(
        std::vector<uvc::uvc_device_info>& uvc,
        std::vector<uvc::usb_device_info>& usb);

    class sr300_camera final : public device
    {
    public:
        class preset_option : public option
        {
        public:
            void set(float value) override { 
                _owner.rs_apply_ivcam_preset(static_cast<int>(value));
                last_value = value;
            }
            float query() const override { return last_value; }
            option_range get_range() const override
            {
                return{ 0, RS_VISUAL_PRESET_COUNT, 1, RS_VISUAL_PRESET_DEFAULT };
            }
            bool is_enabled() const override { return true; }

            const char* get_description() const override
            {
                return "Recommended sets of options optimized for different visual use-cases";
            }
            const char* get_value_description(float val) const override
            {
                return rs_visual_preset_to_string(
                    static_cast<rs_visual_preset>(
                        static_cast<int>(val)));
            }

            explicit preset_option(sr300_camera& owner) : _owner(owner) {}

        private:
            float last_value = RS_VISUAL_PRESET_DEFAULT;
            sr300_camera& _owner;
        };

        sr300_camera(const uvc::backend& backend,
            const uvc::uvc_device_info& color,
            const uvc::uvc_device_info& depth,
            const uvc::usb_device_info& hwm_device)
            : _hw_monitor(backend.create_usb_device(hwm_device))
        {
            using namespace ivcam;

            // create uvc-endpoint from backend uvc-device
            auto color_ep = std::make_shared<uvc_endpoint>(backend.create_uvc_device(color));
            auto depth_ep = std::make_shared<uvc_endpoint>(backend.create_uvc_device(depth));
            depth_ep->register_xu(depth_xu); // make sure the XU is initialized everytime we power the camera
            depth_ep->register_pixel_format(pf_invz);
            depth_ep->register_pixel_format(pf_sr300_inzi);
            depth_ep->register_pixel_format(pf_sr300_invi);
            color_ep->register_pixel_format(pf_yuy2);
            color_ep->register_pixel_format(pf_yuyv);

            auto fw_version = _hw_monitor.get_firmware_version_string(GVD, gvd_fw_version_offset);
            auto serial = _hw_monitor.get_module_serial_string(GVD);
            auto location = depth_ep->invoke_powered([](uvc::uvc_device& dev)
            {
                return dev.get_device_location();
            });
            enable_timestamp(true, true);


            register_device("Intel RealSense SR300", fw_version, serial, "");

            // map subdevice to endpoint
            assign_endpoint(RS_SUBDEVICE_COLOR, color_ep);
            assign_endpoint(RS_SUBDEVICE_DEPTH, depth_ep);

            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_BACKLIGHT_COMPENSATION);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_BRIGHTNESS);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_CONTRAST);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_EXPOSURE);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_GAIN);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_GAMMA);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_HUE);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_SATURATION);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_SHARPNESS);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_WHITE_BALANCE);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_ENABLE_AUTO_EXPOSURE);
            register_pu(RS_SUBDEVICE_COLOR, RS_OPTION_ENABLE_AUTO_WHITE_BALANCE);

            register_depth_xu<uint8_t>(RS_OPTION_LASER_POWER, IVCAM_DEPTH_LASER_POWER,
                "Power of the SR300 projector, with 0 meaning projector off");
            register_depth_xu<uint8_t>(RS_OPTION_ACCURACY, IVCAM_DEPTH_ACCURACY,
                "Set the number of patterns projected per frame.\nThe higher the accuracy value the more patterns projected.\nIncreasing the number of patterns help to achieve better accuracy.\nNote that this control is affecting the Depth FPS");
            register_depth_xu<uint8_t>(RS_OPTION_MOTION_RANGE, IVCAM_DEPTH_MOTION_RANGE,
                "Motion vs. Range trade-off, with lower values allowing for better motion\nsensitivity and higher values allowing for better depth range");
            register_depth_xu<uint8_t>(RS_OPTION_CONFIDENCE_THRESHOLD, IVCAM_DEPTH_CONFIDENCE_THRESH,
                "The confidence level threshold used by the Depth algorithm pipe to set whether\na pixel will get a valid range or will be marked with invalid range");
            register_depth_xu<uint8_t>(RS_OPTION_FILTER_OPTION, IVCAM_DEPTH_FILTER_OPTION,
                "Set the filter to apply to each depth frame.\nEach one of the filter is optimized per the application requirements");

            register_autorange_options();

            register_option(RS_OPTION_VISUAL_PRESET, RS_SUBDEVICE_DEPTH, std::make_shared<preset_option>(*this));

            auto c = get_calibration();
            pose depth_to_color = {
                transpose(reinterpret_cast<const float3x3 &>(c.Rt)),
                          reinterpret_cast<const float3 &>(c.Tt) * 0.001f
            };
            set_pose(RS_SUBDEVICE_DEPTH, inverse(depth_to_color));
            set_pose(RS_SUBDEVICE_COLOR, { { { 1,0,0 },{ 0,1,0 },{ 0,0,1 } },{ 0,0,0 } });
            set_depth_scale((c.Rmax / 0xFFFF) * 0.001f);
        }

        void rs_apply_ivcam_preset(int preset)
        {
            const auto DEPTH_CONTROLS = 5;
            const rs_option arr_options[DEPTH_CONTROLS] = {
                RS_OPTION_LASER_POWER,
                RS_OPTION_ACCURACY,
                RS_OPTION_FILTER_OPTION,
                RS_OPTION_CONFIDENCE_THRESHOLD,
                RS_OPTION_MOTION_RANGE
            };

            const ivcam::cam_auto_range_request ar_requests[RS_VISUAL_PRESET_COUNT] =
            {
                { 1,     1, 180,  303,  180,   2,  16,  -1, 1000, 450 }, /* ShortRange                */
                { 1,     0, 303,  605,  303,  -1,  -1,  -1, 1250, 975 }, /* LongRange                 */
                { 0,     0,  -1,   -1,   -1,  -1,  -1,  -1,   -1,  -1 }, /* BackgroundSegmentation    */
                { 1,     1, 100,  179,  100,   2,  16,  -1, 1000, 450 }, /* GestureRecognition        */
                { 0,     1,  -1,   -1,   -1,   2,  16,  16, 1000, 450 }, /* ObjectScanning            */
                { 0,     0,  -1,   -1,   -1,  -1,  -1,  -1,   -1,  -1 }, /* FaceAnalytics             */
                { 2,     0,  40, 1600,  800,  -1,  -1,  -1,   -1,  -1 }, /* FaceLogin                 */
                { 1,     1, 100,  179,  179,   2,  16,  -1, 1000, 450 }, /* GRCursor                  */
                { 0,     0,  -1,   -1,   -1,  -1,  -1,  -1,   -1,  -1 }, /* Default                   */
                { 1,     1, 180,  605,  303,   2,  16,  -1, 1250, 650 }, /* MidRange                  */
                { 2,     0,  40, 1600,  800,  -1,  -1,  -1,   -1,  -1 }, /* IROnly                    */
            };

            const float arr_values[RS_VISUAL_PRESET_COUNT][DEPTH_CONTROLS] = {
                { 1,  1,  5,  1, -1 }, /* ShortRange                */
                { 1,  1,  7,  0, -1 }, /* LongRange                 */
                { 16,  1,  6,  2, 22 }, /* BackgroundSegmentation    */
                { 1,  1,  6,  3, -1 }, /* GestureRecognition        */
                { 1,  1,  3,  1,  9 }, /* ObjectScanning            */
                { 16,  1,  5,  1, 22 }, /* FaceAnalytics             */
                { 1, -1, -1, -1, -1 }, /* FaceLogin                 */
                { 1,  1,  6,  1, -1 }, /* GRCursor                  */
                { 16,  1,  5,  3,  9 }, /* Default                   */
                { 1,  1,  5,  1, -1 }, /* MidRange                  */
                { 1, -1, -1, -1, -1 }  /* IROnly                    */
            };

            // The Default preset is handled differntly from all the rest,
            // When the user applies the Default preset the camera is expected to return to
            // Default values of depth options:
            if (preset == RS_VISUAL_PRESET_DEFAULT)
            {
                for (auto opt : arr_options)
                {
                    auto&& o = get_option(RS_SUBDEVICE_DEPTH, opt);
                    o.set(o.get_range().def);
                }
            }
            else
            {
                for (auto i = 0; i < DEPTH_CONTROLS; i++)
                {
                    if (arr_values[preset][i] >= 0)
                    {
                        auto&& o = get_option(RS_SUBDEVICE_DEPTH, arr_options[i]);
                        o.set(arr_values[preset][i]);
                    }
                }
                //if (arr_values[preset][0] == 1)
                    //set_auto_range(ar_requests[preset]);
            }
        }

        // NOTE: it is the user's responsibility to make sure the profile makes sense on the given subdevice. UB otherwise.
        virtual rs_intrinsics get_intrinsics(rs_subdevice subdevice, stream_request profile) const override
        {
            if (!supports(subdevice)) throw std::runtime_error("Requested subdevice is unsupported.");
            switch (subdevice) {
                // was getting errors about uint32_t to int conversions. Because they originate as ints, this should be safe
            case RS_SUBDEVICE_DEPTH: return make_depth_intrinsics(get_calibration(), { int(profile.width), int(profile.height) });
            case RS_SUBDEVICE_COLOR: return make_color_intrinsics(get_calibration(), { int(profile.width), int(profile.height) });
            default: throw std::runtime_error("Not Implemented");
            }
        }

    private:
        hw_monitor _hw_monitor;

        template<class T>
        void register_depth_xu(rs_option opt, uint8_t id, std::string desc)
        {
            register_option(opt, RS_SUBDEVICE_DEPTH,
                std::make_shared<uvc_xu_option<T>>(
                    get_uvc_endpoint(RS_SUBDEVICE_DEPTH),
                    ivcam::depth_xu, id, std::move(desc)));
        }

        void register_autorange_options()
        {
            auto arr = std::make_shared<ivcam::cam_auto_range_request>();
            auto arr_reader_writer = make_struct_interface<ivcam::cam_auto_range_request>(
                [arr]() { return *arr; },
                [arr, this](ivcam::cam_auto_range_request r) {
                set_auto_range(r);
                *arr = r;
            });
            //register_option(RS_OPTION_SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE, RS_SUBDEVICE_DEPTH,
            //    make_field_option(arr_reader_writer, &ivcam::cam_auto_range_request::enableMvR, { 0, 2, 1, 1 }));
            //register_option(RS_OPTION_SR300_AUTO_RANGE_ENABLE_LASER, RS_SUBDEVICE_DEPTH,
            //    make_field_option(arr_reader_writer, &ivcam::cam_auto_range_request::enableLaser, { 0, 1, 1, 1 }));
            // etc..
        }

        static rs_intrinsics make_depth_intrinsics(const ivcam::camera_calib_params& c, const int2& dims);
        static rs_intrinsics make_color_intrinsics(const ivcam::camera_calib_params& c, const int2& dims);
        float read_mems_temp() const;
        int read_ir_temp() const;

        void force_hardware_reset() const;
        void enable_timestamp(bool colorEnable, bool depthEnable) const;
        void set_auto_range(const ivcam::cam_auto_range_request& c) const;

        ivcam::camera_calib_params get_calibration() const;
    };
}


//#ifndef LIBREALSENSE_SR300_H
//#define LIBREALSENSE_SR300_H
//
//#include <atomic>
//#include "ivcam-private.h"
//#include "ivcam-device.h"
//
//#define SR300_PRODUCT_ID 0x0aa5
//
//namespace rsimpl
//{
//
//    class sr300_camera final : public iv_camera
//    {
//        void set_fw_logger_option(double value);
//        unsigned get_fw_logger_option();
//
//    public:
//        sr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const ivcam::camera_calib_params & calib);
//        ~sr300_camera() {};
//
//        void set_options(const rs_option options[], size_t count, const double values[]) override;
//        void get_options(const rs_option options[], size_t count, double values[]) override;
//
//        virtual void start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex) override;
//        virtual void stop_fw_logger() override;
//    };
//
//    std::shared_ptr<rs_device> make_sr300_device(std::shared_ptr<uvc::device> device);
//}
//
//#endif
