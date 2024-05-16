// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "d500-fw-update-device.h"
#include "d500-private.h"


namespace librealsense
{
ds_d500_update_device::ds_d500_update_device( std::shared_ptr< const device_info > const & dev_info,
                                    std::shared_ptr< platform::usb_device > const & usb_device )
    : update_device( dev_info, usb_device, "D500" )
    {
        auto info = usb_device->get_info();
        _name = ds::rs500_sku_names.find(info.pid) != ds::rs500_sku_names.end() ? ds::rs500_sku_names.at(info.pid) : "unknown";  
        _serial_number = parse_serial_number(_serial_number_buffer);
    }


    bool ds_d500_update_device::check_fw_compatibility(const std::vector<uint8_t>& image) const
    {
        // Currently we cannot extract FW version from HKR FW image
        return true;
    }

    std::string ds_d500_update_device::parse_serial_number(const std::vector<uint8_t>& buffer) const
    {
        if (buffer.size() != sizeof(serial_number_data))
            throw std::runtime_error("DFU - failed to parse serial number!");

        std::stringstream rv;
        for (auto i = 0; i < ds::module_serial_size; i++)
            rv << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(buffer[i]);

        return rv.str();
    }

    void ds_d500_update_device::update(const void* fw_image, int fw_image_size, rs2_update_progress_callback_sptr update_progress_callback) const
    {
        update_device::update( fw_image, fw_image_size, update_progress_callback );
    }

    bool ds_d500_update_device::wait_for_manifest_completion(std::shared_ptr<platform::usb_messenger> messenger, const rs2_dfu_state state,
        size_t timeout, rs2_update_progress_callback_sptr update_progress_callback) const
    {
        std::chrono::milliseconds elapsed_milliseconds;
        auto start = std::chrono::system_clock::now();
        rs2_dfu_state dfu_state = RS2_DFU_STATE_APP_IDLE;
        dfu_status_payload status;
        int percentage_of_transfer = 0;

        do {
            uint32_t transferred = 0;
            auto sts = messenger->control_transfer(0xa1 /*DFU_GETSTATUS_PACKET*/, RS2_DFU_GET_STATUS, 0, 0, (uint8_t*)&status, sizeof(status), transferred, 5000);
            dfu_state = status.get_state();
            percentage_of_transfer = (int)(status.iString);
            std::stringstream ss;
            ss << "DFU_GETSTATUS called, state is: " << to_string(dfu_state); 
            ss << ", iString equals: " << percentage_of_transfer << ", and bwPollTimeOut equals: " << status.bwPollTimeout << std::endl;
            LOG_DEBUG(ss.str().c_str());

            if (update_progress_callback)
            {
                auto progress_for_bar = compute_progress(static_cast<float>(percentage_of_transfer), 20.f, 100.f, 5.f) / 100.f;
                update_progress_callback->on_update_progress(progress_for_bar);
            }

            if (sts != platform::RS2_USB_STATUS_SUCCESS)
                LOG_DEBUG("control xfer error: " << to_string(sts));

            //test for dfu error state
            if (status.is_error_state()) {
                return false;
            }

            // FW doesn't set the bwPollTimeout value, therefore it is wrong to use status.bwPollTimeout
            std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TIMEOUT));

            auto curr = std::chrono::system_clock::now();
            elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(curr - start);
        } while (percentage_of_transfer < 100);

        return true;
    }
}
