// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "ds5.h"

namespace rsimpl
{
    std::shared_ptr<rsimpl::device> ds5_info::create(const uvc::backend& backend) const
    {
        return std::make_shared<ds5_camera>(backend, _depth, _hwm, _hid);
    }

    // DS5 has 2 pins of video streaming
    ds5_info::ds5_info(std::vector<uvc::uvc_device_info> depth, uvc::usb_device_info hwm, std::vector<uvc::hid_device_info> hid)
        : _depth(std::move(depth)),
          _hwm(std::move(hwm)),
          _hid(std::move(hid))
    {
    }

    std::vector<std::shared_ptr<device_info>> pick_ds5_devices(
        std::vector<uvc::uvc_device_info>& uvc,
        std::vector<uvc::usb_device_info>& usb,
        std::vector<uvc::hid_device_info>& hid)
    {
        std::vector<uvc::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto valid_pid = filter_by_product(uvc, rs4xx_sku_pid);
        auto group_devices = group_devices_and_hids_by_unique_id(group_devices_by_unique_id(valid_pid), hid);
        for (auto& group : group_devices)
        {
            auto& devices = group.first;
            auto& hids = group.second;
            if (!devices.empty() &&
                mi_present(devices, 0))
            {
                uvc::usb_device_info hwm;

                if (ds::try_fetch_usb_device(usb, devices.front(), hwm))
                {
                    auto info = std::make_shared<ds5_info>(devices, hwm, hids);
                    chosen.insert(chosen.end(), devices.begin(), devices.end());
                    results.push_back(info);
                }
                else
                {
                    LOG_WARNING("try_fetch_usb_device(...) failed.");
                }
            }
            else
            {
                LOG_WARNING("DS5 group_devices is empty.");
            }
        }

        trim_device_list(uvc, chosen);

        return results;
    }

    std::vector<uint8_t> ds5_camera::send_receive_raw_data(const std::vector<uint8_t>& input)
    {
        return _hw_monitor->send(input);
    }

    rs_intrinsics ds5_camera::get_intrinsics(int subdevice, stream_profile profile) const
    {
        if (subdevice >= get_endpoints_count())
            throw invalid_value_exception(to_string() << "Requested subdevice " <<
                                          subdevice << " is unsupported.");

        if (subdevice == _depth_device_idx)
        {
            return get_intrinsic_by_resolution(
                *_coefficients_table_raw,
                ds::calibration_table_id::coefficients_table_id,
                profile.width, profile.height);
        }

        throw not_implemented_exception("Not Implemented");
    }

    bool ds5_camera::is_camera_in_advanced_mode() const
    {
        command cmd(ds::UAMG);
        assert(_hw_monitor);
        auto ret = _hw_monitor->send(cmd);
        if (ret.empty())
            throw invalid_value_exception("command result is empty!");

        return bool(ret.front());
    }

    std::vector<uint8_t> ds5_camera::get_raw_calibration_table(ds::calibration_table_id table_id) const
    {
        command cmd(ds::GETINTCAL, table_id);
        return _hw_monitor->send(cmd);
    }

    std::shared_ptr<hid_endpoint> ds5_camera::create_hid_device(const uvc::backend& backend,
                                                                const std::vector<uvc::hid_device_info>& all_hid_infos)
    {
        // TODO: implement multiple hid devices
        assert(!all_hid_infos.empty());
        auto hid_ep = std::make_shared<hid_endpoint>(backend.create_hid_device(all_hid_infos[0]));
        return hid_ep;
    }

    std::shared_ptr<uvc_endpoint> ds5_camera::create_depth_device(const uvc::backend& backend,
                                                      const std::vector<uvc::uvc_device_info>& all_device_infos)
    {
        using namespace ds;

        std::vector<std::shared_ptr<uvc::uvc_device>> depth_devices;
        for (auto&& info : filter_by_mi(all_device_infos, 0)) // Filter just mi=0, DEPTH
            depth_devices.push_back(backend.create_uvc_device(info));

        auto depth_ep = std::make_shared<uvc_endpoint>(std::make_shared<uvc::multi_pins_uvc_device>(depth_devices),
                                                       std::unique_ptr<frame_timestamp_reader>(new ds5_timestamp_reader()));
        depth_ep->register_xu(depth_xu); // make sure the XU is initialized everytime we power the camera
        depth_ep->register_pixel_format(pf_z16); // Depth
        depth_ep->register_pixel_format(pf_y8); // Left Only - Luminance
        depth_ep->register_pixel_format(pf_yuyv); // Left Only
        depth_ep->register_pixel_format(pf_uyvyl); // Color from Depth

        depth_ep->register_pu(RS_OPTION_GAIN);
        depth_ep->register_pu(RS_OPTION_ENABLE_AUTO_EXPOSURE);

        depth_ep->register_option(RS_OPTION_EXPOSURE,
            std::make_shared<uvc_xu_option<uint16_t>>(*depth_ep,
                depth_xu,
                DS5_EXPOSURE, "DS5 Exposure")); // TODO: Update description

        // TODO: These if conditions will be implemented as inheritance classes
        auto pid = all_device_infos.front().pid;
        if (pid == RS410A_PID || pid == RS450T_PID || pid == RS430C_PID)
        {
            depth_ep->register_option(RS_OPTION_EMITTER_ENABLED, std::make_shared<emitter_option>(*depth_ep));

            depth_ep->register_option(RS_OPTION_LASER_POWER,
                std::make_shared<uvc_xu_option<uint16_t>>(*depth_ep,
                    depth_xu,
                    DS5_LASER_POWER, "Manual laser power in mw. applicable only when laser power mode is set to Manual"));
        }

        depth_ep->set_pose({ { { 1,0,0 },{ 0,1,0 },{ 0,0,1 } },{ 0,0,0 } });

        return depth_ep;
    }

