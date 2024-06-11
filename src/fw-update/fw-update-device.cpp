// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.

#include "fw-update-device.h"
#include "../types.h"
#include "../context.h"
#include "../device-info.h"
#include "ds/d400/d400-private.h"

#include <rsutils/string/hexdump.h>

#include <chrono>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <string>
#include <regex>
#include <fstream>

#define DEFAULT_TIMEOUT 100
#define FW_UPDATE_INTERFACE_NUMBER 0
namespace librealsense
{
    std::string get_formatted_fw_version(uint32_t fw_last_version)
    {
        uint8_t* ptr = (uint8_t*)(&fw_last_version);
        std::vector<uint8_t> buffer(ptr, ptr + sizeof(fw_last_version));
        std::stringstream fw_version;
        std::string delimiter = "";
        for (auto i = 1; i <= buffer.size(); i++)
        {
            fw_version << delimiter << static_cast<int>(buffer[buffer.size() - i]);
            delimiter = ".";
        }
        return fw_version.str();
    }

    rs2_dfu_state update_device::get_dfu_state(std::shared_ptr<platform::usb_messenger> messenger) const
    {
        uint8_t state = RS2_DFU_STATE_DFU_ERROR;
        uint32_t transferred = 0;
        auto res = messenger->control_transfer(0xa1 /*DFU_GETSTATUS_PACKET*/, RS2_DFU_GET_STATE, 0, 0, &state, 1, transferred, DEFAULT_TIMEOUT);
        if (res == librealsense::platform::RS2_USB_STATUS_ACCESS)
            throw backend_exception("Permission Denied!\n"
                "This is often an indication of outdated or missing udev-rules.\n"
                "If using Debian package, run sudo apt-get install librealsense2-dkms\n"
                "If building from source, run ./scripts/setup_udev_rules.sh",
                RS2_EXCEPTION_TYPE_BACKEND);
        return res == platform::RS2_USB_STATUS_SUCCESS ? (rs2_dfu_state)state : RS2_DFU_STATE_DFU_ERROR;
    }

    void update_device::detach(std::shared_ptr<platform::usb_messenger> messenger) const
    {
        auto timeout = 1000;
        uint32_t transferred = 0;
        auto res = messenger->control_transfer(0x21 /*DFU_DETACH_PACKET*/, RS2_DFU_DETACH, timeout, 0, NULL, 0, transferred, timeout);
        if (res != platform::RS2_USB_STATUS_SUCCESS)
            LOG_WARNING("DFU - failed to detach device");
    }

    void update_device::read_device_info(std::shared_ptr<platform::usb_messenger> messenger)
    {
        auto state = get_dfu_state(messenger);

        if (state != RS2_DFU_STATE_DFU_IDLE)
            throw std::runtime_error("DFU detach failed!");

        dfu_fw_status_payload payload;
        uint32_t transferred = 0;
        auto sts = messenger->control_transfer(0xa1, RS2_DFU_UPLOAD, 0, 0, reinterpret_cast<uint8_t*>(&payload), sizeof(payload), transferred, DEFAULT_TIMEOUT);
        if (sts != platform::RS2_USB_STATUS_SUCCESS)
            throw std::runtime_error("Failed to read info from DFU device!");

        _serial_number_buffer = std::vector<uint8_t>(sizeof(payload.serial_number));
        _serial_number_buffer.assign((uint8_t*)&payload.serial_number, (uint8_t*)&payload.serial_number + sizeof(payload.serial_number));
        _is_dfu_locked = payload.dfu_is_locked != 0;
        _highest_fw_version = get_formatted_fw_version(payload.fw_highest_version);
        _last_fw_version = get_formatted_fw_version(payload.fw_last_version);

        std::string lock_status = _is_dfu_locked ? "device is locked" : "device is unlocked";
        LOG_INFO("This device is in DFU mode, previously-installed firmware: " << _last_fw_version <<
            ", the highest firmware ever installed: " << _highest_fw_version);

        LOG_INFO("DFU status: " << lock_status << " , DFU version is: " << payload.dfu_version);
    }

