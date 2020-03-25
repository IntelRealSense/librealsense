// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "ds5-factory.h"
#include "ds5-private.h"
#include "ds5-options.h"
#include "ds5-timestamp.h"
#include "ds5-nonmonochrome.h"
#include "ds5-active.h"
#include "ds5-color.h"
#include "ds5-motion.h"
#include "sync.h"

namespace librealsense
{
    // PSR
    class rs400_device : public ds5_nonmonochrome, public ds5_advanced_mode_base
    {
    public:
        rs400_device(std::shared_ptr<context> ctx,
                     const platform::backend_device_group& group,
                     bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_nonmonochrome(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 1280, 720, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            }
            else
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_RGB8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 640, 480, RS2_FORMAT_RGB8, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }
            return tags;
        };
    };

    // DS5U_S
    class rs405u_device : public ds5u_device,
        public ds5_advanced_mode_base
    {
    public:
        rs405u_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
            ds5u_device(ctx, group),
            ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 720, 720, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 1152, 1152, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 1152, 1152, RS2_FORMAT_Y16, 30, profile_tag::PROFILE_TAG_SUPERSET });
            }
            else
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 720, 720, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 1152, 1152, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 1152, 1152, RS2_FORMAT_Y16, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }
            return tags;
        };

        bool contradicts(const stream_profile_interface* a, const std::vector<stream_profile>& others) const override
        {
            if (auto vid_a = dynamic_cast<const video_stream_profile_interface*>(a))
            {
                for (auto request : others)
                {
                    if (a->get_framerate() != 0 && request.fps != 0 && (a->get_framerate() != request.fps))
                        return true;
                }
            }
            return false;
        }
    };

    // ASR (D460)
    class rs410_device : public ds5_nonmonochrome,
                         public ds5_active, public ds5_advanced_mode_base
    {
    public:
        rs410_device(std::shared_ptr<context> ctx,
                     const platform::backend_device_group& group,
                     bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_nonmonochrome(ctx, group),
              ds5_active(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 1280, 720, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 0, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            }
            else
            {
                //TODO: F416 currntlly detected as RS_USB2_PID when connected via USB2 port
            }
            return tags;
        };
    };

    // ASRC
    class rs415_device : public ds5_nonmonochrome,
                         public ds5_active,
                         public ds5_color,
                         public ds5_advanced_mode_base
    {
    public:
        rs415_device(std::shared_ptr<context> ctx,
                     const platform::backend_device_group& group,
                     bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_nonmonochrome(ctx, group),
              ds5_active(ctx, group),
              ds5_color(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_COLOR, -1, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_DEPTH, -1, 1280, 720, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, -1, 1280, 720, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            }
            else
            {
                tags.push_back({ RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, -1, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }
            return tags;
        };
    };

    class rs416_device : public ds5_nonmonochrome,
        public ds5_active, public ds5_advanced_mode_base
    {
    public:
        rs416_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
            ds5_device(ctx, group),
            ds5_nonmonochrome(ctx, group),
            ds5_active(ctx, group),
            ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 720, 720, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 0, 720, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            }
            else
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            }
            return tags;
        };

        bool contradicts(const stream_profile_interface* a, const std::vector<stream_profile>& others) const override
        {
            if (auto vid_a = dynamic_cast<const video_stream_profile_interface*>(a))
            {
                for (auto request : others)
                {
                    if (a->get_framerate() != 0 && request.fps != 0 && (a->get_framerate() != request.fps))
                        return true;
                }
            }
            return false;
        }
    };

    class rs416_rgb_device :
        public ds5_nonmonochrome,
        public ds5_active,
        public ds5_color,
        public ds5_advanced_mode_base

    {
    public:
        rs416_rgb_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
            ds5_device(ctx, group),
            ds5_nonmonochrome(ctx, group),
            ds5_active(ctx, group),
            ds5_color(ctx, group),
            ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 720, 720, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 720, 720, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            }
            else
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }
            return tags;
        };

        bool contradicts(const stream_profile_interface* a, const std::vector<stream_profile>& others) const override
        {
            if (auto vid_a = dynamic_cast<const video_stream_profile_interface*>(a))
            {
                for (auto request : others)
                {
                    if (a->get_framerate() != 0 && request.fps != 0 && (a->get_framerate() != request.fps))
                        return true;
                }
            }
            return false;
        }
    };

    // PWGT
    class rs420_mm_device : public ds5_motion, public ds5_advanced_mode_base
    {
    public:
        rs420_mm_device(std::shared_ptr<context> ctx,
                        const platform::backend_device_group group,
                        bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_motion(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            }
            else
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }

            if (_fw_version >= firmware_version("5.10.4.0"))
            {
                tags.push_back({ RS2_STREAM_FISHEYE, -1, 640, 480, RS2_FORMAT_RAW8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT});
                tags.push_back({RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_63, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            }

            return tags;
        };
    };

    // PWG
    class rs420_device : public ds5_device, public ds5_advanced_mode_base
    {
    public:
        rs420_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
                     bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            }
            else
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }
            return tags;
        };
    };

    // AWG
    class rs430_device : public ds5_active, public ds5_advanced_mode_base
    {
    public:
        rs430_device(std::shared_ptr<context> ctx,
                     const platform::backend_device_group group,
                     bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            }
            else
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }
            return tags;
        };
    };

    class rs430i_device : public ds5_active, public ds5_advanced_mode_base, public ds5_motion
    {
    public:
        rs430i_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group group,
            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()),
              ds5_motion(ctx, group)
        {}

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            }
            else
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }
            tags.push_back({ RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_63, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });

            return tags;
        };
        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;
    };

    // AWGT
    class rs430_mm_device : public ds5_active,
                            public ds5_motion,
                            public ds5_advanced_mode_base
    {
    public:
        rs430_mm_device(std::shared_ptr<context> ctx,
                        const platform::backend_device_group group,
                        bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_motion(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });

            }
            else
            {
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, 2, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }

            tags.push_back({ RS2_STREAM_FISHEYE, -1, 640, 480, RS2_FORMAT_RAW8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT});
            if (_fw_version >= firmware_version("5.10.4.0")) // Back-compat with records is required
            {
                tags.push_back({RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_63, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            }

            return tags;
        };
    };

    // AWGC
    class rs435_device : public ds5_active,
                         public ds5_color,
                         public ds5_advanced_mode_base
    {
    public:
        rs435_device(std::shared_ptr<context> ctx,
                     const platform::backend_device_group group,
                     bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_color(ctx,  group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, -1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            }
            else
            {
                tags.push_back({ RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, -1, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }
            return tags;
        };
    };

    // AWGCT
    class rs430_rgb_mm_device : public ds5_active,
                                public ds5_color,
                                public ds5_motion,
                                public ds5_advanced_mode_base
    {
    public:
        rs430_rgb_mm_device(std::shared_ptr<context> ctx,
                            const platform::backend_device_group group,
                            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_color(ctx,  group),
              ds5_motion(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            if (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined)
            {
                tags.push_back({ RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_DEPTH, -1, 848, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, -1, 848, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            }
            else
            {
                tags.push_back({ RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 15, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
                tags.push_back({ RS2_STREAM_INFRARED, -1, 640, 480, RS2_FORMAT_Y8, 15, profile_tag::PROFILE_TAG_SUPERSET });
            }
            return tags;
        };
    };


    class rs435i_device  :      public ds5_active,
                                public ds5_color,
                                public ds5_motion,
                                public ds5_advanced_mode_base
    {
    public:
        rs435i_device(std::shared_ptr<context> ctx,
                    const platform::backend_device_group group,
                    bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_color(ctx,  group),
              ds5_motion(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) 
        {
            check_and_restore_rgb_stream_extrinsic();
        }

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            bool usb3mode = (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined);

            uint32_t depth_width  = usb3mode ?      848 : 640;
            uint32_t depth_height = usb3mode ?      480 : 480;
            uint32_t color_width = usb3mode ?       1280 : 640;
            uint32_t color_height = usb3mode ?      720 : 480;
            uint32_t fps    = usb3mode ?            30 :  15;

            tags.push_back({ RS2_STREAM_COLOR, -1, color_width, color_height, RS2_FORMAT_RGB8, fps, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_DEPTH, -1, depth_width, depth_height, RS2_FORMAT_Z16, fps, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_INFRARED, -1, depth_width, depth_height, RS2_FORMAT_Y8, fps, profile_tag::PROFILE_TAG_SUPERSET });
            tags.push_back({RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_63, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });

            return tags;
        };
        bool compress_while_record() const override { return false; }

    private:
        void check_and_restore_rgb_stream_extrinsic()
        {
            for(auto iter = 0, rec =0; iter < 2; iter++, rec++)
            {
                std::vector<byte> cal;
                try
                {
                    cal = *_color_calib_table_raw;
                }
                catch (...)
                {
                    LOG_WARNING("Cannot read RGB calibration table");
                }

                if (!is_rgb_extrinsic_valid(cal) && !rec)
                {
                    restore_rgb_extrinsic();
                }
                else
                    break;
            };
        }

        bool is_rgb_extrinsic_valid(const std::vector<uint8_t>& raw_data) const
        {
            try
            {
                // verify extrinsic calibration table structure
                auto table = ds::check_calib<ds::rgb_calibration_table>(raw_data);

                if ( (table->header.version != 0 && table->header.version != 0xffff) && (table->header.table_size >= sizeof(ds::rgb_calibration_table) - sizeof(ds::table_header)))
                {
                    float3 trans_vector = table->translation_rect;
                    // Translation Heuristic tests
                    auto found = false;
                    for (auto i = 0; i < 3; i++)
                    {
                        //Nan/Infinity are not allowed
                        if (!std::isfinite(trans_vector[i]))
                        {
                            LOG_WARNING("RGB extrinsic - translation is corrupted: " << trans_vector);
                            return false;
                        }
                        // Translation must be assigned for at least one axis 
                        if (std::fabs(trans_vector[i]) > std::numeric_limits<float>::epsilon())
                            found = true;
                    }

                    if (!found)
                    {
                        LOG_WARNING("RGB extrinsic - invalid (zero) translation: " << trans_vector);
                        return false;
                    }

                    // Rotation Heuristic tests
                    auto num_found = 0;
                    float3x3 rect_rot_mat = table->rotation_matrix_rect;
                    for (auto i = 0; i < 3; i++)
                    {
                        for (auto j = 0; j < 3; j++)
                        {
                            //Nan/Infinity are not allowed
                            if (!std::isfinite(rect_rot_mat(i, j)))
                            {
                                LOG_DEBUG("RGB extrinsic - rotation matrix corrupted:\n" << rect_rot_mat);
                                return false;
                            }

                            if (std::fabs(rect_rot_mat(i, j)) > std::numeric_limits<float>::epsilon())
                                num_found++;
                        }
                    }

                    bool res = (num_found >= 3); // At least three matrix indexes must be non-zero
                    if (!res) // At least three matrix indexes must be non-zero
                        LOG_DEBUG("RGB extrinsic - rotation matrix invalid:\n" << rect_rot_mat);

                    return res;
                }
                else
                {
                    LOG_WARNING("RGB extrinsic - header corrupted: "
                        << "Version: " <<std::setfill('0') << std::setw(4) << std::hex << table->header.version
                        << ", type " << std::dec << table->header.table_type << ", size " << table->header.table_size);
                    return false;
                }
            }
            catch (...)
            {
                return false;
            }
        }

        void assign_rgb_stream_extrinsic(const std::vector<byte>& calib)
        {
            //write calibration to preset
            command cmd(ds::fw_cmd::SETINTCALNEW, 0x20, 0x2);
            cmd.data = calib;
            ds5_device::_hw_monitor->send(cmd);
        }

        std::vector<byte> read_sector(const uint32_t address, const uint16_t size) const
        {
            if (size > ds5_advanced_mode_base::HW_MONITOR_COMMAND_SIZE)
                throw std::runtime_error(to_string() << "Device memory read failed. max size: "
                    << int(ds5_advanced_mode_base::HW_MONITOR_COMMAND_SIZE)
                    << ", requested: " << int(size));
            command cmd(ds::fw_cmd::FRB, address, size);
            return ds5_device::_hw_monitor->send(cmd);
        }

        std::vector<byte> read_rgb_gold() const
        {
            command cmd(ds::fw_cmd::LOADINTCAL, 0x20, 0x1);
            return ds5_device::_hw_monitor->send(cmd);
        }

        std::vector<byte> restore_calib_factory_settings() const
        {
            command cmd(ds::fw_cmd::CAL_RESTORE_DFLT);
            return ds5_device::_hw_monitor->send(cmd);
        }

        void restore_rgb_extrinsic(void)
        {
            bool res = false;
            LOG_WARNING("invalid RGB extrinsic was identified, recovery routine was invoked");
            try
            {
                if ((res = is_rgb_extrinsic_valid(read_rgb_gold())))
                {
                    restore_calib_factory_settings();
                }
                else
                {
                    if (_fw_version == firmware_version("5.11.6.200"))
                    {
                        const uint32_t gold_address = 0x17c49c;
                        const uint16_t bytes_to_read = 0x100;
                        auto alt_calib = read_sector(gold_address, bytes_to_read);
                        if ((res = is_rgb_extrinsic_valid(alt_calib)))
                            assign_rgb_stream_extrinsic(alt_calib);
                        else
                            res = false;
                    }
                    else
                        res = false;
                }

                // Update device's internal state
                if (res)
                {
                    LOG_WARNING("RGB stream extrinsic successfully recovered");
                    _color_calib_table_raw.reset();
                    _color_extrinsic.get()->reset();
                    environment::get_instance().get_extrinsics_graph().register_extrinsics(*_color_stream, *_depth_stream, _color_extrinsic);
                }
                else
                {
                    LOG_ERROR("RGB Extrinsic recovery routine failed");
                    _color_extrinsic.get()->reset();
                }
            }
            catch (...)
            {
                LOG_ERROR("RGB Extrinsic recovery routine failed");
            }
        }
    };

    class rs465_device : public ds5_active,
                         public ds5_nonmonochrome,
                         public ds5_color,
                         public ds5_motion,
                         public ds5_advanced_mode_base
    {
    public:
        rs465_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
            ds5_device(ctx, group),
            ds5_active(ctx, group),
            ds5_color(ctx, group),
            ds5_motion(ctx, group),
            ds5_nonmonochrome(ctx, group),
            ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            bool usb3mode = (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined);

            uint32_t width = usb3mode ? 1280 : 640;
            uint32_t height = usb3mode ? 720 : 480;
            uint32_t fps = usb3mode ? 30 : 15;

            tags.push_back({ RS2_STREAM_COLOR, -1, width, height, RS2_FORMAT_RGB8, fps, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_DEPTH, -1, width, height, RS2_FORMAT_Z16, fps, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_INFRARED, -1, width, height, RS2_FORMAT_Y8, fps, profile_tag::PROFILE_TAG_SUPERSET });
            tags.push_back({ RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_100, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });

            return tags;
        };
    };

    class rs400_imu_device  :      public ds5_motion,
                                public ds5_advanced_mode_base
    {
    public:
        rs400_imu_device(std::shared_ptr<context> ctx,
                    const platform::backend_device_group group,
                    bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_motion(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            tags.push_back({RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_63, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });

            return tags;
        };
    };

    class rs405_device  :      public ds5_active,
                                public ds5_color,
                                public ds5_motion,
                                public ds5_advanced_mode_base
    {
    public:
        rs405_device(std::shared_ptr<context> ctx,
                    const platform::backend_device_group group,
                    bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_color(ctx,  group),
              ds5_motion(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())
        {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            bool usb3mode = (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined);

            uint32_t depth_width  = usb3mode ?      848 : 640;
            uint32_t depth_height = usb3mode ?      480 : 480;
            uint32_t color_width = usb3mode ?       1280 : 640;
            uint32_t color_height = usb3mode ?      720 : 480;
            uint32_t fps    = usb3mode ?            30 :  15;

            tags.push_back({ RS2_STREAM_COLOR, -1, color_width, color_height, RS2_FORMAT_RGB8, fps, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_DEPTH, -1, depth_width, depth_height, RS2_FORMAT_Z16, fps, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_INFRARED, -1, depth_width, depth_height, RS2_FORMAT_Y8, fps, profile_tag::PROFILE_TAG_SUPERSET });
            tags.push_back({RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_63, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });

            return tags;
        }

        bool compress_while_record() const override { return false; }
    };

    class rs455_device  :      public ds5_active,
                                public ds5_color,
                                public ds5_motion,
                                public ds5_advanced_mode_base
    {
    public:
        rs455_device(std::shared_ptr<context> ctx,
                    const platform::backend_device_group group,
                    bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_active(ctx, group),
              ds5_color(ctx,  group),
              ds5_motion(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())
        {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;
            auto usb_spec = get_usb_spec();
            bool usb3mode = (usb_spec >= platform::usb3_type || usb_spec == platform::usb_undefined);

            uint32_t depth_width  = usb3mode ?      848 : 640;
            uint32_t depth_height = usb3mode ?      480 : 480;
            uint32_t color_width = usb3mode ?       1280 : 640;
            uint32_t color_height = usb3mode ?      720 : 480;
            uint32_t fps    = usb3mode ?            30 :  15;

            tags.push_back({ RS2_STREAM_COLOR, -1, color_width, color_height, RS2_FORMAT_RGB8, fps, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_DEPTH, -1, depth_width, depth_height, RS2_FORMAT_Z16, fps, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_INFRARED, -1, depth_width, depth_height, RS2_FORMAT_Y8, fps, profile_tag::PROFILE_TAG_SUPERSET });
            tags.push_back({RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_63, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });

            return tags;
        }

        bool compress_while_record() const override { return false; }
    };

    std::shared_ptr<device_interface> ds5_info::create(std::shared_ptr<context> ctx,
                                                       bool register_device_notifications) const
    {
        using namespace ds;

        if (_depth.size() == 0) throw std::runtime_error("Depth Camera not found!");
        auto pid = _depth.front().pid;
        platform::backend_device_group group{_depth, _hwm, _hid};

        switch(pid)
        {
        case RS400_PID:
            return std::make_shared<rs400_device>(ctx, group, register_device_notifications);
        case RS405U_PID:
            return std::make_shared<rs405u_device>(ctx, group, register_device_notifications);
        case RS410_PID:
        case RS460_PID:
            return std::make_shared<rs410_device>(ctx, group, register_device_notifications);
        case RS415_PID:
            return std::make_shared<rs415_device>(ctx, group, register_device_notifications);
        case RS416_PID:
            return std::make_shared<rs416_device>(ctx, group, register_device_notifications);
        case RS416_RGB_PID:
            return std::make_shared<rs416_rgb_device>(ctx, group, register_device_notifications);
        case RS420_PID:
            return std::make_shared<rs420_device>(ctx, group, register_device_notifications);
        case RS420_MM_PID:
            return std::make_shared<rs420_mm_device>(ctx, group, register_device_notifications);
        case RS430_PID:
            return std::make_shared<rs430_device>(ctx, group, register_device_notifications);
        case RS430I_PID:
            return std::make_shared<rs430i_device>(ctx, group, register_device_notifications);
        case RS430_MM_PID:
            return std::make_shared<rs430_mm_device>(ctx, group, register_device_notifications);
        case RS430_MM_RGB_PID:
            return std::make_shared<rs430_rgb_mm_device>(ctx, group, register_device_notifications);
        case RS435_RGB_PID:
            return std::make_shared<rs435_device>(ctx, group, register_device_notifications);
        case RS435I_PID:
            return std::make_shared<rs435i_device>(ctx, group, register_device_notifications);
        case RS465_PID:
            return std::make_shared<rs465_device>(ctx, group, register_device_notifications);
        case RS_USB2_PID:
            return std::make_shared<rs410_device>(ctx, group, register_device_notifications);
        case RS400_IMU_PID:
            return std::make_shared<rs400_imu_device>(ctx, group, register_device_notifications);
        case ds::RS405_PID:
            return std::make_shared<rs405_device>(ctx, group, register_device_notifications);
        case ds::RS455_PID:
            return std::make_shared<rs455_device>(ctx, group, register_device_notifications);
        default:
            throw std::runtime_error(to_string() << "Unsupported RS400 model! 0x"
                << std::hex << std::setw(4) << std::setfill('0') <<(int)pid);
        }
    }

    std::vector<std::shared_ptr<device_info>> ds5_info::pick_ds5_devices(
        std::shared_ptr<context> ctx,
        platform::backend_device_group& group)
    {
        std::vector<platform::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto valid_pid = filter_by_product(group.uvc_devices, ds::rs400_sku_pid);
        auto group_devices = group_devices_and_hids_by_unique_id(group_devices_by_unique_id(valid_pid), group.hid_devices);

        for (auto& g : group_devices)
        {
            auto& devices = g.first;
            auto& hids = g.second;

            bool all_sensors_present = mi_present(devices, 0);

            // Device with multi sensors can be enabled only if all sensors (RGB + Depth) are present
            auto is_pid_of_multisensor_device = [](int pid) { return std::find(std::begin(ds::multi_sensors_pid), std::end(ds::multi_sensors_pid), pid) != std::end(ds::multi_sensors_pid); };
            bool is_device_multisensor = false;
            for (auto&& uvc : devices)
            {
                if (is_pid_of_multisensor_device(uvc.pid))
                    is_device_multisensor = true;
            }

            if(is_device_multisensor)
            {
                all_sensors_present = all_sensors_present && mi_present(devices, 3);
            }


#if !defined(__APPLE__) // Not supported by macos
            auto is_pid_of_hid_sensor_device = [](int pid) { return std::find(std::begin(ds::hid_sensors_pid), std::end(ds::hid_sensors_pid), pid) != std::end(ds::hid_sensors_pid); };
            bool is_device_hid_sensor = false;
            for (auto&& uvc : devices)
            {
                if (is_pid_of_hid_sensor_device(uvc.pid))
                    is_device_hid_sensor = true;
            }

            // Device with hids can be enabled only if both hids (gyro and accelerometer) are present
            // Assuming that hids amount of 2 and above guarantee that gyro and accelerometer are present
            if (is_device_hid_sensor)
            {
                all_sensors_present &= (hids.size() >= 2);
            }
#endif

            if (!devices.empty() && all_sensors_present)
            {
                platform::usb_device_info hwm;

                std::vector<platform::usb_device_info> hwm_devices;
                if (ds::try_fetch_usb_device(group.usb_devices, devices.front(), hwm))
                {
                    hwm_devices.push_back(hwm);
                }
                else
                {
                    LOG_DEBUG("try_fetch_usb_device(...) failed.");
                }

                auto info = std::make_shared<ds5_info>(ctx, devices, hwm_devices, hids);
                chosen.insert(chosen.end(), devices.begin(), devices.end());
                results.push_back(info);

            }
            else
            {
                LOG_WARNING("DS5 group_devices is empty.");
            }
        }

        trim_device_list(group.uvc_devices, chosen);

        return results;
    }


    inline std::shared_ptr<matcher> create_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
    {
        return std::make_shared<timestamp_composite_matcher>(matchers);
    }

    std::shared_ptr<matcher> rs400_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get()};
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs405u_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get()};
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs410_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get()};
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs415_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs465_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        // TODO - A proper matcher for High-FPS sensor is required
        std::vector<stream_interface*> mm_streams = { _accel_stream.get(), _gyro_stream.get() };
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs416_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get() };
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs416_rgb_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs420_mm_device::create_matcher(const frame_holder& frame) const
    {
        //TODO: add matcher to mm
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get()};
        std::vector<stream_interface*> mm_streams = {
            _fisheye_stream.get(),
            _accel_stream.get(),
            _gyro_stream.get()
        };
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs420_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get()};
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs430_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get() };
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs430_mm_device::create_matcher(const frame_holder& frame) const
    {
        //TODO: add matcher to mm
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get() };
        std::vector<stream_interface*> mm_streams = {
            _fisheye_stream.get(),
            _accel_stream.get(),
            _gyro_stream.get()
        };
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs435_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs430_rgb_mm_device::create_matcher(const frame_holder& frame) const
    {
        //TODO: add matcher to mm
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        std::vector<stream_interface*> mm_streams = {
            _fisheye_stream.get(),
            _accel_stream.get(),
            _gyro_stream.get()
        };
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs430i_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get() };
        // TODO - A proper matcher for High-FPS sensor is required
        std::vector<stream_interface*> mm_streams = { _accel_stream.get(), _gyro_stream.get() };
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs435i_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        // TODO - A proper matcher for High-FPS sensor is required
        std::vector<stream_interface*> mm_streams = { _accel_stream.get(), _gyro_stream.get()};
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs400_imu_device::create_matcher(const frame_holder& frame) const
    {
        // TODO - A proper matcher for High-FPS sensor is required
        std::vector<stream_interface*> mm_streams = { _accel_stream.get(), _gyro_stream.get()};
        return matcher_factory::create(RS2_MATCHER_DEFAULT, mm_streams);
    }

    std::shared_ptr<matcher> rs405_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        std::vector<stream_interface*> mm_streams = { _accel_stream.get(), _gyro_stream.get()};
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs455_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        std::vector<stream_interface*> mm_streams = { _accel_stream.get(), _gyro_stream.get()};
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }
}
