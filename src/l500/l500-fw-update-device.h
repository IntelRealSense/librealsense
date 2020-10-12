// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "fw-update/fw-update-device.h"

namespace librealsense
{
    class l500_update_device : public update_device
    {
    public:
        static const uint16_t DFU_VERSION_MASK = 0xFE;
        static const uint16_t DFU_VERSION_VALUE = 0x4A; // On Units with old DFU payload can be 74/75 decimal

        // The L515 device EEPROM has different bytes order then D4xx device.
        // this struct overrides the generic serial_number_data struct at fw-update-device.h
        struct serial_number_data
        {
            uint8_t spare[2];
            uint8_t serial[6];
        };

        l500_update_device(std::shared_ptr<context> ctx, bool register_device_notifications, std::shared_ptr<platform::usb_device> usb_device);
        virtual ~l500_update_device() = default;

        void update(const void* fw_image, int fw_image_size, update_progress_callback_ptr = nullptr) const override;

    protected:
        virtual const std::string& get_name() const override { return _name; }
        virtual const std::string& get_product_line() const override { return _product_line; }
        virtual const std::string& get_serial_number() const override { return _serial_number; }
        std::string parse_serial_number(const std::vector<uint8_t>& buffer) const;

    private:
        std::string _name;
        std::string _product_line;
        std::string _serial_number;
    };
}
