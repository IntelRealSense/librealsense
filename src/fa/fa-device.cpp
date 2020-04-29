// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>
#include <string>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "fa-device.h"
#include "fa-private.h"
#include "stream.h"
#include "environment.h"

#include "ds5/ds5-timestamp.h"

#include "proc/decimation-filter.h"
#include "proc/threshold.h"
#include "proc/disparity-transform.h"
#include "proc/spatial-filter.h"
#include "proc/colorizer.h"
#include "proc/temporal-filter.h"
#include "proc/y8i-to-y8y8.h"
#include "proc/y12i-to-y16y16.h"
#include "proc/color-formats-converter.h"
#include "proc/syncer-processing-block.h"
#include "proc/hole-filling-filter.h"
#include "proc/depth-formats-converter.h"
#include "proc/depth-decompress.h"
#include "../common/fw/firmware-version.h"
#include "fw-update/fw-update-unsigned.h"
#include "../third-party/json.hpp"

#include "proc/uvyv_to_yuyv.h"
#include "../usb/usb-types.h"

namespace librealsense
{
    std::map<uint32_t, rs2_format> fa_ir_fourcc_to_rs2_format = {
        {rs_fourcc('Y','U','Y','2'), RS2_FORMAT_YUYV},
        {rs_fourcc('U','Y','V','Y'), RS2_FORMAT_UYVY},
        {rs_fourcc('Z','1','6',' '),  RS2_FORMAT_Z16},
        {rs_fourcc('B','G','1','6'), RS2_FORMAT_RAW16}
    }; 
    std::map<uint32_t, rs2_stream> fa_ir_fourcc_to_rs2_stream = {
        {rs_fourcc('Y','U','Y','2'), RS2_STREAM_INFRARED},
        {rs_fourcc('U','Y','V','Y'), RS2_STREAM_INFRARED},
        {rs_fourcc('Z','1','6',' '),  RS2_STREAM_INFRARED},
        {rs_fourcc('B','G','1','6'), RS2_STREAM_INFRARED}
    };   
    std::vector<uint8_t> fa_device::send_receive_raw_data(const std::vector<uint8_t>& input)
    {
        return _hw_monitor->send(input);
    }

    void fa_device::hardware_reset()
    {
        command cmd(ds::HWRST);
        _hw_monitor->send(cmd);
    }

    void fa_device::enter_update_state() const
    {
        try {
            LOG_INFO("entering to update state, device disconnect is expected");
            command cmd(ds::DFU);
            cmd.param1 = 1;
            _hw_monitor->send(cmd);
        }
        catch (...) {
            // The set command returns a failure because switching to DFU resets the device while the command is running.
        }
    }