    std::string update_device::to_string(rs2_dfu_state state) const
    {
        switch (state)
        {
        case(RS2_DFU_STATE_APP_IDLE):
            return "APP_IDLE";
        case(RS2_DFU_STATE_APP_DETACH):
            return "APP_DETACH";
        case(RS2_DFU_STATE_DFU_DOWNLOAD_SYNC):
            return "DFU_DOWNLOAD_SYNC";
        case(RS2_DFU_STATE_DFU_DOWNLOAD_BUSY):
            return "DFU_DOWNLOAD_BUSY";
        case(RS2_DFU_STATE_DFU_DOWNLOAD_IDLE):
            return "DFU_DOWNLOAD_IDLE";
        case(RS2_DFU_STATE_DFU_MANIFEST_SYNC):
            return "DFU_MANIFEST_SYNC";
        case(RS2_DFU_STATE_DFU_MANIFEST):
            return "DFU_MANIFEST";
        case(RS2_DFU_STATE_DFU_MANIFEST_WAIT_RESET):
            return "DFU_MANIFEST_WAIT_RESET";
        case(RS2_DFU_STATE_DFU_UPLOAD_IDLE):
            return "DFU_UPLOAD_IDLE";
        case(RS2_DFU_STATE_DFU_ERROR):
            return "DFU_ERROR";
        default:
            return "DFU_STATE_???";
        }
    }

    bool update_device::wait_for_state(std::shared_ptr<platform::usb_messenger> messenger, const rs2_dfu_state state, size_t timeout) const
    {
        std::chrono::milliseconds elapsed_milliseconds;
        auto start = std::chrono::system_clock::now();
        do {
            dfu_status_payload status;
            uint32_t transferred = 0;
            auto sts = messenger->control_transfer(0xa1 /*DFU_GETSTATUS_PACKET*/, RS2_DFU_GET_STATUS, 0, 0, (uint8_t*)&status, sizeof(status), transferred, 5000);

            if (sts != platform::RS2_USB_STATUS_SUCCESS)
                return false;

            if (status.is_in_state(state)) {
                return true;
            }

            //test for dfu error state
            if (status.is_error_state()) {
                return false;
            }

            // FW doesn't set the bwPollTimeout value, therefore it is wrong to use status.bwPollTimeout
            std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TIMEOUT));

