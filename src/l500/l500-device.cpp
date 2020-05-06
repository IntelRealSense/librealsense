// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-device.h"

#include <vector>

#include "context.h"
#include "stream.h"
#include "image.h"

#include "l500-depth.h"
#include "l500-private.h"

#include "proc/decimation-filter.h"
#include "proc/threshold.h" 
#include "proc/spatial-filter.h"
#include "proc/temporal-filter.h"
#include "proc/hole-filling-filter.h"
#include "proc/zero-order.h"
#include "proc/syncer-processing-block.h"
#include "proc/rotation-transform.h"
#include "fw-update/fw-update-unsigned.h"

namespace librealsense
{
    std::map<uint32_t, rs2_format> l500_depth_fourcc_to_rs2_format = {
        { rs_fourcc('G','R','E','Y'), RS2_FORMAT_Y8 },
        { rs_fourcc('Z','1','6',' '), RS2_FORMAT_Z16 },
        { rs_fourcc('C',' ',' ',' '), RS2_FORMAT_RAW8 },
    };

    std::map<uint32_t, rs2_stream> l500_depth_fourcc_to_rs2_stream = {
        { rs_fourcc('G','R','E','Y'), RS2_STREAM_INFRARED },
        { rs_fourcc('Z','1','6',' '), RS2_STREAM_DEPTH },
        { rs_fourcc('C',' ',' ',' '), RS2_STREAM_CONFIDENCE }
    };

    using namespace ivcam2;

