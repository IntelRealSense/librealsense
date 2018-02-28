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
#include "ds5-rolling-shutter.h"
#include "ds5-active.h"
#include "ds5-color.h"
#include "ds5-motion.h"
#include "sync.h"

namespace librealsense
{
    // PSR
    class rs400_device : public ds5_rolling_shutter, public ds5_advanced_mode_base
    {
    public:
        rs400_device(std::shared_ptr<context> ctx,
                     const platform::backend_device_group& group,
                     bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_rolling_shutter(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;
    };

    // DS5U_S
    class rs405_device : public ds5u_device,
        public ds5_advanced_mode_base
    {
    public:
        rs405_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
            ds5u_device(ctx, group),
            ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;
    };

    // ASR
    class rs410_device : public ds5_rolling_shutter,
                         public ds5_active, public ds5_advanced_mode_base
    {
    public:
        rs410_device(std::shared_ptr<context> ctx,
                     const platform::backend_device_group& group,
                     bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
              ds5_device(ctx, group),
              ds5_rolling_shutter(ctx, group),
              ds5_active(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;
    };

    // ASRC
    class rs415_device : public ds5_rolling_shutter,
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
              ds5_rolling_shutter(ctx, group),
              ds5_active(ctx, group),
              ds5_color(ctx, group),
              ds5_advanced_mode_base(ds5_device::_hw_monitor, get_depth_sensor())  {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;
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

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const;
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
        case RS405_PID:
            return std::make_shared<rs405_device>(ctx, group, register_device_notifications);
        case RS410_PID:
        case RS460_PID:
            return std::make_shared<rs410_device>(ctx, group, register_device_notifications);
        case RS415_PID:
            return std::make_shared<rs415_device>(ctx, group, register_device_notifications);
        case RS420_PID:
            return std::make_shared<rs420_device>(ctx, group, register_device_notifications);
        case RS420_MM_PID:
            return std::make_shared<rs420_mm_device>(ctx, group, register_device_notifications);
        case RS430_PID:
            return std::make_shared<rs430_device>(ctx, group, register_device_notifications);
        case RS430_MM_PID:
            return std::make_shared<rs430_mm_device>(ctx, group, register_device_notifications);
        case RS430_MM_RGB_PID:
            return std::make_shared<rs430_rgb_mm_device>(ctx, group, register_device_notifications);
        case RS435_RGB_PID:
            return std::make_shared<rs435_device>(ctx, group, register_device_notifications);
        case RS_USB2_PID:
            return std::make_shared<rs410_device>(ctx, group, register_device_notifications);
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

            if((devices[0].pid == ds::RS430_MM_PID || devices[0].pid == ds::RS420_MM_PID) &&  hids.size()==0)
                continue;

            bool all_sensors_present = mi_present(devices, 0);

            constexpr std::array<int, 3> multi_sensors = { ds::RS415_PID, ds::RS430_MM_RGB_PID, ds::RS435_RGB_PID };
            auto is_pid_of_multisensor_device = [&multi_sensors](int pid) { return std::find(std::begin(multi_sensors), std::end(multi_sensors), pid) != std::end(multi_sensors); };
            bool is_device_multisensor = false;;
            for (auto&& uvc : devices)
            {
                if (is_pid_of_multisensor_device(uvc.pid))
                    is_device_multisensor = true;
            }

            if(is_device_multisensor)
            {
                all_sensors_present = all_sensors_present && mi_present(devices, 3);
            }

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
        if (frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            return matcher_factory::create(RS2_MATCHER_DLR, streams);
        }
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs405_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get()};

        if (frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            return matcher_factory::create(RS2_MATCHER_DLR, streams);
        }
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs410_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get()};
        if (frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            return matcher_factory::create(RS2_MATCHER_DLR, streams);
        }
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs415_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        if (frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            return matcher_factory::create(RS2_MATCHER_DLR_C, streams);
        }
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs420_mm_device::create_matcher(const frame_holder& frame) const
    {
        //TODO: add matcher to mm
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get()};
        std::vector<stream_interface*> mm_streams = { _fisheye_stream.get(),
        _accel_stream.get(),
        _gyro_stream.get()};

        if (frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            return create_composite_matcher({ matcher_factory::create(RS2_MATCHER_DLR, streams),
                matcher_factory::create(RS2_MATCHER_DEFAULT, mm_streams) });
        }
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs420_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get()};
        if (frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            return matcher_factory::create(RS2_MATCHER_DLR, streams);
        }
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs430_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get() };
        if (frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            return matcher_factory::create(RS2_MATCHER_DLR, streams);
        }
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs430_mm_device::create_matcher(const frame_holder& frame) const
    {
        //TODO: add matcher to mm
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get() };
        std::vector<stream_interface*> mm_streams = { _fisheye_stream.get(),
            _accel_stream.get(),
            _gyro_stream.get() };

        if (!frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            return create_composite_matcher({ matcher_factory::create(RS2_MATCHER_DLR, streams),
                matcher_factory::create(RS2_MATCHER_DEFAULT, mm_streams) });
        }
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs435_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        if (frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            return matcher_factory::create(RS2_MATCHER_DLR_C, streams);
        }
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs430_rgb_mm_device::create_matcher(const frame_holder& frame) const
    {
        //TODO: add matcher to mm
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        std::vector<stream_interface*> mm_streams = { _fisheye_stream.get(),
            _accel_stream.get(),
            _gyro_stream.get()};

        if (frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            return create_composite_matcher({ matcher_factory::create(RS2_MATCHER_DLR_C, streams),
                matcher_factory::create(RS2_MATCHER_DEFAULT, mm_streams) });
        }
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }
}