            auto curr = std::chrono::system_clock::now();
            elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(curr - start);
        } while (elapsed_milliseconds < std::chrono::milliseconds(timeout));

        return false;
    }

    float update_device::compute_progress(float progress, float start, float end, float threshold) const
    {
        if (threshold < 1.f)
            throw std::invalid_argument("Avoid division by zero");
        return start + (ceil(progress * threshold) / threshold) * (end - start) / 100.f;
    }

    update_device::update_device( std::shared_ptr< const device_info > const & dev_info,
                                  std::shared_ptr< platform::usb_device > const & usb_device,
                                  const std::string & product_line )
        : _dev_info( dev_info )
        , _usb_device( usb_device )
        , _physical_port( usb_device->get_info().id )
        , _pid( rsutils::string::from() << std::uppercase << rsutils::string::hexdump( usb_device->get_info().pid ))
        , _product_line( product_line )
    {
        if (auto messenger = _usb_device->open(FW_UPDATE_INTERFACE_NUMBER))
        {
            auto state = get_dfu_state(messenger);
            LOG_DEBUG("DFU state is: " << state);
            if (state != RS2_DFU_STATE_DFU_IDLE)
                detach(messenger);

            read_device_info(messenger);
        }
        else
        {
            std::stringstream s;
            s << "access failed for " << std::hex <<  _usb_device->get_info().vid << ":"
              <<_usb_device->get_info().pid << " uid: " <<  _usb_device->get_info().id << std::dec;
            LOG_ERROR(s.str().c_str());
            throw std::runtime_error(s.str().c_str());
        }
    }

    update_device::update_device( std::shared_ptr< const device_info > const & dev_info,
                                  std::shared_ptr< platform::mipi_device > const & mipi_device,
                                  const std::string & product_line )
        : _dev_info( dev_info )
        , _mipi_device( mipi_device )
        , _physical_port( mipi_device->get_info().dfu_device_path )
        , _pid( rsutils::string::from() << std::uppercase << rsutils::string::hexdump( mipi_device->get_info().pid ))
        , _product_line( product_line )
        , _serial_number( mipi_device->get_info().serial_number )
    {
        std::ifstream fw_path_in_device(_physical_port.c_str());
        if (!fw_path_in_device)
        {
            throw std::runtime_error("Firmware Update failed - wrong path or permissions missing");
            return;
        }
        fw_path_in_device.close();
    }

    update_device::~update_device()
    {

    }

    void update_device::update_usb(const void* fw_image, int fw_image_size, rs2_update_progress_callback_sptr update_progress_callback) const
    {
        // checking fw compatibility (covering the case of recovery device with wrong product line fw )
        std::vector<uint8_t> buffer((uint8_t*)fw_image, (uint8_t*)fw_image + fw_image_size);
        if (!check_fw_compatibility(buffer))
            throw librealsense::invalid_value_exception("Device: " + get_serial_number() + " failed to update firmware\nImage is unsupported for this device or corrupted");

        auto messenger = _usb_device->open(FW_UPDATE_INTERFACE_NUMBER);

        const size_t transfer_size = 1024;

        size_t remaining_bytes = fw_image_size;
        uint16_t blocks_count = uint16_t( fw_image_size / transfer_size );
        uint16_t block_number = 0;

        size_t offset = 0;
        uint32_t transferred = 0;
        int retries = 10;

        while (remaining_bytes > 0)
        {
            size_t chunk_size = std::min(transfer_size, remaining_bytes);

            auto curr_block = ((uint8_t*)fw_image + offset);
            auto sts = messenger->control_transfer(0x21 /*DFU_DOWNLOAD_PACKET*/, RS2_DFU_DOWNLOAD, block_number, 0, curr_block, uint32_t(chunk_size), transferred, 5000);
            if (sts != platform::RS2_USB_STATUS_SUCCESS || !wait_for_state(messenger, RS2_DFU_STATE_DFU_DOWNLOAD_IDLE, 1000))
            {
                auto state = get_dfu_state(messenger);
                // the update process may be interrupted by another thread that trys to create another fw_update_device.
                // this retry mechanism should overcome such scenario.
                // we limit the number of retries in order to avoid infinite loop.
                if (state == RS2_DFU_STATE_DFU_IDLE && retries--)
                    continue;

                auto sn = get_serial_number();
                if(_is_dfu_locked)
                    throw std::runtime_error("Device: " + sn  + " is locked for update.\nUse firmware version higher than: " + _highest_fw_version);
                else if (state == RS2_DFU_STATE_DFU_ERROR)
                    throw std::runtime_error("Device: " + sn + " failed to update firmware\nImage is unsupported for this device or corrupted");
                else
                    throw std::runtime_error("Device: " + sn + " failed to download firmware\nPlease verify that no other librealsense application is running");
            }

            block_number++;
            remaining_bytes -= chunk_size;
            offset += chunk_size;

            float progress = (float)block_number / (float)blocks_count;
            LOG_DEBUG("fw update progress: " << progress);
            if (update_progress_callback)
            {
                auto progress_for_bar = compute_progress(progress, 0.f, 20.f, 5.f) / 100.f;
                update_progress_callback->on_update_progress(progress_for_bar);
            }
        }

        // After the final block of firmware has been sent to the device and the status solicited, the host sends a
        // DFU_DNLOAD request with the wLength field cleared to 0 and then solicits the status again.If the
        // result indicates that the device is ready and there are no errors, then the Transfer phase is complete and
        // the Manifestation phase begins.
        auto sts = messenger->control_transfer(0x21 /*DFU_DOWNLOAD_PACKET*/, RS2_DFU_DOWNLOAD, block_number, 0, NULL, 0, transferred, DEFAULT_TIMEOUT);
        if (sts != platform::RS2_USB_STATUS_SUCCESS)
            throw std::runtime_error("Failed to send final FW packet");

        dfu_manifest_phase(messenger, update_progress_callback);
    }

    void update_device::dfu_manifest_phase(const platform::rs_usb_messenger& messenger, rs2_update_progress_callback_sptr update_progress_callback) const
    {
        // After the zero length DFU_DNLOAD request terminates the Transfer
        // phase, the device is ready to manifest the new firmware. As described
        // previously, some devices may accumulate the firmware image and perform
        // the entire reprogramming operation at one time. Others may have only a
        // small amount remaining to be reprogrammed, and still others may have
        // none. Regardless, the device enters the dfuMANIFEST-SYNC state and
        // awaits the solicitation of the status report by the host. Upon receipt
        // of the anticipated DFU_GETSTATUS, the device enters the dfuMANIFEST
        // state, where it completes its reprogramming operations.

        // WaitForDFU state sends several DFU_GETSTATUS requests, until we hit
        // either RS2_DFU_STATE_DFU_MANIFEST_WAIT_RESET or RS2_DFU_STATE_DFU_ERROR status.
        // This command also reset the device
        if (!wait_for_state(messenger, RS2_DFU_STATE_DFU_MANIFEST_WAIT_RESET, 20000))
            throw std::runtime_error("Firmware manifest failed");
    }

    void update_device::update_mipi(const void* fw_image, int fw_image_size, rs2_update_progress_callback_sptr update_progress_callback) const
    {
        // checking fw compatibility (covering the case of recovery device with wrong product line fw )
        std::vector<uint8_t> buffer((uint8_t*)fw_image, (uint8_t*)fw_image + fw_image_size);
        const size_t transfer_size = 1024;

        size_t remaining_bytes = fw_image_size;
        uint16_t blocks_count = uint16_t( fw_image_size / transfer_size );
        uint16_t block_number = 0;

        size_t offset = 0;
        uint32_t transferred = 0;

        if (!check_fw_compatibility(buffer))
            throw librealsense::invalid_value_exception("Device: " + get_serial_number() + " failed to update firmware\nImage is unsupported for this device or corrupted");
        // Write signed firmware to appropriate file descriptor

        std::ofstream fw_path_in_device(_physical_port.c_str(), std::ios::binary);
        if (!fw_path_in_device)
        {
            throw std::runtime_error("Firmware Update failed - wrong path or permissions missing");
            return;
        }
        while (remaining_bytes > 0)
        {
            size_t chunk_size = std::min(transfer_size, remaining_bytes);

            auto curr_block = ((uint8_t*)fw_image + offset);

            fw_path_in_device.write(reinterpret_cast<const char*>(curr_block), chunk_size);

            block_number++;
            remaining_bytes -= chunk_size;
            offset += chunk_size;

            float progress = (float)block_number / (float)blocks_count;
            LOG_DEBUG("fw update progress: " << progress);
            if (update_progress_callback)
                update_progress_callback->on_update_progress(progress);
        }
        LOG_INFO("Firmware Update for MIPI device done.");
        fw_path_in_device.close();
    }

    void update_device::update(const void* fw_image, int fw_image_size, rs2_update_progress_callback_sptr update_progress_callback) const
    {
        if(_pid == "ABCD" || _pid == "BBCD")
        {
            update_mipi(fw_image, fw_image_size, update_progress_callback);
        }
        else
        {
            update_usb(fw_image, fw_image_size, update_progress_callback);
        }
    }

    sensor_interface& update_device::get_sensor(size_t i)
    {
        throw std::runtime_error("try to get sensor from fw loader device");
    }

    const sensor_interface& update_device::get_sensor(size_t i) const
    {
        throw std::runtime_error("update_device does not support get_sensor API");
    }

    size_t update_device::get_sensors_count() const
    {
        return 0;
    }

    void update_device::hardware_reset()
    {
        //TODO_MK
    }

    std::shared_ptr<matcher> update_device::create_matcher(const frame_holder& frame) const
    {
        return nullptr;
    }

    std::shared_ptr<context> update_device::get_context() const
    {
        return _dev_info->get_context();
    }

    std::shared_ptr< const device_info > update_device::get_device_info() const
    {
        return _dev_info;
    }

    std::pair<uint32_t, rs2_extrinsics> update_device::get_extrinsics(const stream_interface& stream) const
    {
        throw std::runtime_error("update_device does not support get_extrinsics API");
    }

    bool update_device::is_valid() const
    {
        return true;
    }

    std::vector<tagged_profile> update_device::get_profiles_tags() const
    {
        return std::vector<tagged_profile>();
    }

    void update_device::tag_profiles(stream_profiles profiles) const
    {

    }

    bool update_device::compress_while_record() const
    {
        return false;
    }

    const std::string& update_device::get_info(rs2_camera_info info) const
    {
        switch (info)
        {
        case RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID:    return get_serial_number();
        case RS2_CAMERA_INFO_NAME:                  return get_name();
        case RS2_CAMERA_INFO_PRODUCT_LINE:          return get_product_line();
        case RS2_CAMERA_INFO_PHYSICAL_PORT:         return _physical_port;
        case RS2_CAMERA_INFO_PRODUCT_ID:            return _pid;
        case RS2_CAMERA_INFO_FIRMWARE_VERSION:
            if( ! _last_fw_version.empty() )
                return _last_fw_version;
            // fall-thru
        default:
            throw std::runtime_error("update_device does not support " + std::string(rs2_camera_info_to_string(info)));
        }
    }

    bool update_device::supports_info(rs2_camera_info info) const
    {
        switch (info)
        {
        case RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID:
        case RS2_CAMERA_INFO_NAME:
        case RS2_CAMERA_INFO_PRODUCT_LINE:
        case RS2_CAMERA_INFO_PHYSICAL_PORT:
        case RS2_CAMERA_INFO_PRODUCT_ID:
            return true;

        case RS2_CAMERA_INFO_FIRMWARE_VERSION:
            return ! _last_fw_version.empty();

        default:
            return false;
        }
    }

    void update_device::create_snapshot(std::shared_ptr<info_interface>& snapshot) const
    {

    }
    void update_device::enable_recording(std::function<void(const info_interface&)> record_action)
    {

    }
}