    std::vector<uint8_t> fa_device::backup_flash(update_progress_callback_ptr callback)
    {
        int flash_size = 1024 * 2048;
        int max_bulk_size = 1016;
        int max_iterations = int(flash_size / max_bulk_size + 1);

        std::vector<uint8_t> flash;
        flash.reserve(flash_size);
		
        uvc_sensor& raw_ir_sensor = get_raw_ir_sensor();
        raw_ir_sensor.invoke_powered([&](platform::uvc_device& dev)
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
                            command cmd(ds::FRB);
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

    static void update_flash_section(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& image, uint32_t offset, uint32_t size, update_progress_callback_ptr callback, float continue_from, float ratio)
    {
        size_t sector_count = size / fa::FLASH_SECTOR_SIZE;
        size_t first_sector = offset / fa::FLASH_SECTOR_SIZE;

        if (sector_count * ds::FLASH_SECTOR_SIZE != size)
            sector_count++;

        sector_count += first_sector;

        for (int sector_index = first_sector; sector_index < sector_count; sector_index++)
        {
            command cmdFES(ds::FES);
            cmdFES.require_response = false;
            cmdFES.param1 = sector_index;
            cmdFES.param2 = 1;
            auto res = hwm->send(cmdFES);

            for (int i = 0; i < ds::FLASH_SECTOR_SIZE; )
            {
                auto index = sector_index * ds::FLASH_SECTOR_SIZE + i;
                if (index >= offset + size)
                    break;
                int packet_size = std::min((int)(HW_MONITOR_COMMAND_SIZE - (i % HW_MONITOR_COMMAND_SIZE)), (int)(ds::FLASH_SECTOR_SIZE - i));
                command cmdFWB(ds::FWB);
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

    static void update_section(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& merged_image, flash_section fs, uint32_t tables_size,
        update_progress_callback_ptr callback, float continue_from, float ratio)
    {
        auto first_table_offset = fs.tables.front().offset;
        float total_size = float(fs.app_size + tables_size);

        float app_ratio = fs.app_size / total_size * ratio;
        float tables_ratio = tables_size / total_size * ratio;

        update_flash_section(hwm, merged_image, fs.offset, fs.app_size, callback, continue_from, app_ratio);
        update_flash_section(hwm, merged_image, first_table_offset, tables_size, callback, app_ratio, tables_ratio);
    }

    static void update_flash_internal(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& image, std::vector<uint8_t>& flash_backup, update_progress_callback_ptr callback, int update_mode)
    {
        auto flash_image_info = ds::get_flash_info(image);
        auto flash_backup_info = ds::get_flash_info(flash_backup);
        auto merged_image = merge_images(flash_backup_info, flash_image_info, image);

        // update read-write section
        auto first_table_offset = flash_image_info.read_write_section.tables.front().offset;
        auto tables_size = flash_image_info.header.read_write_start_address + flash_image_info.header.read_write_size - first_table_offset;
        update_section(hwm, merged_image, flash_image_info.read_write_section, tables_size, callback, 0, update_mode == RS2_UNSIGNED_UPDATE_MODE_READ_ONLY ? 0.5f : 1.0f);

        if (update_mode == RS2_UNSIGNED_UPDATE_MODE_READ_ONLY)
        {
            // update read-only section
            auto first_table_offset = flash_image_info.read_only_section.tables.front().offset;
            auto tables_size = flash_image_info.header.read_only_start_address + flash_image_info.header.read_only_size - first_table_offset;
            update_section(hwm, merged_image, flash_image_info.read_only_section, tables_size, callback, 0.5, 0.5);
        }
    }
    
    void fa_device::update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode)
    {
        if (_is_locked)
            throw std::runtime_error("this camera is locked and doesn't allow direct flash write, for firmware update use rs2_update_firmware method (DFU)");

        uvc_sensor& raw_ir_sensor = get_raw_ir_sensor();
        raw_ir_sensor.invoke_powered([&](platform::uvc_device& dev)
            {
                command cmdPFD(ds::PFD);
                cmdPFD.require_response = false;
                auto res = _hw_monitor->send(cmdPFD);

                switch (update_mode)
                {
                case RS2_UNSIGNED_UPDATE_MODE_FULL:
                    update_flash_section(_hw_monitor, image, 0, ds::FLASH_SIZE, callback, 0, 1.0);
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

                command cmdHWRST(ds::HWRST);
                res = _hw_monitor->send(cmdHWRST);
            });
    }

    class fa_ir_sensor : public synthetic_sensor
    {
    public:
        fa_ir_sensor(fa_device* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor)
            : synthetic_sensor("Infrared Camera", uvc_sensor, owner, fa_ir_fourcc_to_rs2_format, fa_ir_fourcc_to_rs2_stream),
            _owner(owner) {}

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto results = synthetic_sensor::init_stream_profiles();

            for (auto&& p : results)
            {
                // Register stream types
                if (p->get_stream_type() == RS2_STREAM_INFRARED && p->get_stream_index() == 0)
                {
                    assign_stream(_owner->_left_ir_stream, p);
                    environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_owner->_left_ir_stream, *p);
                }
                else if (p->get_stream_type() == RS2_STREAM_INFRARED && p->get_stream_index() == 1)
                {
                    assign_stream(_owner->_right_ir_stream, p);
                    environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_owner->_right_ir_stream, *p);
                }
            }

            return results;
        }

    private:
        const fa_device* _owner;
    };

    void register_options(std::shared_ptr<fa_ir_sensor> ir_ep, std::shared_ptr<uvc_sensor> raw_ir_ep)
    {
        // OPTIONS
        // EXPOSURE
        auto exposure_option = std::make_shared<uvc_pu_option>(*raw_ir_ep, RS2_OPTION_EXPOSURE);
        auto auto_exposure_option = std::make_shared<uvc_pu_option>(*raw_ir_ep, RS2_OPTION_ENABLE_AUTO_EXPOSURE);
        ir_ep->register_option(RS2_OPTION_EXPOSURE, exposure_option);
        ir_ep->register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, auto_exposure_option);
        ir_ep->register_option(RS2_OPTION_EXPOSURE,
            std::make_shared<auto_disabling_control>(
                exposure_option,
                auto_exposure_option));

        // GAIN
        ir_ep->register_pu(RS2_OPTION_GAIN);
        //WHITE BALANCE
        /*auto white_balance_option = std::make_shared<uvc_pu_option>(*raw_ir_ep, RS2_OPTION_WHITE_BALANCE);
        auto auto_white_balance_option = std::make_shared<uvc_pu_option>(*raw_ir_ep, RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
        ir_ep->register_option(RS2_OPTION_WHITE_BALANCE, white_balance_option);
        ir_ep->register_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, auto_white_balance_option);
        ir_ep->register_option(RS2_OPTION_WHITE_BALANCE,
            std::make_shared<auto_disabling_control>(
                white_balance_option,
                auto_white_balance_option));*/

        //LOW LIGHT COMPENSATION
        //ir_ep->register_pu(RS2_OPTION_BACKLIGHT_COMPENSATION);
    }
    
    fa_device::fa_device(const std::shared_ptr<context>& ctx,
        const std::vector<platform::uvc_device_info>& uvc_infos,
        const platform::backend_device_group& group,
        bool register_device_notifications)
        : device(ctx, group, register_device_notifications),
        _left_ir_stream(new stream(RS2_STREAM_INFRARED, 0)),
        _right_ir_stream(new stream(RS2_STREAM_INFRARED, 1))
    {
        std::vector<std::shared_ptr<platform::uvc_device>> devs;
        for (auto&& info : uvc_infos)
            devs.push_back(ctx->get_backend().create_uvc_device(info));

        auto raw_ir_ep = std::make_shared<uvc_sensor>("Infrared Camera",
            std::make_shared<platform::multi_pins_uvc_device>(devs),
            std::unique_ptr<ds5_timestamp_reader>(new ds5_timestamp_reader(environment::get_instance().get_time_service())),
            this);

        auto ir_ep = std::make_shared<fa_ir_sensor>(this, raw_ir_ep);
        ir_ep->register_processing_block(processing_block_factory::create_pbf_vector<yuy2_converter>
            (RS2_FORMAT_YUYV, map_supported_color_formats(RS2_FORMAT_YUYV), RS2_STREAM_INFRARED, 0));

        // Second Infrared is marked as UYVY so that the streams will have different profiles - needed because of design of mf-uvc class 
        // It is then converted back to YUYV for streaming
        // TODO Remi - composite processing block to be used so that uyvy will be read as yuyv, and the color formats will be still available
        ir_ep->register_processing_block(processing_block_factory::create_pbf_vector<yuy2_converter>
            (RS2_FORMAT_UYVY, map_supported_color_formats(RS2_FORMAT_YUYV), RS2_STREAM_INFRARED, 1));


        // RAW STREAM
        ir_ep->register_processing_block(processing_block_factory::
            create_id_pbf(RS2_FORMAT_RAW16, RS2_STREAM_INFRARED, 0));
        ir_ep->register_processing_block(processing_block_factory::create_pbf_vector<memcpy_converter>
            (RS2_FORMAT_RAW16, { RS2_FORMAT_Y16 }, RS2_STREAM_INFRARED, 0));
        /*ir_ep->register_processing_block(processing_block_factory::
            create_id_pbf(RS2_FORMAT_RAW16, RS2_STREAM_INFRARED, 1));*/
        std::vector<rs2_format> target = { RS2_FORMAT_RAW16, RS2_FORMAT_Y16 };
        ir_ep->register_processing_block(processing_block_factory::create_pbf_vector<memcpy_converter>
            (RS2_FORMAT_Z16, target, RS2_STREAM_INFRARED, 1));
        /*ir_ep->register_processing_block(processing_block_factory::create_pbf_vector<raw16_to_y16_converter>
            (RS2_FORMAT_Z16, { RS2_FORMAT_Y16 }, RS2_STREAM_INFRARED, 1));*/
        add_sensor(ir_ep);
        
        // CAMERA INFO
        register_info(RS2_CAMERA_INFO_NAME, "F450 Camera");
        std::string pid_str(to_string() << std::setfill('0') << std::setw(4) << std::hex << uvc_infos.front().pid);
        std::transform(pid_str.begin(), pid_str.end(), pid_str.begin(), ::toupper);

        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, uvc_infos.front().unique_id);
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, uvc_infos.front().device_path);
        register_info(RS2_CAMERA_INFO_PRODUCT_ID, pid_str);

