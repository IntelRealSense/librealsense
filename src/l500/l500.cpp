// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include <vector>
#include "device.h"
#include "context.h"
#include "stream.h"
#include "l500.h"
#include "l500-private.h"

#define MM_TO_METER 1/1000

namespace librealsense
{
    std::shared_ptr<device_interface> l500_info::create(std::shared_ptr<context> ctx,
        bool register_device_notifications) const
    {
        return std::make_shared<l500_device>(ctx,
                                             this->get_device_data(),
                                             register_device_notifications);
    }

    std::vector<std::shared_ptr<device_info>> l500_info::pick_l500_devices(
        std::shared_ptr<context> ctx,
        std::vector<platform::uvc_device_info>& uvc,
        std::vector<platform::usb_device_info>& usb)
    {
        std::vector<platform::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto correct_pid = filter_by_product(uvc, { L500_PID });
        auto group_devices = group_devices_by_unique_id(correct_pid);
        for (auto& group : group_devices)
        {
            if (!group.empty() && mi_present(group, 0))
            {
                auto depth = get_mi(group, 0);
                platform::usb_device_info hwm;

                if (ivcam2::try_fetch_usb_device(usb, depth, hwm))
                {
                    auto info = std::make_shared<l500_info>(ctx, group, hwm);
                    chosen.push_back(depth);
                    results.push_back(info);
                }
                else
                {
                    LOG_WARNING("try_fetch_usb_device(...) failed.");
                }
            }
            else
            {
                LOG_WARNING("L500 group_devices is empty.");
            }
        }

        trim_device_list(uvc, chosen);

        return results;
    }

    std::vector<uint8_t> l500_device::get_raw_calibration_table() const
    {
       /* command cmd(ivcam2::fw_cmd::DPT_INTRINSICS_GET);
        auto res = _hw_monitor->send(cmd);*/

        //WA untill fw will fix DPT_INTRINSICS_GET command
        command cmd_fx(0x01, 0xa00e0804, 0xa00e0808);
        command cmd_fy(0x01, 0xa00e080c, 0xa00e0810);
        command cmd_cx(0x01, 0xa00e0814, 0xa00e0818);
        command cmd_cy(0x01, 0xa00e0818, 0xa00e081c);
        auto fx = _hw_monitor->send(cmd_fx); // CBUFspare_000
        auto fy = _hw_monitor->send(cmd_fy); // CBUFspare_002
        auto cx = _hw_monitor->send(cmd_cx); // CBUFspare_004
        auto cy = _hw_monitor->send(cmd_cy); // CBUFspare_005

        std::vector<uint8_t> vec;
        vec.insert(vec.end(), fx.begin(), fx.end());
        vec.insert(vec.end(), cx.begin(), cx.end());
        vec.insert(vec.end(), fy.begin(), fy.end());
        vec.insert(vec.end(), cy.begin(), cy.end());

        return vec;
    }

