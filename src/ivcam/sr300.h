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
#include "metadata-parser.h"
#include "image.h"
#include <cstddef>
#include "environment.h"
#include "core/debug.h"
#include "stream.h"

namespace librealsense
{
    const uint16_t SR300_PID = 0x0aa5;

    const double TIMESTAMP_10NSEC_TO_MSEC = 0.00001;

    class sr300_camera;

    class sr300_timestamp_reader : public frame_timestamp_reader
    {
        bool started;
        uint64_t total;

        uint32_t last_timestamp;
        mutable uint64_t counter;
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

        double get_frame_timestamp(const request_mapping& /*mode*/, const platform::frame_object& fo) override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            // Timestamps are encoded within the first 32 bits of the image and provided in 10nsec units
            uint32_t rolling_timestamp = *reinterpret_cast<const uint32_t *>(fo.pixels);
            if (!started)
            {
                total = last_timestamp = rolling_timestamp;
                last_timestamp = rolling_timestamp;
                started = true;
            }

            const int delta = rolling_timestamp - last_timestamp; // NOTE: Relies on undefined behavior: signed int wraparound
            last_timestamp = rolling_timestamp;
            total += delta;

            return total * 0.00001; // to msec
        }

        unsigned long long get_frame_counter(const request_mapping & /*mode*/, const platform::frame_object& fo) const override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            return ++counter;
        }

        rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const override
        {
            if(fo.metadata_size >= platform::uvc_header_size )
                return RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;
            else
                return RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
        }
    };

    class sr300_timestamp_reader_from_metadata : public frame_timestamp_reader
    {
       std::unique_ptr<sr300_timestamp_reader> _backup_timestamp_reader;
       bool one_time_note;
       mutable std::recursive_mutex _mtx;
       arithmetic_wraparound<uint32_t,uint64_t > ts_wrap;

    protected:

        bool has_metadata_ts(const platform::frame_object& fo) const
        {
            // Metadata support for a specific stream is immutable
            const bool has_md_ts = [&]{ std::lock_guard<std::recursive_mutex> lock(_mtx);
                return ((fo.metadata != nullptr) && (fo.metadata_size >= platform::uvc_header_size) && ((byte*)fo.metadata)[0] >= platform::uvc_header_size);
            }();

            return has_md_ts;
        }

        bool has_metadata_fc(const platform::frame_object& fo) const
        {
            // Metadata support for a specific stream is immutable
            const bool has_md_frame_counter = [&] { std::lock_guard<std::recursive_mutex> lock(_mtx);
            return ((fo.metadata != nullptr) && (fo.metadata_size > platform::uvc_header_size) && ((byte*)fo.metadata)[0] > platform::uvc_header_size);
            }();

            return has_md_frame_counter;
        }

    public:
        sr300_timestamp_reader_from_metadata() :_backup_timestamp_reader(nullptr), one_time_note(false)
        {
            _backup_timestamp_reader = std::unique_ptr<sr300_timestamp_reader>(new sr300_timestamp_reader());
            reset();
        }

        rs2_time_t get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo) override;

        unsigned long long get_frame_counter(const request_mapping & mode, const platform::frame_object& fo) const override;

        void reset() override;

        rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const override;
    };

    class sr300_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(std::shared_ptr<context> ctx,
                                                 bool register_device_notifications) const override;

        sr300_info(std::shared_ptr<context> ctx,
                    platform::uvc_device_info color,
                    platform::uvc_device_info depth,
                    platform::usb_device_info hwm)
            : device_info(ctx), _color(std::move(color)),
             _depth(std::move(depth)), _hwm(std::move(hwm)) {}

        static std::vector<std::shared_ptr<device_info>> pick_sr300_devices(
            std::shared_ptr<context> ctx,
            std::vector<platform::uvc_device_info>& platform,
            std::vector<platform::usb_device_info>& usb);

        platform::backend_device_group get_device_data() const override
        {
            return platform::backend_device_group({ _color, _depth }, { _hwm });
        }

    private:
        platform::uvc_device_info _color;
        platform::uvc_device_info _depth;
        platform::usb_device_info _hwm;
    };

    class sr300_camera final : public virtual device, public debug_interface
    {
    public:
        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            tags.push_back({ RS2_STREAM_COLOR, -1, 1920, 1080, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_INFRARED, -1, 640, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            return tags;
        };

        class preset_option : public option_base
        {
        public:
            void set(float value) override
            {
                if (!is_valid(value))
                    throw invalid_value_exception(to_string() << "set(preset_option) failed! Given value " << value << " is out of range.");

                _owner.rs2_apply_ivcam_preset(static_cast<int>(value));
                last_value = value;
                _recording_function(*this);
            }

            float query() const override { return last_value; }

            bool is_enabled() const override { return true; }

            const char* get_description() const override
            {
                return "Recommended sets of options optimized for different visual use-cases";
            }

            const char* get_value_description(float val) const override
            {
                return rs2_sr300_visual_preset_to_string(
                    static_cast<rs2_sr300_visual_preset>(
                        static_cast<int>(val)));
            }

            explicit preset_option(sr300_camera& owner, const option_range& opt_range)
                : option_base(opt_range),
                  _owner(owner)
            {}

        private:
            float last_value = RS2_SR300_VISUAL_PRESET_DEFAULT;
            sr300_camera& _owner;
        };

        class sr300_color_sensor : public uvc_sensor, public video_sensor_interface
        {
        public:
            explicit sr300_color_sensor(sr300_camera* owner, std::shared_ptr<platform::uvc_device> uvc_device,
                std::unique_ptr<frame_timestamp_reader> timestamp_reader,
                std::shared_ptr<context> ctx)
                : uvc_sensor("RGB Camera", uvc_device, move(timestamp_reader), owner), _owner(owner)
            {}

            rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
            {
                return make_color_intrinsics(*_owner->_camer_calib_params, { int(profile.width), int(profile.height) });
            }

            stream_profiles init_stream_profiles() override
            {
                auto lock = environment::get_instance().get_extrinsics_graph().lock();

                auto results = uvc_sensor::init_stream_profiles();

                for (auto p : results)
                {
                    // Register stream types
                    if (p->get_stream_type() == RS2_STREAM_COLOR)
                    {
                        assign_stream(_owner->_color_stream, p);
                    }

                    // Register intrinsics
                    auto video = dynamic_cast<video_stream_profile_interface*>(p.get());

                    auto profile = to_profile(p.get());
                    std::weak_ptr<sr300_color_sensor> wp =
                        std::dynamic_pointer_cast<sr300_color_sensor>(this->shared_from_this());
                    video->set_intrinsics([profile, wp]()
                    {
                        auto sp = wp.lock();
                        if (sp)
                            return sp->get_intrinsics(profile);
                        else
                            return rs2_intrinsics{};
                    });
                }

                return results;
            }
        private:
            const sr300_camera* _owner;
        };

        class sr300_depth_sensor : public uvc_sensor, public video_sensor_interface, public depth_sensor
        {
        public:
            explicit sr300_depth_sensor(sr300_camera* owner, std::shared_ptr<platform::uvc_device> uvc_device,
                std::unique_ptr<frame_timestamp_reader> timestamp_reader,
                std::shared_ptr<context> ctx)
                : uvc_sensor("Coded-Light Depth Sensor", uvc_device, move(timestamp_reader), owner), _owner(owner)
            {}

            rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
            {
                return make_depth_intrinsics(*_owner->_camer_calib_params, { int(profile.width), int(profile.height) });
            }

            stream_profiles init_stream_profiles() override
            {
                 auto lock = environment::get_instance().get_extrinsics_graph().lock();

                auto results = uvc_sensor::init_stream_profiles();

                for (auto p : results)
                {
                    // Register stream types
                    if (p->get_stream_type() == RS2_STREAM_DEPTH)
                    {
                        assign_stream(_owner->_depth_stream, p);
                    }
                    else if (p->get_stream_type() == RS2_STREAM_INFRARED)
                    {
                        assign_stream(_owner->_ir_stream, p);
                    }

                    // Register intrinsics
                    auto video = dynamic_cast<video_stream_profile_interface*>(p.get());

                    auto profile = to_profile(p.get());
                    std::weak_ptr<sr300_depth_sensor> wp =
                        std::dynamic_pointer_cast<sr300_depth_sensor>(this->shared_from_this());

                    video->set_intrinsics([profile, wp]()
                    {
                        auto sp = wp.lock();
                        if (sp)
                            return sp->get_intrinsics(profile);
                        else
                            return rs2_intrinsics{};
                    });
                }

                return results;
            }

            float get_depth_scale() const override { return get_option(RS2_OPTION_DEPTH_UNITS).query(); }

            void create_snapshot(std::shared_ptr<depth_sensor>& snapshot) const  override
            {
                snapshot = std::make_shared<depth_sensor_snapshot>(get_depth_scale());
            }
            void enable_recording(std::function<void(const depth_sensor&)> recording_function) override
            {
                get_option(RS2_OPTION_DEPTH_UNITS).enable_recording([this, recording_function](const option& o) {
                    recording_function(*this);
                });
            }
        private:
            const sr300_camera* _owner;
        };

        std::shared_ptr<uvc_sensor> create_color_device(std::shared_ptr<context> ctx,
                                                        const platform::uvc_device_info& color)
        {
            auto color_ep = std::make_shared<sr300_color_sensor>(this, ctx->get_backend().create_uvc_device(color),
                                                           std::unique_ptr<frame_timestamp_reader>(new sr300_timestamp_reader_from_metadata()),
                                                           ctx);
            color_ep->register_pixel_format(pf_yuy2);
            color_ep->register_pixel_format(pf_yuyv);

            color_ep->register_pu(RS2_OPTION_BACKLIGHT_COMPENSATION);
            color_ep->register_pu(RS2_OPTION_BRIGHTNESS);
            color_ep->register_pu(RS2_OPTION_CONTRAST);
            color_ep->register_pu(RS2_OPTION_GAIN);
            color_ep->register_pu(RS2_OPTION_GAMMA);
            color_ep->register_pu(RS2_OPTION_HUE);
            color_ep->register_pu(RS2_OPTION_SATURATION);
            color_ep->register_pu(RS2_OPTION_SHARPNESS);

            auto white_balance_option = std::make_shared<uvc_pu_option>(*color_ep, RS2_OPTION_WHITE_BALANCE);
            auto auto_white_balance_option = std::make_shared<uvc_pu_option>(*color_ep, RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
            color_ep->register_option(RS2_OPTION_WHITE_BALANCE, white_balance_option);
            color_ep->register_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, auto_white_balance_option);
            color_ep->register_option(RS2_OPTION_WHITE_BALANCE,
                std::make_shared<auto_disabling_control>(
                    white_balance_option,
                    auto_white_balance_option));

            auto exposure_option = std::make_shared<uvc_pu_option>(*color_ep, RS2_OPTION_EXPOSURE);
            auto auto_exposure_option = std::make_shared<uvc_pu_option>(*color_ep, RS2_OPTION_ENABLE_AUTO_EXPOSURE);
            color_ep->register_option(RS2_OPTION_EXPOSURE, exposure_option);
            color_ep->register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, auto_exposure_option);
            color_ep->register_option(RS2_OPTION_EXPOSURE,
                std::make_shared<auto_disabling_control>(
                    exposure_option,
                    auto_exposure_option));

            auto md_offset = offsetof(metadata_raw, mode);
            color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp,
                [](rs2_metadata_type param) { return static_cast<rs2_metadata_type>(param * TIMESTAMP_10NSEC_TO_MSEC); }));
            color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER,   make_sr300_attribute_parser(&md_sr300_rgb::frame_counter, md_offset));
            color_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_FPS,      make_sr300_attribute_parser(&md_sr300_rgb::actual_fps, md_offset));
            color_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP,make_sr300_attribute_parser(&md_sr300_rgb::frame_latency, md_offset));
            color_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, make_sr300_attribute_parser(&md_sr300_rgb::actual_exposure, md_offset, [](rs2_metadata_type param) { return param*100; }));
            color_ep->register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE,   make_sr300_attribute_parser(&md_sr300_rgb::auto_exp_mode, md_offset, [](rs2_metadata_type param) { return (param !=1); }));
            color_ep->register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL,      make_sr300_attribute_parser(&md_sr300_rgb::gain, md_offset));
            color_ep->register_metadata(RS2_FRAME_METADATA_WHITE_BALANCE,   make_sr300_attribute_parser(&md_sr300_rgb::color_temperature, md_offset));

            return color_ep;
        }

        std::shared_ptr<uvc_sensor> create_depth_device(std::shared_ptr<context> ctx,
                                                        const platform::uvc_device_info& depth)
        {
            using namespace ivcam;

            auto&& backend = ctx->get_backend();

            // create uvc-endpoint from backend uvc-device
            auto depth_ep = std::make_shared<sr300_depth_sensor>(this, backend.create_uvc_device(depth),
                                                           std::unique_ptr<frame_timestamp_reader>(new sr300_timestamp_reader_from_metadata()),
                                                           ctx);
            depth_ep->register_xu(depth_xu); // make sure the XU is initialized everytime we power the camera
            depth_ep->register_pixel_format(pf_invz);
            depth_ep->register_pixel_format(pf_y8);
            depth_ep->register_pixel_format(pf_sr300_invi);
            depth_ep->register_pixel_format(pf_sr300_inzi);

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
                                                                                                option_range{0, RS2_SR300_VISUAL_PRESET_COUNT - 1, 1, RS2_SR300_VISUAL_PRESET_DEFAULT}));

            auto md_offset = offsetof(metadata_raw, mode);

            depth_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp,
                [](rs2_metadata_type param) { return static_cast<rs2_metadata_type>(param * TIMESTAMP_10NSEC_TO_MSEC); }));
            depth_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER,   make_sr300_attribute_parser(&md_sr300_depth::frame_counter, md_offset));
            depth_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, make_sr300_attribute_parser(&md_sr300_depth::actual_exposure, md_offset,
                [](rs2_metadata_type param) { return param * 100; }));
            depth_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_FPS,      make_sr300_attribute_parser(&md_sr300_depth::actual_fps, md_offset));

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


        sr300_camera(std::shared_ptr<context> ctx,
                     const platform::uvc_device_info& color,
                     const platform::uvc_device_info& depth,
                     const platform::usb_device_info& hwm_device,
                     const platform::backend_device_group& group,
                     bool register_device_notifications);

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

            // This extra functionality is disable for now:
            //const ivcam::cam_auto_range_request ar_requests[RS2_IVCAM_VISUAL_PRESET_COUNT] =
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

            const float arr_values[RS2_SR300_VISUAL_PRESET_COUNT][DEPTH_CONTROLS] = {
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
            if (preset == RS2_SR300_VISUAL_PRESET_DEFAULT)
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
        void create_snapshot(std::shared_ptr<debug_interface>& snapshot) const override;
        void enable_recording(std::function<void(const debug_interface&)> record_action) override;


        virtual std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;
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

        std::shared_ptr<stream_interface> _depth_stream;
        std::shared_ptr<stream_interface> _ir_stream;
        std::shared_ptr<stream_interface> _color_stream;
        std::shared_ptr<lazy<rs2_extrinsics>> _depth_to_color_extrinsics;

        lazy<ivcam::camera_calib_params> _camer_calib_params;
    };
}