    l500_device::l500_device(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        :device(ctx, group), global_time_interface(), 
        _depth_stream(new stream(RS2_STREAM_DEPTH)),
        _ir_stream(new stream(RS2_STREAM_INFRARED)),
        _confidence_stream(new stream(RS2_STREAM_CONFIDENCE))
    {
        _depth_device_idx = add_sensor(create_depth_device(ctx, group.uvc_devices));
        auto pid = group.uvc_devices.front().pid;
        std::string device_name = (rs500_sku_names.end() != rs500_sku_names.find(pid)) ? rs500_sku_names.at(pid) : "RS5xx";

        using namespace ivcam2;

        auto&& backend = ctx->get_backend();

        auto& depth_sensor = get_depth_sensor();
        auto& raw_depth_sensor = get_raw_depth_sensor();

        if (group.usb_devices.size() > 0)
        {
            _hw_monitor = std::make_shared<hw_monitor>(
                std::make_shared<locked_transfer>(backend.create_usb_device(group.usb_devices.front()),
                    raw_depth_sensor));
        }
        else
        {
            _hw_monitor = std::make_shared<hw_monitor>(
                std::make_shared<locked_transfer>(std::make_shared<command_transfer_over_xu>(
                    raw_depth_sensor, depth_xu, L500_HWMONITOR),
                    raw_depth_sensor));
        }

#ifdef HWM_OVER_XU
        if (group.usb_devices.size() > 0)
        {
            _hw_monitor = std::make_shared<hw_monitor>(
                std::make_shared<locked_transfer>(std::make_shared<command_transfer_over_xu>(
                    raw_depth_sensor, depth_xu, L500_HWMONITOR),
                    raw_depth_sensor));
        }
#endif

        std::vector<uint8_t> gvd_buff(HW_MONITOR_BUFFER_SIZE);
        _hw_monitor->get_gvd(gvd_buff.size(), gvd_buff.data(), GVD);
        // fooling tests recordings - don't remove
        _hw_monitor->get_gvd(gvd_buff.size(), gvd_buff.data(), GVD);

        auto optic_serial = _hw_monitor->get_module_serial_string(gvd_buff, module_serial_offset, module_serial_size);
        auto asic_serial = _hw_monitor->get_module_serial_string(gvd_buff, module_asic_serial_offset, module_serial_size);
        auto fwv = _hw_monitor->get_firmware_version_string(gvd_buff, fw_version_offset);
        _fw_version = firmware_version(fwv);

        _is_locked = _hw_monitor->get_gvd_field<bool>(gvd_buff, is_camera_locked_offset);

        // TODO: flash lock is not suuported yet.
        // reporting lock to the application blocks flash FW update.
        // remove when flash update support is required.
        _is_locked = true; 

        auto pid_hex_str = hexify(group.uvc_devices.front().pid);

        using namespace platform;

        auto usb_mode = raw_depth_sensor.get_usb_specification();
        if (usb_spec_names.count(usb_mode) && (usb_undefined != usb_mode))
        {
            auto usb_type_str = usb_spec_names.at(usb_mode);
            register_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, usb_type_str);
        }

        register_info(RS2_CAMERA_INFO_NAME, device_name);
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, optic_serial);
        register_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER, asic_serial);
        register_info(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID, asic_serial);
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, _fw_version);
        register_info(RS2_CAMERA_INFO_DEBUG_OP_CODE, std::to_string(static_cast<int>(fw_cmd::GLD)));
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, group.uvc_devices.front().device_path);
        register_info(RS2_CAMERA_INFO_PRODUCT_ID, pid_hex_str);
        register_info(RS2_CAMERA_INFO_PRODUCT_LINE, "L500");
        register_info(RS2_CAMERA_INFO_CAMERA_LOCKED, _is_locked ? "YES" : "NO");

        std::shared_ptr< freefall_option > freefall_opt;
        if( _fw_version >= firmware_version( "1.3.5.0" ) )
        {
            depth_sensor.register_option(
                RS2_OPTION_FREEFALL_DETECTION_ENABLED,
                freefall_opt = std::make_shared< freefall_option >( *_hw_monitor )
            );
        }
        else
        {
            LOG_DEBUG( "Skipping Freefall control: requires FW 1.3.5" );
        }
        if( _fw_version >= firmware_version( "1.3.12.9" ) )
        {
            depth_sensor.register_option(
                RS2_OPTION_INTER_CAM_SYNC_MODE,
                std::make_shared< hw_sync_option >( *_hw_monitor, freefall_opt )
            );
        }
        else
        {
            LOG_DEBUG( "Skipping HW Sync control: requires FW 1.3.12.9" );
        }
    }

    std::shared_ptr<synthetic_sensor> l500_device::create_depth_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& all_device_infos)
    {
        auto&& backend = ctx->get_backend();

        std::vector<std::shared_ptr<platform::uvc_device>> depth_devices;
        for (auto&& info : filter_by_mi(all_device_infos, 0)) // Filter just mi=0, DEPTH
            depth_devices.push_back(backend.create_uvc_device(info));

        std::unique_ptr<frame_timestamp_reader> timestamp_reader_metadata(new l500_timestamp_reader_from_metadata(backend.create_time_service()));
        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto raw_depth_ep = std::make_shared<uvc_sensor>("Raw Depth Sensor", std::make_shared<platform::multi_pins_uvc_device>(depth_devices),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(timestamp_reader_metadata), _tf_keeper, enable_global_time_option)), this);
        raw_depth_ep->register_xu(depth_xu);

        auto depth_ep = std::make_shared<l500_depth_sensor>(this, raw_depth_ep, l500_depth_fourcc_to_rs2_format, l500_depth_fourcc_to_rs2_stream);
        
        depth_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);
        depth_ep->get_option(RS2_OPTION_GLOBAL_TIME_ENABLED).set(0);

        depth_ep->register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, filter_by_mi(all_device_infos, 0).front().device_path);
       
        auto is_zo_enabled_opt = std::make_shared<bool_option>();
        auto weak_is_zo_enabled_opt = std::weak_ptr<bool_option>(is_zo_enabled_opt);
        is_zo_enabled_opt->set(false);
        depth_ep->register_option(RS2_OPTION_ZERO_ORDER_ENABLED, is_zo_enabled_opt);

        depth_ep->register_processing_block(
            { {RS2_FORMAT_Z16}, {RS2_FORMAT_Y8} },
            { {RS2_FORMAT_Z16, RS2_STREAM_DEPTH, 0, 0, 0, 0, &rotate_resolution} },
            [weak_is_zo_enabled_opt]() {
                auto is_zo_enabled_opt = weak_is_zo_enabled_opt.lock();
                auto z16rot = std::make_shared<rotation_transform>(RS2_FORMAT_Z16, RS2_STREAM_DEPTH, RS2_EXTENSION_DEPTH_FRAME);
                auto y8rot = std::make_shared<rotation_transform>(RS2_FORMAT_Y8, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME);
                auto sync = std::make_shared<syncer_process_unit>(is_zo_enabled_opt);
                auto zo = std::make_shared<zero_order>(is_zo_enabled_opt);

                auto cpb = std::make_shared<composite_processing_block>();
                cpb->add(z16rot);
                cpb->add(y8rot);
                cpb->add(sync);
                cpb->add(zo);

                return cpb;
            }
        );

        depth_ep->register_processing_block(
            { {RS2_FORMAT_Z16}, {RS2_FORMAT_Y8} },
            { {RS2_FORMAT_Z16, RS2_STREAM_DEPTH} },
            [weak_is_zo_enabled_opt]() {
                auto is_zo_enabled_opt = weak_is_zo_enabled_opt.lock();
                auto z16rot = std::make_shared<identity_processing_block>();
                auto y8rot = std::make_shared<identity_processing_block>();
                auto sync = std::make_shared<syncer_process_unit>(is_zo_enabled_opt);
                auto zo = std::make_shared<zero_order>(is_zo_enabled_opt);

                auto cpb = std::make_shared<composite_processing_block>();
                cpb->add(z16rot);
                cpb->add(y8rot);
                cpb->add(sync);
                cpb->add(zo);

                return cpb;
            }
        );

        depth_ep->register_processing_block(
            { {RS2_FORMAT_Z16}, {RS2_FORMAT_Y8}, {RS2_FORMAT_RAW8} },
            {
                {RS2_FORMAT_Z16, RS2_STREAM_DEPTH, 0, 0, 0, 0, &rotate_resolution},
                {RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, 0, 0, 0, 0, &l500_confidence_resolution}
            },
            [weak_is_zo_enabled_opt]() {
                auto is_zo_enabled_opt = weak_is_zo_enabled_opt.lock();
                auto z16rot = std::make_shared<rotation_transform>(RS2_FORMAT_Z16, RS2_STREAM_DEPTH, RS2_EXTENSION_DEPTH_FRAME);
                auto y8rot = std::make_shared<rotation_transform>(RS2_FORMAT_Y8, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME);
                auto conf = std::make_shared<confidence_rotation_transform>();
                auto sync = std::make_shared<syncer_process_unit>(is_zo_enabled_opt);
                auto zo = std::make_shared<zero_order>(is_zo_enabled_opt);

                auto cpb = std::make_shared<composite_processing_block>();
                cpb->add(z16rot);
                cpb->add(y8rot);
                cpb->add(conf);
                cpb->add(sync);
                cpb->add(zo);

                return cpb;
            }
        );

        depth_ep->register_processing_block(
            { {RS2_FORMAT_Y8} },
            { {RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 0, 0, 0, 0, &rotate_resolution} },
            []() { return std::make_shared<rotation_transform>(RS2_FORMAT_Y8, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME); }
        );

        depth_ep->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_Y8, RS2_STREAM_INFRARED));

        depth_ep->register_processing_block(
            { {RS2_FORMAT_RAW8} },
            { {RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, 0, 0, 0, 0, &l500_confidence_resolution} },
            []() { return std::make_shared<confidence_rotation_transform>(); }
        );

        depth_ep->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE));

        return depth_ep;
    }

    void l500_device::force_hardware_reset() const
    {
        command cmd(ivcam2::fw_cmd::HW_RESET);
        cmd.require_response = false;
        _hw_monitor->send(cmd);
    }

    void l500_device::create_snapshot(std::shared_ptr<debug_interface>& snapshot) const
    {
        throw not_implemented_exception("create_snapshot(...) not implemented!");
    }

    void l500_device::enable_recording(std::function<void(const debug_interface&)> record_action)
    {
        throw not_implemented_exception("enable_recording(...) not implemented!");
    }

    double l500_device::get_device_time_ms()
    {
        if (dynamic_cast<const platform::playback_backend*>(&(get_context()->get_backend())) != nullptr)
        {
            throw not_implemented_exception("device time not supported for backend.");
        }

        if (!_hw_monitor)
            throw wrong_api_call_sequence_exception("_hw_monitor is not initialized yet");

        command cmd(ivcam2::fw_cmd::MRD, ivcam2::REGISTER_CLOCK_0, ivcam2::REGISTER_CLOCK_0 + 4);
        auto res = _hw_monitor->send(cmd);

        if (res.size() < sizeof(uint32_t))
        {
            LOG_DEBUG("size(res):" << res.size());
            throw std::runtime_error("Not enough bytes returned from the firmware!");
        }
        uint32_t dt = *(uint32_t*)res.data();
        double ts = dt * TIMESTAMP_USEC_TO_MSEC;
        return ts;
    }

    void l500_device::enter_update_state() const
    {
        try {
            LOG_INFO("entering to update state, device disconnect is expected");
            command cmd(ivcam2::DFU);
            cmd.param1 = 1;
            _hw_monitor->send(cmd);
        }
        catch (...) {
            // The set command returns a failure because switching to DFU resets the device while the command is running.
        }
    }

    std::vector<uint8_t> l500_device::backup_flash(update_progress_callback_ptr callback)
    {
        int flash_size = 1024 * 2048;
        int max_bulk_size = 1016;
        int max_iterations = int(flash_size / max_bulk_size + 1);

        std::vector<uint8_t> flash;
        flash.reserve(flash_size);

        get_raw_depth_sensor().invoke_powered([&](platform::uvc_device& dev)
        {
            for (int i = 0; i < max_iterations; i++)
            {
                int offset = max_bulk_size * i;
                int size = max_bulk_size;
                if (i == max_iterations - 1)
                {
                    size = flash_size - offset;
                }

                bool appended = false;

                const int retries = 3;
                for (int j = 0; j < retries && !appended; j++)
                {
                    try
                    {
                        command cmd(ivcam2::FRB);
                        cmd.param1 = offset;
                        cmd.param2 = size;
                        auto res = _hw_monitor->send(cmd);

                        flash.insert(flash.end(), res.begin(), res.end());
                        appended = true;
                    }
                    catch (...)
                    {
                        if (i < retries - 1) std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        else throw;
                    }
                }

                if (callback) callback->on_update_progress((float)i / max_iterations);
            }
            if (callback) callback->on_update_progress(1.0);
        });

        return flash;
    }

    void l500_device::update_flash_section(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& image, uint32_t offset, uint32_t size, update_progress_callback_ptr callback, float continue_from, float ratio)
    {
        size_t sector_count = size / ivcam2::FLASH_SECTOR_SIZE;
        size_t first_sector = offset / ivcam2::FLASH_SECTOR_SIZE;

        if (sector_count * ivcam2::FLASH_SECTOR_SIZE != size)
            sector_count++;

        sector_count += first_sector;

        for (size_t sector_index = first_sector; sector_index < sector_count; sector_index++)
        {
            command cmdFES(ivcam2::FES);
            cmdFES.require_response = false;
            cmdFES.param1 = sector_index;
            cmdFES.param2 = 1;
            auto res = hwm->send(cmdFES);

            for (int i = 0; i < ivcam2::FLASH_SECTOR_SIZE; )
            {
                auto index = sector_index * ivcam2::FLASH_SECTOR_SIZE + i;
                if (index >= offset + size)
                    break;
                int packet_size = std::min((int)(HW_MONITOR_COMMAND_SIZE - (i % HW_MONITOR_COMMAND_SIZE)), (int)(ivcam2::FLASH_SECTOR_SIZE - i));
                command cmdFWB(ivcam2::FWB);
                cmdFWB.require_response = false;
                cmdFWB.param1 = index;
                cmdFWB.param2 = packet_size;
                cmdFWB.data.assign(image.data() + index, image.data() + index + packet_size);
                res = hwm->send(cmdFWB);
                i += packet_size;
            }

            if (callback)
                callback->on_update_progress(continue_from + (float)sector_index / (float)sector_count * ratio);
        }
    }

    void l500_device::update_section(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& merged_image, flash_section fs, uint32_t tables_size,
        update_progress_callback_ptr callback, float continue_from, float ratio)
    {
        auto first_table_offset = fs.tables.front().offset;
        float total_size = fs.app_size + tables_size;

        float app_ratio = fs.app_size / total_size * ratio;
        float tables_ratio = tables_size / total_size * ratio;

        update_flash_section(hwm, merged_image, fs.offset, fs.app_size, callback, continue_from, app_ratio);
        update_flash_section(hwm, merged_image, first_table_offset, tables_size, callback, app_ratio, tables_ratio);
    }

    void l500_device::update_flash_internal(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& image, std::vector<uint8_t>& flash_backup, update_progress_callback_ptr callback, int update_mode)
    {
        auto flash_image_info = ivcam2::get_flash_info(image);
        auto flash_backup_info = ivcam2::get_flash_info(flash_backup);
        auto merged_image = merge_images(flash_backup_info, flash_image_info, image);

        // update read-write section
        auto first_table_offset = flash_image_info.read_write_section.tables.front().offset;
        auto tables_size = flash_image_info.header.read_write_start_address + flash_image_info.header.read_write_size - first_table_offset;
        update_section(hwm, merged_image, flash_image_info.read_write_section, tables_size, callback, 0, update_mode == RS2_UNSIGNED_UPDATE_MODE_READ_ONLY ? 0.5 : 1.0);

        if (update_mode == RS2_UNSIGNED_UPDATE_MODE_READ_ONLY)
        {
            // update read-only section
            auto first_table_offset = flash_image_info.read_only_section.tables.front().offset;
            auto tables_size = flash_image_info.header.read_only_start_address + flash_image_info.header.read_only_size - first_table_offset;
            update_section(hwm, merged_image, flash_image_info.read_only_section, tables_size, callback, 0.5, 0.5);
        }
    }

    void l500_device::update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode)
    {
        if (_is_locked)
            throw std::runtime_error("this camera is locked and doesn't allow direct flash write, for firmware update use rs2_update_firmware method (DFU)");

        get_raw_depth_sensor().invoke_powered([&](platform::uvc_device& dev)
        {
            command cmdPFD(ivcam2::PFD);
            cmdPFD.require_response = false;
            auto res = _hw_monitor->send(cmdPFD);

            switch (update_mode)
            {
            case RS2_UNSIGNED_UPDATE_MODE_FULL:
                update_flash_section(_hw_monitor, image, 0, ivcam2::FLASH_SIZE, callback, 0, 1.0);
                break;
            case RS2_UNSIGNED_UPDATE_MODE_UPDATE:
            case RS2_UNSIGNED_UPDATE_MODE_READ_ONLY:
            {
                auto flash_backup = backup_flash(nullptr);
                update_flash_internal(_hw_monitor, image, flash_backup, callback, update_mode);
                break;
            }
            default:
                throw std::runtime_error("invalid update mode value");
            }

            if (callback) callback->on_update_progress(1.0);

            command cmdHWRST(ivcam2::HW_RESET);
            res = _hw_monitor->send(cmdHWRST);
        });
    }

    notification l500_notification_decoder::decode(int value)
    {
        if (l500_fw_error_report.find(static_cast<uint8_t>(value)) != l500_fw_error_report.end())
            return{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, value, RS2_LOG_SEVERITY_ERROR, l500_fw_error_report.at(static_cast<uint8_t>(value)) };

        return{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, value, RS2_LOG_SEVERITY_WARN, (to_string() << "L500 HW report - unresolved type " << value) };
    }
}
