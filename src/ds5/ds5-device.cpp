// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

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

#include "ds5-device.h"
#include "ds5-private.h"
#include "ds5-options.h"
#include "ds5-timestamp.h"
#include "stream.h"
#include "environment.h"
#include "ds5-color.h"
#include "ds5-nonmonochrome.h"

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

namespace librealsense
{
    std::map<uint32_t, rs2_format> ds5_depth_fourcc_to_rs2_format = {
        {rs_fourcc('Y','U','Y','2'), RS2_FORMAT_YUYV},
        {rs_fourcc('Y','U','Y','V'), RS2_FORMAT_YUYV},
        {rs_fourcc('U','Y','V','Y'), RS2_FORMAT_UYVY},
        {rs_fourcc('G','R','E','Y'), RS2_FORMAT_Y8},
        {rs_fourcc('Y','8','I',' '), RS2_FORMAT_Y8I},
        {rs_fourcc('W','1','0',' '), RS2_FORMAT_W10},
        {rs_fourcc('Y','1','6',' '), RS2_FORMAT_Y16},
        {rs_fourcc('Y','1','2','I'), RS2_FORMAT_Y12I},
        {rs_fourcc('Z','1','6',' '), RS2_FORMAT_Z16},
        {rs_fourcc('Z','1','6','H'), RS2_FORMAT_Z16H},
        {rs_fourcc('R','G','B','2'), RS2_FORMAT_BGR8}
        
    };
    std::map<uint32_t, rs2_stream> ds5_depth_fourcc_to_rs2_stream = {
        {rs_fourcc('Y','U','Y','2'), RS2_STREAM_INFRARED},
        {rs_fourcc('Y','U','Y','V'), RS2_STREAM_INFRARED},
        {rs_fourcc('U','Y','V','Y'), RS2_STREAM_INFRARED},
        {rs_fourcc('G','R','E','Y'), RS2_STREAM_INFRARED},
        {rs_fourcc('Y','8','I',' '), RS2_STREAM_INFRARED},
        {rs_fourcc('W','1','0',' '), RS2_STREAM_INFRARED},
        {rs_fourcc('Y','1','6',' '), RS2_STREAM_INFRARED},
        {rs_fourcc('Y','1','2','I'), RS2_STREAM_INFRARED},
        {rs_fourcc('R','G','B','2'), RS2_STREAM_INFRARED},
        {rs_fourcc('Z','1','6',' '), RS2_STREAM_DEPTH},
        {rs_fourcc('Z','1','6','H'), RS2_STREAM_DEPTH}
    };

    ds5_auto_exposure_roi_method::ds5_auto_exposure_roi_method(
        const hw_monitor& hwm,
        ds::fw_cmd cmd)
        : _hw_monitor(hwm), _cmd(cmd) {}

    void ds5_auto_exposure_roi_method::set(const region_of_interest& roi)
    {
        command cmd(_cmd);
        cmd.param1 = roi.min_y;
        cmd.param2 = roi.max_y;
        cmd.param3 = roi.min_x;
        cmd.param4 = roi.max_x;
        _hw_monitor.send(cmd);
    }

    region_of_interest ds5_auto_exposure_roi_method::get() const
    {
        region_of_interest roi;
        command cmd(_cmd + 1);
        auto res = _hw_monitor.send(cmd);

        if (res.size() < 4 * sizeof(uint16_t))
        {
            throw std::runtime_error("Invalid result size!");
        }

        auto words = reinterpret_cast<uint16_t*>(res.data());

        roi.min_y = words[0];
        roi.max_y = words[1];
        roi.min_x = words[2];
        roi.max_x = words[3];

        return roi;
    }

    std::vector<uint8_t> ds5_device::send_receive_raw_data(const std::vector<uint8_t>& input)
    {
        return _hw_monitor->send(input);
    }

    void ds5_device::hardware_reset()
    {
        command cmd(ds::HWRST);
        _hw_monitor->send(cmd);
    }

    void ds5_device::enter_update_state() const
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