    ds5_camera::ds5_camera(const uvc::backend& backend,
                           const std::vector<uvc::uvc_device_info>& dev_info,
                           const uvc::usb_device_info& hwm_device,
                           const std::vector<uvc::hid_device_info>& hid_info)
        : _depth_device_idx(add_endpoint(create_depth_device(backend, dev_info))),
          _hw_monitor(std::make_shared<hw_monitor>(std::make_shared<locked_transfer>(backend.create_usb_device(hwm_device), get_depth_endpoint())))
    {
        using namespace ds;

        _coefficients_table_raw = [this]() { return get_raw_calibration_table(coefficients_table_id); };

        static const char* device_name = "Intel RealSense DS5";
        auto fw_version = _hw_monitor->get_firmware_version_string(GVD, fw_version_offset);
        auto serial = _hw_monitor->get_module_serial_string(GVD, module_serial_offset);

        auto& depth_ep = get_depth_endpoint();
        depth_ep.register_option(RS_OPTION_ENABLE_FW_LOGGER,
            std::make_shared<fw_logger_option>(_hw_monitor, ds::fw_cmd::GLD, 100, "DS5 FW Logger"));
        if (is_camera_in_advanced_mode())
        {
            depth_ep.register_pixel_format(pf_y8i); // L+R
            depth_ep.register_pixel_format(pf_y12i); // L+R - Calibration not rectified
        }

        // TODO: These if conditions will be implemented as inheritance classes
        auto pid = dev_info.front().pid;

        std::shared_ptr<uvc_endpoint> fisheye_ep;
        int fe_index;
        if (pid == RS450T_PID)
        {
            auto fisheye_infos = filter_by_mi(dev_info, 3);
            if (fisheye_infos.size() != 1)
                throw invalid_value_exception("RS450 model is expected to include a single fish-eye device!");

            fisheye_ep = std::make_shared<uvc_endpoint>(backend.create_uvc_device(fisheye_infos.front()),
                                                        std::unique_ptr<frame_timestamp_reader>(new ds5_timestamp_reader()));
            fisheye_ep->register_xu(fisheye_xu); // make sure the XU is initialized everytime we power the camera
            fisheye_ep->register_pixel_format(pf_raw8);
            fisheye_ep->register_pixel_format(pf_fe_raw8_unpatched_kernel); // W/O for unpatched kernel
            fisheye_ep->register_pu(RS_OPTION_GAIN);
            fisheye_ep->register_option(RS_OPTION_EXPOSURE,
                std::make_shared<uvc_xu_option<uint16_t>>(*fisheye_ep,
                    fisheye_xu,
                    FISHEYE_EXPOSURE, "Fisheye Exposure")); // TODO: Update description

            // Add fisheye endpoint
            fe_index = add_endpoint(fisheye_ep);
            fisheye_ep->set_pose({ { { 1,0,0 },{ 0,1,0 },{ 0,0,1 } },{ 0,0,0 } });

            // Add hid endpoint
            auto hid_index = add_endpoint(create_hid_device(backend, hid_info));
            std::map<rs_camera_info, std::string> camera_info = {{RS_CAMERA_INFO_DEVICE_NAME, device_name},
                                                                 {RS_CAMERA_INFO_MODULE_NAME, "Motion Module"},
                                                                 {RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER, serial},
                                                                 {RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION, fw_version},
                                                                 {RS_CAMERA_INFO_DEVICE_LOCATION, hid_info.front().device_path}};
            register_endpoint_info(hid_index, camera_info);
        }

        set_depth_scale(0.001f); // TODO: find out what is the right value

        // Register endpoint info
        for(auto& element : dev_info)
        {
            if (element.mi == 0) // mi 0 is relate to DS5 device
            {
                std::map<rs_camera_info, std::string> camera_info = {{RS_CAMERA_INFO_DEVICE_NAME, device_name},
                                                                     {RS_CAMERA_INFO_MODULE_NAME, "Stereo Module"},
                                                                     {RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER, serial},
                                                                     {RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION, fw_version},
                                                                     {RS_CAMERA_INFO_DEVICE_LOCATION, element.device_path}};
                register_endpoint_info(_depth_device_idx, camera_info);
            }
            else if (fisheye_ep && element.pid == RS450T_PID && element.mi == 3) // mi 3 is relate to Fisheye device
            {
                std::map<rs_camera_info, std::string> camera_info = {{RS_CAMERA_INFO_DEVICE_NAME, device_name},
                                                                     {RS_CAMERA_INFO_MODULE_NAME, "Fisheye Camera"},
                                                                     {RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER, serial},
                                                                     {RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION, fw_version},
                                                                     {RS_CAMERA_INFO_DEVICE_LOCATION, element.device_path}};
                register_endpoint_info(fe_index, camera_info);
            }
        }
    }
}
