// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "notifications.h"

namespace rs2
{
    int parse_product_line(std::string id);
    std::string get_available_firmware_version(int product_line);
    std::map<int, std::vector<uint8_t>> create_default_fw_table();
    std::vector<int> parse_fw_version(const std::string& fw);
    bool is_upgradeable(const std::string& curr, const std::string& available);
    bool is_recommended_fw_available(std::string version);

    class firmware_update_manager : public process_manager
    {
    public:
        firmware_update_manager(device_model& model, device dev, context ctx, std::vector<uint8_t> fw, bool is_signed) 
            : process_manager("Firmware Update"), _model(model),
              _fw(fw), _is_signed(is_signed), _dev(dev), _ctx(ctx) {}

    private:
        void process_flow(std::function<void()> cleanup, 
            invoker invoke) override;
        bool check_for(
            std::function<bool()> action, std::function<void()> cleanup,
            std::chrono::system_clock::duration delta);

        device _dev;
        context _ctx;
        std::vector<uint8_t> _fw;
        bool _is_signed;
        device_model& _model;
    };

    struct fw_update_notification_model : public process_notification_model
    {
        fw_update_notification_model(std::string name,
            std::shared_ptr<firmware_update_manager> manager, bool expaned);

        void set_color_scheme(float t) const override;
        void draw_content(ux_window& win, int x, int y, float t, std::string& error_message) override;
        void draw_expanded(ux_window& win, std::string& error_message) override;
        int calc_height() override;
    };
}