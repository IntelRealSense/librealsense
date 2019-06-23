// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "fw-update-device.h"
#include "../types.h"
#include "../context.h"
#include "../ds5/ds5-private.h"

#include <chrono>
#include <stdexcept>
#include <algorithm>
#include <thread>

namespace librealsense
{
    rs2_dfu_state update_device::get_dfu_state(std::shared_ptr<platform::usb_messenger> messenger) const
    {
        uint8_t state = RS2_DFU_STATE_DFU_ERROR;
        uint32_t transferred = 0;
        auto res = messenger->control_transfer(0xa1 /*DFU_GETSTATUS_PACKET*/, RS2_DFU_GET_STATE, 0, 0, &state, 1, transferred, 10);
        if (res == librealsense::platform::RS2_USB_STATUS_ACCESS)
            throw backend_exception("Permission Denied!\n"
                "This is often an indication of outdated or missing udev-rules.\n"
                "If using Debian package, run sudo apt-get install librealsense2-dkms\n"
                "If building from source, run ./scripts/setup_udev_rules.sh", 
                RS2_EXCEPTION_TYPE_BACKEND);
        return (rs2_dfu_state)state;
    }

    void update_device::detach(std::shared_ptr<platform::usb_messenger> messenger) const
    {
        auto timeout = 1000;
        uint32_t transferred = 0;
        messenger->control_transfer(0x21 /*DFU_DETACH_PACKET*/, RS2_DFU_DETACH, timeout, 0, NULL, 0, transferred, timeout);
    }

    bool update_device::wait_for_state(std::shared_ptr<platform::usb_messenger> messenger, const rs2_dfu_state state, size_t timeout) const 
    {
        std::chrono::milliseconds elapsed_milliseconds;
        auto start = std::chrono::system_clock::now();
        do {
            dfu_status_payload status;
            uint32_t transferred = 0;
            auto sts = messenger->control_transfer(0xa1 /*DFU_GETSTATUS_PACKET*/, RS2_DFU_GET_STATUS, 0, 0, (uint8_t*)&status, sizeof(status), transferred, 5000);

            if (status.is_in_state(state)) {
                return true;
            }

            //test for dfu error state
            if (status.is_error_state()) {
                return false;
            }

            // FW doesn't set the bwPollTimeout value, therefore it is wrong to use status.bwPollTimeout
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            auto curr = std::chrono::system_clock::now();
            elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(curr - start);
        } while (elapsed_milliseconds < std::chrono::milliseconds(timeout));

        return false;
    }

    update_device::update_device(const std::shared_ptr<context>& ctx, bool register_device_notifications, std::shared_ptr<platform::usb_device> usb_device)
        : _context(ctx), _usb_device(usb_device)
    {
        auto messenger = _usb_device->open();

        auto state = get_dfu_state(messenger);
        detach(messenger);
        state = get_dfu_state(messenger);

        if (state != RS2_DFU_STATE_DFU_IDLE)
            throw std::runtime_error("Cannot open recovery device!");

        dfu_fw_status_payload paylaod;
        uint32_t transferred = 0;
        auto sts = messenger->control_transfer(0xa1, RS2_DFU_UPLOAD, 0, 0, reinterpret_cast<uint8_t*>(&paylaod), sizeof(paylaod), transferred, 10);
        if (sts != platform::RS2_USB_STATUS_SUCCESS)
            throw std::runtime_error("Failed to read serial number from recovery device!");

        std::stringstream formattedBuffer;
        for (auto i = 0; i < sizeof(paylaod.serial_number.serial); i++)
            formattedBuffer << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(paylaod.serial_number.serial[i]);

        _asic_serial_number = formattedBuffer.str();
    }

    update_device::~update_device()
    {

    }

    //since we currently don't have serial number that we can use to identify the device
    //this method looks for changes in the DFU device list
    bool update_device::wait_for_device(int mask, uint32_t timeout) const
    {
        auto dfu_dev_count = _context->query_devices(mask).size();
        if (dfu_dev_count == 0)
            return true;

        auto begin = std::chrono::system_clock::now();
        while (dfu_dev_count == _context->query_devices(mask).size())
        {
            auto end = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            if (duration > timeout)
                break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void update_device::update(const void* fw_image, int fw_image_size, fw_update_progress_callback_ptr update_progress_callback) const
    {
        auto messenger = _usb_device->open();

        const size_t transfer_size = 1024;

        size_t remaining_bytes = fw_image_size;
        uint16_t blocks_count = fw_image_size / transfer_size;
        uint16_t block_number = 0;

        size_t offset = 0;
        uint32_t transferred = 0;

        while (remaining_bytes > 0) 
        {
            size_t chunk_size = std::min(transfer_size, remaining_bytes);

            auto curr_block = ((uint8_t*)fw_image + offset);
            auto sts = messenger->control_transfer(0x21 /*DFU_DOWNLOAD_PACKET*/, RS2_DFU_DOWNLOAD, block_number, 0, curr_block, chunk_size, transferred, 1000000);
            if(sts != platform::RS2_USB_STATUS_SUCCESS || !wait_for_state(messenger, RS2_DFU_STATE_DFU_DOWNLOAD_IDLE, 100000))
                throw std::runtime_error("Failed to download firmware");

            block_number++;
            remaining_bytes -= chunk_size;
            offset += chunk_size;

            float progress = (float)block_number / (float)blocks_count;
            LOG_DEBUG("fw update progress: " << progress);
            if (update_progress_callback)
                update_progress_callback->on_fw_update_progress(progress);
        }

        // After the final block of firmware has been sent to the device and the status solicited, the host sends a
        // DFU_DNLOAD request with the wLength field cleared to 0 and then solicits the status again.If the
        // result indicates that the device is ready and there are no errors, then the Transfer phase is complete and
        // the Manifestation phase begins.
        auto sts = messenger->control_transfer(0x21 /*DFU_DOWNLOAD_PACKET*/, RS2_DFU_DOWNLOAD, block_number, 0, NULL, 0, transferred, 10);
        if (sts != platform::RS2_USB_STATUS_SUCCESS)
            throw std::runtime_error("Failed to send final FW packet");

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

    sensor_interface& update_device::get_sensor(size_t i)
    { 
        throw std::runtime_error("try to get sensor from fw loader device");
    }

    const sensor_interface& update_device::get_sensor(size_t i) const
    {
        throw std::runtime_error("try to get sensor from fw loader device");
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
        return _context;
    }

    platform::backend_device_group update_device::get_device_data() const
    {
        throw std::runtime_error("get_device_data is not supported by update_device");//TODO_MK
    }

    std::pair<uint32_t, rs2_extrinsics> update_device::get_extrinsics(const stream_interface& stream) const
    {
        throw std::runtime_error("get_extrinsics is not supported by update_device");
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
        case RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER: return get_asic_serial_number();
        case RS2_CAMERA_INFO_NAME: return get_name();
        case RS2_CAMERA_INFO_PRODUCT_LINE: return get_product_line();
        default:
            throw std::runtime_error(std::string(rs2_camera_info_to_string(info)) + " is not supported by fw update device");
        }
    }

    bool update_device::supports_info(rs2_camera_info info) const
    {
        switch (info)
        {
        case RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER:
        case RS2_CAMERA_INFO_NAME:
        case RS2_CAMERA_INFO_PRODUCT_LINE:return true;
        default: return false;
        }
    }

    void update_device::create_snapshot(std::shared_ptr<info_interface>& snapshot) const
    {
        
    }
    void update_device::enable_recording(std::function<void(const info_interface&)> record_action)
    {
        
    }
}