    l500_device::l500_device(std::shared_ptr<context> ctx,
                             const platform::backend_device_group& group,
                             bool register_device_notifications)
        : device(ctx, group, register_device_notifications),
        _depth_device_idx(add_sensor(create_depth_device(ctx, group.uvc_devices))),
        _depth_stream(new stream(RS2_STREAM_DEPTH)),
        _ir_stream(new stream(RS2_STREAM_INFRARED)),
        _confidence_stream(new stream(RS2_STREAM_CONFIDENCE))
    {
        _calib_table_raw = [this]() { return get_raw_calibration_table(); };

        static const auto device_name = "Intel RealSense L500";

        using namespace ivcam2;

        auto&& backend = ctx->get_backend();

#ifdef HWM_OVER_XU
        _hw_monitor = std::make_shared<hw_monitor>(
                    std::make_shared<locked_transfer>(std::make_shared<command_transfer_over_xu>(
                                                      get_depth_sensor(), depth_xu, L500_HWMONITOR),
                                                      get_depth_sensor()));
#else
        _hw_monitor = std::make_shared<hw_monitor>(
                    std::make_shared<locked_transfer>(backend.create_usb_device(group.usb_devices.front()),
                                                                                get_depth_sensor()));
#endif
        *_calib_table_raw;  //work around to bug on fw
        auto fw_version = _hw_monitor->get_firmware_version_string(GVD, fw_version_offset);
        auto serial = _hw_monitor->get_module_serial_string(GVD, module_serial_offset, module_serial_size);

        auto pid = group.uvc_devices.front().pid;
        auto pid_hex_str = hexify(pid >> 8) + hexify(static_cast<uint8_t>(pid));

        register_info(RS2_CAMERA_INFO_NAME, device_name);
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, serial);
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, fw_version);
        register_info(RS2_CAMERA_INFO_DEBUG_OP_CODE, std::to_string(static_cast<int>(fw_cmd::GLD)));
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, group.uvc_devices.front().device_path);
        register_info(RS2_CAMERA_INFO_PRODUCT_ID, pid_hex_str);

       
        get_depth_sensor().register_option(RS2_OPTION_DEPTH_UNITS, std::make_shared<const_value_option>("Number of meters represented by a single depth unit",
            lazy<float>([&]() {
            return read_znorm(); })));


        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_ir_stream);
        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_confidence_stream);

        register_stream_to_extrinsic_group(*_depth_stream, 0);
        register_stream_to_extrinsic_group(*_ir_stream, 0);
    }

    void l500_device::create_snapshot(std::shared_ptr<debug_interface>& snapshot) const
    {
        throw not_implemented_exception("create_snapshot(...) not implemented!");
    }

    void l500_device::enable_recording(std::function<void(const debug_interface&)> record_action)
    {
        throw not_implemented_exception("enable_recording(...) not implemented!");
    }

    std::vector<tagged_profile> l500_device::get_profiles_tags() const
    {
        std::vector<tagged_profile> tags;

        tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 360, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
        tags.push_back({ RS2_STREAM_INFRARED, -1, 640, 360, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
        tags.push_back({ RS2_STREAM_CONFIDENCE, -1, 640, 360, RS2_FORMAT_RAW8, 30, profile_tag::PROFILE_TAG_SUPERSET });
        
        return tags;
    };

    void l500_device::force_hardware_reset() const
    {
        command cmd(ivcam2::fw_cmd::HWReset);
        cmd.require_response = false;
        _hw_monitor->send(cmd);
    }

    std::shared_ptr<matcher> l500_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<std::shared_ptr<matcher>> depth_matchers;

        std::vector<stream_interface*> streams = { _depth_stream.get(), _ir_stream.get(), _confidence_stream.get() };

        for (auto& s : streams)
        {
            depth_matchers.push_back(std::make_shared<identity_matcher>(s->get_unique_id(), s->get_stream_type()));
        }
        std::vector<std::shared_ptr<matcher>> matchers;
        if (!frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            matchers.push_back(std::make_shared<timestamp_composite_matcher>(depth_matchers));
        }
        else
        {
            matchers.push_back(std::make_shared<frame_number_composite_matcher>(depth_matchers));
        }

        return std::make_shared<timestamp_composite_matcher>(matchers);
    }

    bool l500_device::try_read_zo_point_x(int* zo_x)
    {
        if (auto ver = read_algo_version() >= 115)
        {
            int x, y;
            read_zo_point(&x, &y);
            *zo_x = x;
            return true;
        }
        else
        {
            LOG_WARNING("Could not read zo point values the algo version is " << ver);
            return false;
        }
    }

    bool l500_device::try_read_zo_point_y(int * zo_y)
    {
        if (auto ver = read_algo_version() >= 115)
        {
            int x, y;
            read_zo_point(&x, &y);
            *zo_y = y;
            return true;
        }
        else
        {
            LOG_WARNING("Could not read zo point values the algo version is " << ver);
            return false;
        }
    }

    void l500_device::read_zo_point(int* zo_x, int* zo_y)
    {
        command cmd(1, 0xa00e1b8c, 0xa00e1b90);
        auto res = _hw_monitor->send(cmd);
        auto data = (uint16_t*)res.data();
        *zo_x = data[0];
        *zo_y = data[1];
    }

    int l500_device::read_algo_version()
    {
        command cmd(1, 0xa0020bd8, 0xa0020bdc);
        auto res = _hw_monitor->send(cmd);
        auto data = (uint8_t*)res.data();
        auto ver = data[0] + 100* data[1];
        return ver;
    }

    float l500_device::read_baseline()
    {
        command cmd(1, 0xa00e0868, 0xa00e086c);
        auto res = _hw_monitor->send(cmd);
        auto data = (float*)res.data();
        return *data;
    }

    float l500_device::read_znorm()
    {
        auto res = _hw_monitor->send(command(0x01, 0xa00e0b08, 0xa00e0b0c));
        auto znorm = *(float*)res.data();
        return 1/znorm* MM_TO_METER;
    }

    rs2_time_t l500_timestamp_reader_from_metadata::get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        if (has_metadata_ts(fo))
        {
            auto md = (librealsense::metadata_raw*)(fo.metadata);
            return (double)(ts_wrap.calc(md->header.timestamp))*0.0001;
        }
        else
        {
            if (!one_time_note)
            {
                LOG_WARNING("UVC metadata payloads are not available for stream "
                    << std::hex << mode.pf->fourcc << std::dec << (mode.profile.format)
                    << ". Please refer to installation chapter for details.");
                one_time_note = true;
            }
            return _backup_timestamp_reader->get_frame_timestamp(mode, fo);
        }
    }

    unsigned long long l500_timestamp_reader_from_metadata::get_frame_counter(const request_mapping & mode, const platform::frame_object& fo) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        if (has_metadata_fc(fo))
        {
            auto md = (byte*)(fo.metadata);
            return ((int*)md)[7];
        }

        return _backup_timestamp_reader->get_frame_counter(mode, fo);
    }

    void l500_timestamp_reader_from_metadata::reset()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        one_time_note = false;
        _backup_timestamp_reader->reset();
        ts_wrap.reset();
    }

    rs2_timestamp_domain l500_timestamp_reader_from_metadata::get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        return (has_metadata_ts(fo)) ? RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK : _backup_timestamp_reader->get_frame_timestamp_domain(mode, fo);
    }

    float zo_point_option_x::query() const
    {
        if (_value == 0)
        {
            _owner->try_read_zo_point_x(&_value);

        }
        return _value;
    }

    float zo_point_option_y::query() const
    {
        if (_value == 0)
        {
            _owner->try_read_zo_point_y(&_value);

        }
        return _value;
    }
}