    std::vector<uint8_t> ds5_device::backup_flash(update_progress_callback_ptr callback)
    {
        int flash_size = 1024 * 2048;
        int max_bulk_size = 1016;
        int max_iterations = int(flash_size / max_bulk_size + 1);

        std::vector<uint8_t> flash;
        flash.reserve(flash_size);

        uvc_sensor& raw_depth_sensor = get_raw_depth_sensor();
        raw_depth_sensor.invoke_powered([&](platform::uvc_device& dev)
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

    void update_flash_section(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& image, uint32_t offset, uint32_t size, update_progress_callback_ptr callback, float continue_from, float ratio)
    {
        size_t sector_count = size / ds::FLASH_SECTOR_SIZE;
        size_t first_sector = offset / ds::FLASH_SECTOR_SIZE;

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

    void update_section(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& merged_image, flash_section fs, uint32_t tables_size,
        update_progress_callback_ptr callback, float continue_from, float ratio)
    {
        auto first_table_offset = fs.tables.front().offset;
        float total_size = float(fs.app_size + tables_size);

        float app_ratio = fs.app_size / total_size * ratio;
        float tables_ratio = tables_size / total_size * ratio;

        update_flash_section(hwm, merged_image, fs.offset, fs.app_size, callback, continue_from, app_ratio);
        update_flash_section(hwm, merged_image, first_table_offset, tables_size, callback, app_ratio, tables_ratio);
    }

    void update_flash_internal(std::shared_ptr<hw_monitor> hwm, const std::vector<uint8_t>& image, std::vector<uint8_t>& flash_backup, update_progress_callback_ptr callback, int update_mode)
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

    void ds5_device::update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode)
    {
        if (_is_locked)
            throw std::runtime_error("this camera is locked and doesn't allow direct flash write, for firmware update use rs2_update_firmware method (DFU)");

        auto& raw_depth_sensor = get_raw_depth_sensor();
        raw_depth_sensor.invoke_powered([&](platform::uvc_device& dev)
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

    class ds5_depth_sensor : public synthetic_sensor, public video_sensor_interface, public depth_stereo_sensor, public roi_sensor_base
    {
    public:
        explicit ds5_depth_sensor(ds5_device* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor)
            : synthetic_sensor(ds::DEPTH_STEREO, uvc_sensor, owner, ds5_depth_fourcc_to_rs2_format, ds5_depth_fourcc_to_rs2_stream),
            _owner(owner),
            _depth_units(-1)
        {}

        processing_blocks get_recommended_processing_blocks() const override
        {
            return get_ds5_depth_recommended_proccesing_blocks();
        };

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
        {
            rs2_intrinsics result;
            
            if (ds::try_get_intrinsic_by_resolution_new(*_owner->_new_calib_table_raw,
                profile.width, profile.height, &result))
            {
                return result;
            }
            else 
            {
                return get_intrinsic_by_resolution(
                    *_owner->_coefficients_table_raw,
                    ds::calibration_table_id::coefficients_table_id,
                    profile.width, profile.height);
            }
        }

        void open(const stream_profiles& requests) override
        {
            _depth_units = get_option(RS2_OPTION_DEPTH_UNITS).query();
            synthetic_sensor::open(requests);
        }

        /*
        Infrared profiles are initialized with the following logic:
        - If device has color sensor (D415 / D435), infrared profile is chosen with Y8 format
        - If device does not have color sensor:
           * if it is a rolling shutter device (D400 / D410 / D415 / D405), infrared profile is chosen with RGB8 format
           * for other devices (D420 / D430), infrared profile is chosen with Y8 format
        */
        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto&& results = synthetic_sensor::init_stream_profiles();

            for (auto&& p : results)
            {
                // Register stream types
                if (p->get_stream_type() == RS2_STREAM_DEPTH)
                {
                    assign_stream(_owner->_depth_stream, p);
                }
                else if (p->get_stream_type() == RS2_STREAM_INFRARED && p->get_stream_index() < 2)
                {
                    assign_stream(_owner->_left_ir_stream, p);
                }
                else if (p->get_stream_type() == RS2_STREAM_INFRARED  && p->get_stream_index() == 2)
                {
                    assign_stream(_owner->_right_ir_stream, p);
                }
                auto&& vid_profile = dynamic_cast<video_stream_profile_interface*>(p.get());

                // Register intrinsics
                if (p->get_format() != RS2_FORMAT_Y16) // Y16 format indicate unrectified images, no intrinsics are available for these
                {
                    const auto&& profile = to_profile(p.get());
                    std::weak_ptr<ds5_depth_sensor> wp =
                        std::dynamic_pointer_cast<ds5_depth_sensor>(this->shared_from_this());
                    vid_profile->set_intrinsics([profile, wp]()
                    {
                        auto sp = wp.lock();
                        if (sp)
                            return sp->get_intrinsics(profile);
                        else
                            return rs2_intrinsics{};
                    });
                }
            }

            return results;
        }

        float get_depth_scale() const override 
        { 
            if (_depth_units < 0)
                _depth_units = get_option(RS2_OPTION_DEPTH_UNITS).query();
            return _depth_units; 
        }

        void set_depth_scale(float val){ _depth_units = val; }

        float get_stereo_baseline_mm() const override { return _owner->get_stereo_baseline_mm(); }

        void create_snapshot(std::shared_ptr<depth_sensor>& snapshot) const override
        {
            snapshot = std::make_shared<depth_sensor_snapshot>(get_depth_scale());
        }

        void create_snapshot(std::shared_ptr<depth_stereo_sensor>& snapshot) const override
        {
            snapshot = std::make_shared<depth_stereo_sensor_snapshot>(get_depth_scale(), get_stereo_baseline_mm());
        }

        void enable_recording(std::function<void(const depth_sensor&)> recording_function) override
        {
            //does not change over time
        }

        void enable_recording(std::function<void(const depth_stereo_sensor&)> recording_function) override
        {
            //does not change over time
        }
    protected:
        const ds5_device* _owner;
        mutable std::atomic<float> _depth_units;
        float _stereo_baseline_mm;
    };

    class ds5u_depth_sensor : public ds5_depth_sensor
    {
    public:
        explicit ds5u_depth_sensor(ds5u_device* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor)
            : ds5_depth_sensor(owner, uvc_sensor), _owner(owner)
        {}

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto&& results = synthetic_sensor::init_stream_profiles();

            for (auto&& p : results)
            {
                // Register stream types
                if (p->get_stream_type() == RS2_STREAM_DEPTH)
                {
                    assign_stream(_owner->_depth_stream, p);
                }
                else if (p->get_stream_type() == RS2_STREAM_INFRARED && p->get_stream_index() < 2)
                {
                    assign_stream(_owner->_left_ir_stream, p);
                }
                else if (p->get_stream_type() == RS2_STREAM_INFRARED  && p->get_stream_index() == 2)
                {
                    assign_stream(_owner->_right_ir_stream, p);
                }
                auto&& video = dynamic_cast<video_stream_profile_interface*>(p.get());

                // Register intrinsics
                if (p->get_format() != RS2_FORMAT_Y16) // Y16 format indicate unrectified images, no intrinsics are available for these
                {
                    const auto&& profile = to_profile(p.get());
                    std::weak_ptr<ds5_depth_sensor> wp = std::dynamic_pointer_cast<ds5_depth_sensor>(this->shared_from_this());
                    video->set_intrinsics([profile, wp]()
                    {
                        auto sp = wp.lock();
                        if (sp)
                            return sp->get_intrinsics(profile);
                        else
                            return rs2_intrinsics{};
                    });
                }
            }

            return results;
        }

    private:
        const ds5u_device* _owner;
    };

    bool ds5_device::is_camera_in_advanced_mode() const
    {
        command cmd(ds::UAMG);
        assert(_hw_monitor);
        auto ret = _hw_monitor->send(cmd);
        if (ret.empty())
            throw invalid_value_exception("command result is empty!");

        return (0 != ret.front());
    }

    float ds5_device::get_stereo_baseline_mm() const
    {
        using namespace ds;
        auto table = check_calib<coefficients_table>(*_coefficients_table_raw);
        return fabs(table->baseline);
    }

    std::vector<uint8_t> ds5_device::get_raw_calibration_table(ds::calibration_table_id table_id) const
    {
        command cmd(ds::GETINTCAL, table_id);
        return _hw_monitor->send(cmd);
    }

    std::vector<uint8_t> ds5_device::get_new_calibration_table() const
    {
        if (_fw_version >= firmware_version("5.11.9.5"))
        {
            command cmd(ds::RECPARAMSGET);
            return _hw_monitor->send(cmd);
        }
        return {};
    }

    ds::d400_caps ds5_device::parse_device_capabilities(const uint16_t pid) const
    {
        using namespace ds;
        std::array<unsigned char,HW_MONITOR_BUFFER_SIZE> gvd_buf;
        _hw_monitor->get_gvd(gvd_buf.size(), gvd_buf.data(), GVD);

        // Opaque retrieval
        d400_caps val{d400_caps::CAP_UNDEFINED};
        if (gvd_buf[active_projector])  // DepthActiveMode
            val |= d400_caps::CAP_ACTIVE_PROJECTOR;
        if (gvd_buf[rgb_sensor])                           // WithRGB
            val |= d400_caps::CAP_RGB_SENSOR;
        if (gvd_buf[imu_sensor])
        {
            val |= d400_caps::CAP_IMU_SENSOR;
            if (hid_bmi_055_pid.end() != hid_bmi_055_pid.find(pid))
                val |= d400_caps::CAP_BMI_055;
            else
            {
                if (hid_bmi_085_pid.end() != hid_bmi_085_pid.find(pid))
                    val |= d400_caps::CAP_BMI_085;
                else
                    LOG_WARNING("The IMU sensor is undefined for PID " << std::hex << pid << std::dec);
            }
        }
        if (0xFF != (gvd_buf[fisheye_sensor_lb] & gvd_buf[fisheye_sensor_hb]))
            val |= d400_caps::CAP_FISHEYE_SENSOR;
        if (0x1 == gvd_buf[depth_sensor_type])
            val |= d400_caps::CAP_ROLLING_SHUTTER;  // e.g. ASRC
        if (0x2 == gvd_buf[depth_sensor_type])
            val |= d400_caps::CAP_GLOBAL_SHUTTER;   // e.g. AWGC

        return val;
    }

    std::shared_ptr<synthetic_sensor> ds5_device::create_depth_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& all_device_infos)
    {
        using namespace ds;

        auto&& backend = ctx->get_backend();

        std::vector<std::shared_ptr<platform::uvc_device>> depth_devices;
        for (auto&& info : filter_by_mi(all_device_infos, 0)) // Filter just mi=0, DEPTH
            depth_devices.push_back(backend.create_uvc_device(info));

        std::unique_ptr<frame_timestamp_reader> timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));
        std::unique_ptr<frame_timestamp_reader> timestamp_reader_metadata(new ds5_timestamp_reader_from_metadata(std::move(timestamp_reader_backup)));
        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto raw_depth_ep = std::make_shared<uvc_sensor>("Raw Depth Sensor", std::make_shared<platform::multi_pins_uvc_device>(depth_devices),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(timestamp_reader_metadata), _tf_keeper, enable_global_time_option)), this);

        raw_depth_ep->register_xu(depth_xu); // make sure the XU is initialized every time we power the camera

        auto depth_ep = std::make_shared<ds5_depth_sensor>(this, raw_depth_ep);

        depth_ep->register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, filter_by_mi(all_device_infos, 0).front().device_path);

        depth_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        depth_ep->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 1));
        depth_ep->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_Z16, RS2_STREAM_DEPTH));

        depth_ep->register_processing_block({ {RS2_FORMAT_W10} }, { {RS2_FORMAT_RAW10, RS2_STREAM_INFRARED, 1} }, []() { return std::make_shared<w10_converter>(RS2_FORMAT_RAW10); });
        depth_ep->register_processing_block({ {RS2_FORMAT_W10} }, { {RS2_FORMAT_Y10BPACK, RS2_STREAM_INFRARED, 1} }, []() { return std::make_shared<w10_converter>(RS2_FORMAT_Y10BPACK); });

        return depth_ep;
    }

    ds5_device::ds5_device(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        : device(ctx, group), global_time_interface(),
          auto_calibrated(_hw_monitor),
          _device_capabilities(ds::d400_caps::CAP_UNDEFINED),
          _depth_stream(new stream(RS2_STREAM_DEPTH)),
          _left_ir_stream(new stream(RS2_STREAM_INFRARED, 1)),
          _right_ir_stream(new stream(RS2_STREAM_INFRARED, 2))
    {
        _depth_device_idx = add_sensor(create_depth_device(ctx, group.uvc_devices));
        init(ctx, group);
    }

    void ds5_device::init(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
    {
        using namespace ds;

        auto&& backend = ctx->get_backend();
        auto& raw_sensor = get_raw_depth_sensor();

        if (group.usb_devices.size() > 0)
        {
            _hw_monitor = std::make_shared<hw_monitor>(
                std::make_shared<locked_transfer>(
                    backend.create_usb_device(group.usb_devices.front()), raw_sensor));
        }
        else
        {
            _hw_monitor = std::make_shared<hw_monitor>(
                std::make_shared<locked_transfer>(
                    std::make_shared<command_transfer_over_xu>(
                        raw_sensor, depth_xu, DS5_HWMONITOR),
                    raw_sensor));
        }

        // Define Left-to-Right extrinsics calculation (lazy)
        // Reference CS - Right-handed; positive [X,Y,Z] point to [Left,Up,Forward] accordingly.
        _left_right_extrinsics = std::make_shared<lazy<rs2_extrinsics>>([this]()
        {
            rs2_extrinsics ext = identity_matrix();
            auto table = check_calib<coefficients_table>(*_coefficients_table_raw);
            ext.translation[0] = 0.001f * table->baseline; // mm to meters
            return ext;
        });

        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_left_ir_stream);
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_depth_stream, *_right_ir_stream, _left_right_extrinsics);

        register_stream_to_extrinsic_group(*_depth_stream, 0);
        register_stream_to_extrinsic_group(*_left_ir_stream, 0);
        register_stream_to_extrinsic_group(*_right_ir_stream, 0);

        _coefficients_table_raw = [this]() { return get_raw_calibration_table(coefficients_table_id); };
        _new_calib_table_raw = [this]() { return get_new_calibration_table(); };

        auto pid = group.uvc_devices.front().pid;
        std::string device_name = (rs400_sku_names.end() != rs400_sku_names.find(pid)) ? rs400_sku_names.at(pid) : "RS4xx";

        std::vector<uint8_t> gvd_buff(HW_MONITOR_BUFFER_SIZE);
        _hw_monitor->get_gvd(gvd_buff.size(), gvd_buff.data(), GVD);
        // fooling tests recordings - don't remove
        _hw_monitor->get_gvd(gvd_buff.size(), gvd_buff.data(), GVD);

        auto optic_serial = _hw_monitor->get_module_serial_string(gvd_buff, module_serial_offset);
        auto asic_serial = _hw_monitor->get_module_serial_string(gvd_buff, module_asic_serial_offset);
        auto fwv = _hw_monitor->get_firmware_version_string(gvd_buff, camera_fw_version_offset);
        _fw_version = firmware_version(fwv);

        _recommended_fw_version = firmware_version(D4XX_RECOMMENDED_FIRMWARE_VERSION);
        if (_fw_version >= firmware_version("5.10.4.0"))
            _device_capabilities = parse_device_capabilities(pid);

        auto& depth_sensor = get_depth_sensor();
        auto& raw_depth_sensor = get_raw_depth_sensor();

        auto advanced_mode = is_camera_in_advanced_mode();

        using namespace platform;
        auto _usb_mode = usb3_type;
        std::string usb_type_str(usb_spec_names.at(_usb_mode));
        bool usb_modality = (_fw_version >= firmware_version("5.9.8.0"));
        if (usb_modality)
        {
            _usb_mode = raw_depth_sensor.get_usb_specification();
            if (usb_spec_names.count(_usb_mode) && (usb_undefined != _usb_mode))
                usb_type_str = usb_spec_names.at(_usb_mode);
            else  // Backend fails to provide USB descriptor  - occurs with RS3 build. Requires further work
                usb_modality = false;
        }

        if (_fw_version >= firmware_version("5.12.1.1"))
        {
            depth_sensor.register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_Z16H, RS2_STREAM_DEPTH));
        }

        if (advanced_mode && (_usb_mode >= usb3_type))
        {
            depth_sensor.register_processing_block(
                { {RS2_FORMAT_Y8I} },
                { {RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 1} , {RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 2} },
                []() { return std::make_shared<y8i_to_y8y8>(); }
            ); // L+R

            depth_sensor.register_processing_block(
                {RS2_FORMAT_Y12I},
                {{RS2_FORMAT_Y16, RS2_STREAM_INFRARED, 1}, {RS2_FORMAT_Y16, RS2_STREAM_INFRARED, 2}},
                []() {return std::make_shared<y12i_to_y16y16>(); }
            );
        }

        auto pid_hex_str = hexify(pid);

        if ((pid == RS416_PID || pid == RS416_RGB_PID) && _fw_version >= firmware_version("5.12.0.1"))
        {
            depth_sensor.register_option(RS2_OPTION_HARDWARE_PRESET,
                std::make_shared<uvc_xu_option<uint8_t>>(raw_depth_sensor, depth_xu, DS5_HARDWARE_PRESET,
                    "Hardware pipe configuration"));
            depth_sensor.register_option(RS2_OPTION_LED_POWER,
                std::make_shared<uvc_xu_option<uint16_t>>(raw_depth_sensor, depth_xu, DS5_LED_PWR,
                    "Set the power level of the LED, with 0 meaning LED off"));
        }

        if (_fw_version >= firmware_version("5.6.3.0"))
        {
            _is_locked = _hw_monitor->is_camera_locked(GVD, is_camera_locked_offset);

#ifdef HWM_OVER_XU
            //if hw_monitor was created by usb replace it with xu
            // D400_IMU will remain using USB interface due to HW limitations
            if ((group.usb_devices.size() > 0) && (RS400_IMU_PID != pid))
            {
                _hw_monitor = std::make_shared<hw_monitor>(
                    std::make_shared<locked_transfer>(
                        std::make_shared<command_transfer_over_xu>(
                            raw_depth_sensor, depth_xu, DS5_HWMONITOR),
                        raw_depth_sensor));
            }
#endif

            depth_sensor.register_pu(RS2_OPTION_GAIN);
            auto exposure_option = std::make_shared<uvc_xu_option<uint32_t>>(raw_depth_sensor,
                depth_xu,
                DS5_EXPOSURE,
                "Depth Exposure (usec)");
            depth_sensor.register_option(RS2_OPTION_EXPOSURE, exposure_option);

            auto enable_auto_exposure = std::make_shared<uvc_xu_option<uint8_t>>(raw_depth_sensor,
                depth_xu,
                DS5_ENABLE_AUTO_EXPOSURE,
                "Enable Auto Exposure");
            depth_sensor.register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, enable_auto_exposure);

            depth_sensor.register_option(RS2_OPTION_EXPOSURE,
                std::make_shared<auto_disabling_control>(
                    exposure_option,
                    enable_auto_exposure));
        }

        if (_fw_version >= firmware_version("5.5.8.0"))
        {
            depth_sensor.register_option(RS2_OPTION_OUTPUT_TRIGGER_ENABLED,
                std::make_shared<uvc_xu_option<uint8_t>>(raw_depth_sensor, depth_xu, DS5_EXT_TRIGGER,
                    "Generate trigger from the camera to external device once per frame"));

            auto error_control = std::unique_ptr<uvc_xu_option<uint8_t>>(new uvc_xu_option<uint8_t>(raw_depth_sensor, depth_xu, DS5_ERROR_REPORTING, "Error reporting"));

            _polling_error_handler = std::unique_ptr<polling_error_handler>(
                new polling_error_handler(1000,
                    std::move(error_control),
                    raw_depth_sensor.get_notifications_processor(),
                    std::unique_ptr<notification_decoder>(new ds5_notification_decoder())));

            depth_sensor.register_option(RS2_OPTION_ERROR_POLLING_ENABLED, std::make_shared<polling_errors_disable>(_polling_error_handler.get()));

            depth_sensor.register_option(RS2_OPTION_ASIC_TEMPERATURE,
                std::make_shared<asic_and_projector_temperature_options>(raw_depth_sensor,
                    RS2_OPTION_ASIC_TEMPERATURE));
        }

        // Alternating laser pattern is applicable for global shutter/active SKUs
        auto mask = d400_caps::CAP_GLOBAL_SHUTTER | d400_caps::CAP_ACTIVE_PROJECTOR;
        if ((_fw_version >= firmware_version("5.11.3.0")) && ((_device_capabilities & mask) == mask))
        {
            depth_sensor.register_option(RS2_OPTION_EMITTER_ON_OFF, std::make_shared<alternating_emitter_option>(*_hw_monitor, &raw_depth_sensor));
        }
        else if (_fw_version >= firmware_version("5.10.9.0") &&
            _fw_version.experimental()) // Not yet available in production firmware
        {
            depth_sensor.register_option(RS2_OPTION_EMITTER_ON_OFF, std::make_shared<emitter_on_and_off_option>(*_hw_monitor, &raw_depth_sensor));
        }

        if ((_fw_version >= firmware_version("5.12.1.0")) && ((_device_capabilities & d400_caps::CAP_GLOBAL_SHUTTER) == d400_caps::CAP_GLOBAL_SHUTTER))
        {
            depth_sensor.register_option(RS2_OPTION_EMITTER_ALWAYS_ON, std::make_shared<emitter_always_on_option>(*_hw_monitor, &depth_sensor));
        }

        if (_fw_version >= firmware_version("5.9.15.1"))
        {
            depth_sensor.register_option(RS2_OPTION_INTER_CAM_SYNC_MODE,
                std::make_shared<external_sync_mode>(*_hw_monitor));
        }

        roi_sensor_interface* roi_sensor = dynamic_cast<roi_sensor_interface*>(&depth_sensor);
        if (roi_sensor)
            roi_sensor->set_roi_method(std::make_shared<ds5_auto_exposure_roi_method>(*_hw_monitor));

        depth_sensor.register_option(RS2_OPTION_STEREO_BASELINE, std::make_shared<const_value_option>("Distance in mm between the stereo imagers",
            lazy<float>([this]() { return get_stereo_baseline_mm(); })));

        if (advanced_mode && _fw_version >= firmware_version("5.6.3.0"))
        {
            auto depth_scale = std::make_shared<depth_scale_option>(*_hw_monitor);
            auto depth_sensor = As<ds5_depth_sensor, synthetic_sensor>(&get_depth_sensor());
            assert(depth_sensor);

            depth_scale->add_observer([depth_sensor](float val)
            {
                depth_sensor->set_depth_scale(val);
            });

            depth_sensor->register_option(RS2_OPTION_DEPTH_UNITS, depth_scale);
        }
        else
            depth_sensor.register_option(RS2_OPTION_DEPTH_UNITS, std::make_shared<const_value_option>("Number of meters represented by a single depth unit",
                lazy<float>([]() { return 0.001f; })));
        // Metadata registration
        depth_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&uvc_header::timestamp));

        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_depth_mode, depth_y_mode) +
            offsetof(md_depth_y_normal_mode, intel_capture_timing);

        depth_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_rs400_sensor_ts_parser(make_uvc_header_parser(&uvc_header::timestamp),
            make_attribute_parser(&md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset)));

        // attributes of md_capture_stats
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_depth_mode, depth_y_mode) +
            offsetof(md_depth_y_normal_mode, intel_capture_stats);

        depth_sensor.register_metadata(RS2_FRAME_METADATA_WHITE_BALANCE, make_attribute_parser(&md_capture_stats::white_balance, md_capture_stat_attributes::white_balance_attribute, md_prop_offset));

        // attributes of md_depth_control
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_depth_mode, depth_y_mode) +
            offsetof(md_depth_y_normal_mode, intel_depth_control);

        depth_sensor.register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL, make_attribute_parser(&md_depth_control::manual_gain, md_depth_control_attributes::gain_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, make_attribute_parser(&md_depth_control::manual_exposure, md_depth_control_attributes::exposure_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE, make_attribute_parser(&md_depth_control::auto_exposure_mode, md_depth_control_attributes::ae_mode_attribute, md_prop_offset));

        depth_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER, make_attribute_parser(&md_depth_control::laser_power, md_depth_control_attributes::laser_pwr_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE, make_attribute_parser(&md_depth_control::emitterMode, md_depth_control_attributes::emitter_mode_attribute, md_prop_offset,
            [](const rs2_metadata_type& param) { return param == 1 ? 1 : 0; })); // starting at version 2.30.1 this control is superceeded by RS2_FRAME_METADATA_FRAME_EMITTER_MODE
        depth_sensor.register_metadata(RS2_FRAME_METADATA_EXPOSURE_PRIORITY, make_attribute_parser(&md_depth_control::exposure_priority, md_depth_control_attributes::exposure_priority_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_EXPOSURE_ROI_LEFT, make_attribute_parser(&md_depth_control::exposure_roi_left, md_depth_control_attributes::roi_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_EXPOSURE_ROI_RIGHT, make_attribute_parser(&md_depth_control::exposure_roi_right, md_depth_control_attributes::roi_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_EXPOSURE_ROI_TOP, make_attribute_parser(&md_depth_control::exposure_roi_top, md_depth_control_attributes::roi_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_EXPOSURE_ROI_BOTTOM, make_attribute_parser(&md_depth_control::exposure_roi_bottom, md_depth_control_attributes::roi_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_EMITTER_MODE, make_attribute_parser(&md_depth_control::emitterMode, md_depth_control_attributes::emitter_mode_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_LED_POWER, make_attribute_parser(&md_depth_control::ledPower, md_depth_control_attributes::led_power_attribute, md_prop_offset));

        // md_configuration - will be used for internal validation only
        md_prop_offset = offsetof(metadata_raw, mode) + offsetof(md_depth_mode, depth_y_mode) + offsetof(md_depth_y_normal_mode, intel_configuration);

        depth_sensor.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_HW_TYPE, make_attribute_parser(&md_configuration::hw_type, md_configuration_attributes::hw_type_attribute, md_prop_offset));
        depth_sensor.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_SKU_ID, make_attribute_parser(&md_configuration::sku_id, md_configuration_attributes::sku_id_attribute, md_prop_offset));
        depth_sensor.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_FORMAT, make_attribute_parser(&md_configuration::format, md_configuration_attributes::format_attribute, md_prop_offset));
        depth_sensor.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_WIDTH, make_attribute_parser(&md_configuration::width, md_configuration_attributes::width_attribute, md_prop_offset));
        depth_sensor.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_HEIGHT, make_attribute_parser(&md_configuration::height, md_configuration_attributes::height_attribute, md_prop_offset));
        depth_sensor.register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_ACTUAL_FPS,  std::make_shared<ds5_md_attribute_actual_fps> ());

        register_info(RS2_CAMERA_INFO_NAME, device_name);
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, optic_serial);
        register_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER, asic_serial);
        register_info(RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID, asic_serial);
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, _fw_version);
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, group.uvc_devices.front().device_path);
        register_info(RS2_CAMERA_INFO_DEBUG_OP_CODE, std::to_string(static_cast<int>(fw_cmd::GLD)));
        register_info(RS2_CAMERA_INFO_ADVANCED_MODE, ((advanced_mode) ? "YES" : "NO"));
        register_info(RS2_CAMERA_INFO_PRODUCT_ID, pid_hex_str);
        register_info(RS2_CAMERA_INFO_PRODUCT_LINE, "D400");
        register_info(RS2_CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION, _recommended_fw_version);
        register_info(RS2_CAMERA_INFO_CAMERA_LOCKED, _is_locked ? "YES" : "NO");
        
        if (usb_modality)
            register_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, usb_type_str);

        std::string curr_version= _fw_version;

    }

    notification ds5_notification_decoder::decode(int value)
    {
        if (ds::ds5_fw_error_report.find(static_cast<uint8_t>(value)) != ds::ds5_fw_error_report.end())
            return{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, value, RS2_LOG_SEVERITY_ERROR, ds::ds5_fw_error_report.at(static_cast<uint8_t>(value)) };

        return{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, value, RS2_LOG_SEVERITY_WARN, (to_string() << "D400 HW report - unresolved type " << value) };
    }

    void ds5_device::create_snapshot(std::shared_ptr<debug_interface>& snapshot) const
    {
        //TODO: Implement
    }
    void ds5_device::enable_recording(std::function<void(const debug_interface&)> record_action)
    {
        //TODO: Implement
    }

    platform::usb_spec ds5_device::get_usb_spec() const
    {
        if(!supports_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR))
            return platform::usb_undefined;
        auto str = get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR);
        for (auto u : platform::usb_spec_names)
        {
            if (u.second.compare(str) == 0)
                return u.first;
        }
        return platform::usb_undefined;
    }


    double ds5_device::get_device_time_ms()
    {
        //// TODO: Refactor the following query with an extension.
        //if (dynamic_cast<const platform::playback_backend*>(&(get_context()->get_backend())) != nullptr)
        //{
        //    throw not_implemented_exception("device time not supported for backend.");
        //}
        
#ifdef RASPBERRY_PI
        // TODO: This is temporary work-around since global timestamp seems to compromise RPi stability
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
#else
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
        return ts;
#endif
    }

    std::shared_ptr<synthetic_sensor> ds5u_device::create_ds5u_depth_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& all_device_infos)
    {
        using namespace ds;

        auto&& backend = ctx->get_backend();

        std::vector<std::shared_ptr<platform::uvc_device>> depth_devices;
        for (auto&& info : filter_by_mi(all_device_infos, 0)) // Filter just mi=0, DEPTH
            depth_devices.push_back(backend.create_uvc_device(info));

        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));
        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_metadata(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup)));

        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto raw_depth_ep = std::make_shared<uvc_sensor>(ds::DEPTH_STEREO, std::make_shared<platform::multi_pins_uvc_device>(depth_devices), std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(ds5_timestamp_reader_metadata), _tf_keeper, enable_global_time_option)), this);
        auto depth_ep = std::make_shared<ds5u_depth_sensor>(this, raw_depth_ep);

        depth_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        raw_depth_ep->register_xu(depth_xu); // make sure the XU is initialized every time we power the camera

        depth_ep->register_processing_block({ {RS2_FORMAT_W10} }, { {RS2_FORMAT_RAW10, RS2_STREAM_INFRARED, 1} }, []() { return std::make_shared<w10_converter>(RS2_FORMAT_RAW10); });
        depth_ep->register_processing_block({ {RS2_FORMAT_W10} }, { {RS2_FORMAT_Y10BPACK, RS2_STREAM_INFRARED, 1} }, []() { return std::make_shared<w10_converter>(RS2_FORMAT_Y10BPACK); });

        depth_ep->register_processing_block(processing_block_factory::create_pbf_vector<uyvy_converter>(RS2_FORMAT_UYVY, map_supported_color_formats(RS2_FORMAT_UYVY), RS2_STREAM_INFRARED));


        return depth_ep;
    }

    ds5u_device::ds5u_device(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        : ds5_device(ctx, group), device(ctx, group)
    {
        using namespace ds;

        // Override the basic ds5 sensor with the development version
        _depth_device_idx = assign_sensor(create_ds5u_depth_device(ctx, group.uvc_devices), _depth_device_idx);

        init(ctx, group);

        auto& depth_ep = get_depth_sensor();

        // Inhibit specific unresolved options
        depth_ep.unregister_option(RS2_OPTION_OUTPUT_TRIGGER_ENABLED);
        depth_ep.unregister_option(RS2_OPTION_ERROR_POLLING_ENABLED);
        depth_ep.unregister_option(RS2_OPTION_ASIC_TEMPERATURE);
        depth_ep.unregister_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);

        // Enable laser etc.
        auto pid = group.uvc_devices.front().pid;
        if (pid != RS_USB2_PID)
        {
            auto& depth_ep = get_raw_depth_sensor();
            auto emitter_enabled = std::make_shared<emitter_option>(depth_ep);
            depth_ep.register_option(RS2_OPTION_EMITTER_ENABLED, emitter_enabled);

            auto laser_power = std::make_shared<uvc_xu_option<uint16_t>>(depth_ep,
                depth_xu,
                DS5_LASER_POWER,
                "Manual laser power in mw. applicable only when laser power mode is set to Manual");
            depth_ep.register_option(RS2_OPTION_LASER_POWER,
                std::make_shared<auto_disabling_control>(
                    laser_power,
                    emitter_enabled,
                    std::vector<float>{0.f, 2.f}, 1.f));

            depth_ep.register_option(RS2_OPTION_PROJECTOR_TEMPERATURE,
                std::make_shared<asic_and_projector_temperature_options>(depth_ep,
                    RS2_OPTION_PROJECTOR_TEMPERATURE));
        }
    }

    processing_blocks get_ds5_depth_recommended_proccesing_blocks()
    {
        auto res = get_depth_recommended_proccesing_blocks();
        res.push_back(std::make_shared<threshold>());
        res.push_back(std::make_shared<disparity_transform>(true));
        res.push_back(std::make_shared<spatial_filter>());
        res.push_back(std::make_shared<temporal_filter>());
        res.push_back(std::make_shared<hole_filling_filter>());
        res.push_back(std::make_shared<disparity_transform>(false));
        return res;
    }

}
