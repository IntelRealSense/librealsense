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

        if (!_is_dfu_monitoring_enabled)
        {
            LOG_DEBUG("Waiting for the FW to be burnt");
            static constexpr int D500_FW_DFU_TIME = 120; // [sec]
            report_progress_and_wait_for_fw_burn(update_progress_callback, D500_FW_DFU_TIME);
        }
    }

    bool ds_d500_update_device::wait_for_manifest_completion(std::shared_ptr<platform::usb_messenger> messenger, const rs2_dfu_state state,
        std::chrono::seconds timeout_seconds, rs2_update_progress_callback_sptr update_progress_callback) const
    {
        std::chrono::seconds elapsed_seconds;
        auto start = std::chrono::system_clock::now();
        rs2_dfu_state dfu_state = RS2_DFU_STATE_APP_IDLE;
        dfu_status_payload status;
        int percentage_of_transfer = 0;
        int iteration = 0;
        // progress should start increase even in the 2nd iteration, 
        // when this DFU progress is enabled by FW
        int max_iteration_number_for_progress_start = 10;

        do {
            uint32_t transferred = 0;
            auto sts = messenger->control_transfer(0xa1 /*DFU_GETSTATUS_PACKET*/, RS2_DFU_GET_STATUS, 0, 0, (uint8_t*)&status, sizeof(status), transferred, 5000);
            dfu_state = status.get_state();
            percentage_of_transfer = static_cast<int>(status.iString);

            // the below code avoids process stuck when using a d5XX device, 
            // which has a fw version without the DFU progress feature
            if (percentage_of_transfer == 0 &&
                ++iteration == max_iteration_number_for_progress_start)
            {
                _is_dfu_monitoring_enabled = false;
                return true;
            }

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
                LOG_ERROR("control xfer error: " << platform::usb_status_to_string[sts]);

            //test for dfu error state
            if (status.is_error_state()) {
                return false;
            }

            // FW doesn't set the bwPollTimeout value, therefore it is wrong to use status.bwPollTimeout
            std::this_thread::sleep_for(std::chrono::seconds(1));

            auto curr = std::chrono::system_clock::now();
            elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(curr - start);
            if (elapsed_seconds > timeout_seconds)
            {
                LOG_ERROR("DFU in MANIFEST STATUS Timeout");
                return false;
            }
        } while (percentage_of_transfer < 100 && dfu_state == RS2_DFU_STATE_DFU_MANIFEST);

        return true;
    }

    void ds_d500_update_device::dfu_manifest_phase(const platform::rs_usb_messenger& messenger, rs2_update_progress_callback_sptr update_progress_callback) const
    {
        // measuring the progress of the writing to flash (when enabled by FW)
        if (!wait_for_manifest_completion(messenger, RS2_DFU_STATE_DFU_MANIFEST, std::chrono::seconds(200), update_progress_callback))
            throw std::runtime_error("Firmware manifest completion failed");


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
        if (_is_dfu_monitoring_enabled)
        {
            if (!wait_for_state(messenger, RS2_DFU_STATE_DFU_MANIFEST_WAIT_RESET, 20000))
                throw std::runtime_error("Firmware manifest failed");
        }
    }

    void ds_d500_update_device::report_progress_and_wait_for_fw_burn(rs2_update_progress_callback_sptr update_progress_callback, int required_dfu_time) const
    {
        // We calculate the sleep time needed for each cycle to get to 100% progress bar
        if (update_progress_callback)
        {
            float iteration_sleep_time_ms = (static_cast<float>(required_dfu_time) / 100.0f) * 1000.0f;
            for (int i = 1; i <= 100; i++)
            {
                auto percentage_of_transfer = i;
                auto progress_for_bar = compute_progress(static_cast<float>(percentage_of_transfer), 20.f, 100.f, 5.f) / 100.f;
                update_progress_callback->on_update_progress(progress_for_bar);
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(iteration_sleep_time_ms)));
            }
        }
        else
            std::this_thread::sleep_for(std::chrono::seconds(required_dfu_time));
    }
    float ds_d500_update_device::compute_progress(float progress, float start, float end, float threshold) const
    {
        if (threshold < 1.f)
            throw std::invalid_argument("Avoid division by zero");
        return start + (ceil(progress * threshold) / threshold) * (end - start) / 100.f;
    }
}