        //USB TYPE DESCRIPTION
        auto _usb_mode = platform::usb_spec::usb3_type;
        std::string usb_type_str(platform::usb_spec_names.at(_usb_mode));
        _usb_mode = raw_ir_ep->get_usb_specification();
        if (platform::usb_spec_names.count(_usb_mode) && (platform::usb_undefined != _usb_mode)) {
            usb_type_str = platform::usb_spec_names.at(_usb_mode);
            register_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, usb_type_str);
        }
                
        // Metadata registration
        register_metadata();
        
        // Options registration
        register_options(ir_ep, raw_ir_ep);
    }
    
    
    void fa_device::register_metadata()
    {
        auto& ir_sensor = get_ir_sensor();

        ir_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP,
            make_uvc_header_parser(&platform::uvc_header::timestamp));

        // uvc header size
        static const int S_METADATA_UVC_PART_SIZE = 12;

        auto md_prop_offset = S_METADATA_UVC_PART_SIZE;
        //FRAME_COUNTER
        ir_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER,
            make_attribute_parser(&md_f400_header::frame_counter,
                md_f400_capture_timing_attributes::frame_counter_attribute, md_prop_offset));

        //SENSOR_TIMESTAMP
        ir_sensor.register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP,
            make_attribute_parser(&md_f400_header::sensor_timestamp,
                md_f400_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset));

        //EXPOSURE
        ir_sensor.register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE,
            make_attribute_parser(&md_f400_header::exposure_time,
                md_f400_capture_timing_attributes::exposure_time_attribute, md_prop_offset));

        //GAIN
        ir_sensor.register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL,
            make_attribute_parser(&md_f400_header::gain_value,
                md_f400_capture_timing_attributes::gain_value_attribute, md_prop_offset));

        //LED
        ir_sensor.register_metadata(RS2_FRAME_METADATA_LED_POWER_MODE,
            make_attribute_parser(&md_f400_header::led_status,
                md_f400_capture_timing_attributes::led_status_attribute, md_prop_offset));

        //LASER
        ir_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE,
            make_attribute_parser(&md_f400_header::laser_status,
                md_f400_capture_timing_attributes::laser_status_attribute, md_prop_offset));

        //PRESET
        ir_sensor.register_metadata(RS2_FRAME_METADATA_PRESET_ID,
            make_attribute_parser(&md_f400_header::preset_id,
                md_f400_capture_timing_attributes::preset_id_attribute, md_prop_offset));

    }

    notification fa_notification_decoder::decode(int value)
    {
        if (ds::ds5_fw_error_report.find(static_cast<uint8_t>(value)) != ds::ds5_fw_error_report.end())
            return{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, value, RS2_LOG_SEVERITY_ERROR, ds::ds5_fw_error_report.at(static_cast<uint8_t>(value)) };

        return{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, value, RS2_LOG_SEVERITY_WARN, (to_string() << "D400 HW report - unresolved type " << value) };
    }

    
    void fa_device::create_snapshot(std::shared_ptr<debug_interface>& snapshot) const
    {
        //TODO: Implement
    }
    void fa_device::enable_recording(std::function<void(const debug_interface&)> record_action)
    {
        //TODO: Implement
    }

    platform::usb_spec fa_device::get_usb_spec() const
    {
        if (!supports_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR))
            return platform::usb_undefined;
        auto str = get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR);
        for (auto u : platform::usb_spec_names)
        {
            if (u.second.compare(str) == 0)
                return u.first;
        }
        return platform::usb_undefined;
    }

    
    double fa_device::get_device_time_ms()
    {
        return 0.0;
        /*
        if (!_hw_monitor)
            throw wrong_api_call_sequence_exception("_hw_monitor is not initialized yet");

        command cmd(ds::MRD, ds::REGISTER_CLOCK_0, ds::REGISTER_CLOCK_0 + 4);
        auto res = _hw_monitor->send(cmd);

        if (res.size() < sizeof(uint32_t))
        {
            LOG_DEBUG("size(res):" << res.size());
            throw std::runtime_error("Not enough bytes returned from the firmware!");
        }
        uint32_t dt = *(uint32_t*)res.data();
        double ts = dt * TIMESTAMP_USEC_TO_MSEC;
        return ts;*/
    }
}
