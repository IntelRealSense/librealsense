// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <mutex>
#include <string>

#include "device.h"
#include "context.h"
#include "backend.h"
#include "ivcam-private.h"
#include "hw-monitor.h"
#include "image.h"

#include "core/debug.h"

namespace librealsense
{
    const uint16_t SR300_PID = 0x0aa5;

    class sr300_camera;

    class sr300_timestamp_reader : public frame_timestamp_reader
    {
        bool started;
        int64_t total;
        int last_timestamp;
        mutable int64_t counter;
        mutable std::recursive_mutex _mtx;
    public:
        sr300_timestamp_reader() : started(false), total(0), last_timestamp(0), counter(0) {}

        void reset() override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            started = false;
            total = 0;
            last_timestamp = 0;
            counter = 0;
        }

        double get_frame_timestamp(const request_mapping& /*mode*/, const uvc::frame_object& fo) override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            // Timestamps are encoded within the first 32 bits of the image
            int rolling_timestamp = *reinterpret_cast<const int32_t *>(fo.pixels);

            if (!started)
            {
                last_timestamp = rolling_timestamp;
                started = true;
            }

            const int delta = rolling_timestamp - last_timestamp; // NOTE: Relies on undefined behavior: signed int wraparound
            last_timestamp = rolling_timestamp;
            total += delta;
            const int timestamp = static_cast<int>(total / 100000);
            return timestamp;
        }

        unsigned long long get_frame_counter(const request_mapping & /*mode*/, const uvc::frame_object& fo) const override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            return ++counter;
        }

        rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const uvc::frame_object& fo) const override
        {
            if(fo.metadata_size > 0 )
                return RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;

            else
                return RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
        }
    };

    class sr300_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(const uvc::backend& backend) const override;

        sr300_info(std::shared_ptr<uvc::backend> backend,
            uvc::uvc_device_info color,
            uvc::uvc_device_info depth,
            uvc::usb_device_info hwm)
            : device_info(std::move(backend)), _color(std::move(color)),
             _depth(std::move(depth)), _hwm(std::move(hwm))
        {

        }

        static std::vector<std::shared_ptr<device_info>> pick_sr300_devices(
            std::shared_ptr<uvc::backend> backend,
            std::vector<uvc::uvc_device_info>& uvc,
            std::vector<uvc::usb_device_info>& usb);

        uvc::devices_data get_device_data() const override
        {
            return uvc::devices_data({ _color, _depth }, { _hwm });
        }

    private:
        uvc::uvc_device_info _color;
        uvc::uvc_device_info _depth;
        uvc::usb_device_info _hwm;
    };

    class sr300_camera final : public device, public debug_interface
    {
    public:
        class preset_option : public librealsense_option
        {
        public:
            void set(float value) override
            {
                if (!is_valid(value))
                    throw invalid_value_exception(to_string() << "set(preset_option) failed! Given value " << value << " is out of range.");

                _owner.rs2_apply_ivcam_preset(static_cast<int>(value));
                last_value = value;
            }

            float query() const override { return last_value; }

            bool is_enabled() const override { return true; }

            const char* get_description() const override
            {
                return "Recommended sets of options optimized for different visual use-cases";
            }

            const char* get_value_description(float val) const override
            {
                return rs2_visual_preset_to_string(
                    static_cast<rs2_visual_preset>(
                        static_cast<int>(val)));
            }

            explicit preset_option(sr300_camera& owner, const option_range& opt_range)
                : librealsense_option(opt_range),
                  _owner(owner)
            {}

        private:
            float last_value = RS2_VISUAL_PRESET_DEFAULT;
            sr300_camera& _owner;
        };

        class sr300_color_sensor : public uvc_sensor, public video_sensor_interface
        {
        public:
            explicit sr300_color_sensor(const sr300_camera* owner, std::shared_ptr<uvc::uvc_device> uvc_device,
                std::unique_ptr<frame_timestamp_reader> timestamp_reader,
                std::shared_ptr<uvc::time_service> ts)
                : uvc_sensor("RGB Camera", uvc_device, move(timestamp_reader), ts), _owner(owner)
            {}

            rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
            {
                return make_color_intrinsics(_owner->get_calibration(), { int(profile.width), int(profile.height) });
            }
        private:
            const sr300_camera* _owner;
        };

        class sr300_depth_sensor : public uvc_sensor, public video_sensor_interface
        {
        public:
            explicit sr300_depth_sensor(const sr300_camera* owner, std::shared_ptr<uvc::uvc_device> uvc_device,
                std::unique_ptr<frame_timestamp_reader> timestamp_reader,
                std::shared_ptr<uvc::time_service> ts)
                : uvc_sensor("Coded-Light Depth Sensor", uvc_device, move(timestamp_reader), ts), _owner(owner)
            {}

            rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
            {
                return make_depth_intrinsics(_owner->get_calibration(), { int(profile.width), int(profile.height) });
            }
        private:
            const sr300_camera* _owner;
        };

        std::shared_ptr<uvc_sensor> create_color_device(const uvc::backend& backend,
                                                        const uvc::uvc_device_info& color)
        {
            auto color_ep = std::make_shared<sr300_color_sensor>(this, backend.create_uvc_device(color),
                                                           std::unique_ptr<frame_timestamp_reader>(new sr300_timestamp_reader()),
                                                           backend.create_time_service());
            color_ep->register_pixel_format(pf_yuy2);
            color_ep->register_pixel_format(pf_yuyv);

            color_ep->register_pu(RS2_OPTION_BACKLIGHT_COMPENSATION);
            color_ep->register_pu(RS2_OPTION_BRIGHTNESS);
            color_ep->register_pu(RS2_OPTION_CONTRAST);
            color_ep->register_pu(RS2_OPTION_EXPOSURE);
            color_ep->register_pu(RS2_OPTION_GAIN);
            color_ep->register_pu(RS2_OPTION_GAMMA);
            color_ep->register_pu(RS2_OPTION_HUE);
            color_ep->register_pu(RS2_OPTION_SATURATION);
            color_ep->register_pu(RS2_OPTION_SHARPNESS);
            color_ep->register_pu(RS2_OPTION_WHITE_BALANCE);
            color_ep->register_pu(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
            color_ep->register_pu(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);


            color_ep->set_pose(lazy<pose>([](){pose p = {{ { 1,0,0 },{ 0,1,0 },{ 0,0,1 } },{ 0,0,0 }}; return p; }));

            return color_ep;
        }

        std::shared_ptr<uvc_sensor> create_depth_device(const uvc::backend& backend,
                                                        const uvc::uvc_device_info& depth)
        {
            using namespace ivcam;

            // create uvc-endpoint from backend uvc-device
            auto depth_ep = std::make_shared<sr300_depth_sensor>(this, backend.create_uvc_device(depth),
                                                           std::unique_ptr<frame_timestamp_reader>(new sr300_timestamp_reader()),
                                                           backend.create_time_service());
            depth_ep->register_xu(depth_xu); // make sure the XU is initialized everytime we power the camera
            depth_ep->register_pixel_format(pf_invz);
            depth_ep->register_pixel_format(pf_sr300_inzi);
            depth_ep->register_pixel_format(pf_sr300_invi);

            register_depth_xu<uint8_t>(*depth_ep, RS2_OPTION_LASER_POWER, IVCAM_DEPTH_LASER_POWER,
                "Power of the SR300 projector, with 0 meaning projector off");
            register_depth_xu<uint8_t>(*depth_ep, RS2_OPTION_ACCURACY, IVCAM_DEPTH_ACCURACY,
                "Set the number of patterns projected per frame.\nThe higher the accuracy value the more patterns projected.\nIncreasing the number of patterns help to achieve better accuracy.\nNote that this control is affecting the Depth FPS");
            register_depth_xu<uint8_t>(*depth_ep, RS2_OPTION_MOTION_RANGE, IVCAM_DEPTH_MOTION_RANGE,
                "Motion vs. Range trade-off, with lower values allowing for better motion\nsensitivity and higher values allowing for better depth range");
            register_depth_xu<uint8_t>(*depth_ep, RS2_OPTION_CONFIDENCE_THRESHOLD, IVCAM_DEPTH_CONFIDENCE_THRESH,
                "The confidence level threshold used by the Depth algorithm pipe to set whether\na pixel will get a valid range or will be marked with invalid range");
            register_depth_xu<uint8_t>(*depth_ep, RS2_OPTION_FILTER_OPTION, IVCAM_DEPTH_FILTER_OPTION,
                "Set the filter to apply to each depth frame.\nEach one of the filter is optimized per the application requirements");

            depth_ep->register_option(RS2_OPTION_VISUAL_PRESET, std::make_shared<preset_option>(*this,
                                                                                                option_range{0, RS2_VISUAL_PRESET_COUNT, 1, RS2_VISUAL_PRESET_DEFAULT}));

            return depth_ep;
        }

        std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) override
        {
            return _hw_monitor->send(input);
        }

        void hardware_reset() override
        {
            force_hardware_reset();
        }

        uvc_sensor& get_depth_sensor() { return dynamic_cast<uvc_sensor&>(get_sensor(_depth_device_idx)); }


        sr300_camera(const uvc::backend& backend,
            const uvc::uvc_device_info& color,
            const uvc::uvc_device_info& depth,
            const uvc::usb_device_info& hwm_device);

        void rs2_apply_ivcam_preset(int preset)
        {
            const auto DEPTH_CONTROLS = 5;
            const rs2_option arr_options[DEPTH_CONTROLS] = {
                RS2_OPTION_LASER_POWER,
                RS2_OPTION_ACCURACY,
                RS2_OPTION_FILTER_OPTION,
                RS2_OPTION_CONFIDENCE_THRESHOLD,
                RS2_OPTION_MOTION_RANGE
            };

            //const ivcam::cam_auto_range_request ar_requests[RS2_VISUAL_PRESET_COUNT] =
            //{
            //    { 1,     1, 180,  303,  180,   2,  16,  -1, 1000, 450 }, /* ShortRange                */
            //    { 1,     0, 303,  605,  303,  -1,  -1,  -1, 1250, 975 }, /* LongRange                 */
            //    { 0,     0,  -1,   -1,   -1,  -1,  -1,  -1,   -1,  -1 }, /* BackgroundSegmentation    */
            //    { 1,     1, 100,  179,  100,   2,  16,  -1, 1000, 450 }, /* GestureRecognition        */
            //    { 0,     1,  -1,   -1,   -1,   2,  16,  16, 1000, 450 }, /* ObjectScanning            */
            //    { 0,     0,  -1,   -1,   -1,  -1,  -1,  -1,   -1,  -1 }, /* FaceAnalytics             */
            //    { 2,     0,  40, 1600,  800,  -1,  -1,  -1,   -1,  -1 }, /* FaceLogin                 */
            //    { 1,     1, 100,  179,  179,   2,  16,  -1, 1000, 450 }, /* GRCursor                  */
            //    { 0,     0,  -1,   -1,   -1,  -1,  -1,  -1,   -1,  -1 }, /* Default                   */
            //    { 1,     1, 180,  605,  303,   2,  16,  -1, 1250, 650 }, /* MidRange                  */
            //    { 2,     0,  40, 1600,  800,  -1,  -1,  -1,   -1,  -1 }, /* IROnly                    */
            //};

            const float arr_values[RS2_VISUAL_PRESET_COUNT][DEPTH_CONTROLS] = {
                { 1,    1,   5,   1,  -1 }, /* ShortRange                */
                { 1,    1,   7,   0,  -1 }, /* LongRange                 */
                { 16,   1,   6,   2,  22 }, /* BackgroundSegmentation    */
                { 1,    1,   6,   3,  -1 }, /* GestureRecognition        */
                { 1,    1,   3,   1,   9 }, /* ObjectScanning            */
                { 16,   1,   5,   1,  22 }, /* FaceAnalytics             */
                { 1,   -1,  -1,  -1,  -1 }, /* FaceLogin                 */
                { 1,    1,   6,   1,  -1 }, /* GRCursor                  */
                { 16,   1,   5,   3,   9 }, /* Default                   */
                { 1,    1,   5,   1,  -1 }, /* MidRange                  */
                { 1,   -1,  -1,  -1, - 1 }  /* IROnly                    */
            };

            // The Default preset is handled differently from all the rest,
            // When the user applies the Default preset the camera is expected to return to
            // Default values of depth options:
            if (preset == RS2_VISUAL_PRESET_DEFAULT)
            {
                for (auto opt : arr_options)
                {
                    auto&& o = get_depth_sensor().get_option(opt);
                    o.set(o.get_range().def);
                }
            }
            else
            {
                for (auto i = 0; i < DEPTH_CONTROLS; i++)
                {
                    if (arr_values[preset][i] >= 0)
                    {
                        auto&& o = get_depth_sensor().get_option(arr_options[i]);
                        o.set(arr_values[preset][i]);
                    }
                }
                //if (arr_values[preset][0] == 1)
                    //set_auto_range(ar_requests[preset]);
            }
        }
        void create_snapshot(std::shared_ptr<debug_interface>& snapshot) override;
        void create_recordable(std::shared_ptr<debug_interface>& recordable,
                               std::function<void(std::shared_ptr<extension_snapshot>)> record_action) override;
    private:
        const uint8_t _depth_device_idx;
        const uint8_t _color_device_idx;
        std::shared_ptr<hw_monitor> _hw_monitor;

        template<class T>
        void register_depth_xu(uvc_sensor& depth, rs2_option opt, uint8_t id, std::string desc) const
        {
            depth.register_option(opt,
                std::make_shared<uvc_xu_option<T>>(
                    depth,
                    ivcam::depth_xu,
                    id, std::move(desc)));
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
            //register_option(RS2_OPTION_SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE, RS2_SUBDEVICE_DEPTH,
            //    make_field_option(arr_reader_writer, &ivcam::cam_auto_range_request::enableMvR, { 0, 2, 1, 1 }));
            //register_option(RS2_OPTION_SR300_AUTO_RANGE_ENABLE_LASER, RS2_SUBDEVICE_DEPTH,
            //    make_field_option(arr_reader_writer, &ivcam::cam_auto_range_request::enableLaser, { 0, 1, 1, 1 }));
            // etc..
        }

        static rs2_intrinsics make_depth_intrinsics(const ivcam::camera_calib_params& c, const int2& dims);
        static rs2_intrinsics make_color_intrinsics(const ivcam::camera_calib_params& c, const int2& dims);
        float read_mems_temp() const;
        int read_ir_temp() const;

        void force_hardware_reset() const;
        void enable_timestamp(bool colorEnable, bool depthEnable) const;
        void set_auto_range(const ivcam::cam_auto_range_request& c) const;

        ivcam::camera_calib_params get_calibration() const;
    };
}
