// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "notifications.h"


namespace rs2
{
    class safety_mcu_update_manager : public process_manager
    {
    public:
        safety_mcu_update_manager(std::shared_ptr<notifications_model> not_model, device_model& model, device dev, context ctx, std::vector<uint8_t> smcu, bool is_signed)
            : process_manager("Safety MCU Update"), _not_model(not_model), _model(model),
            _smcu(smcu), _is_signed(is_signed), _dev(dev), _ctx(ctx) {}

        const device_model& get_device_model() const { return _model; }
        std::shared_ptr<notifications_model> get_protected_notification_model() { return _not_model.lock(); };

    protected:
        void process_flow(std::function<void()> cleanup,
            invoker invoke) override;
        bool check_for(
            std::function<bool()> action, std::function<void()> cleanup,
            std::chrono::system_clock::duration delta);

        std::weak_ptr<notifications_model> _not_model;
        device _dev;
        context _ctx;
        std::vector<uint8_t> _smcu;
        bool _is_signed;
        device_model& _model;
    };

    struct safety_mcu_update_notification_model : public process_notification_model
    {
        safety_mcu_update_notification_model(std::string name,
            std::shared_ptr<safety_mcu_update_manager> manager, bool expaned);

        void set_color_scheme(float t) const override;
        void draw_content(ux_window& win, int x, int y, float t, std::string& error_message) override;
        void draw_expanded(ux_window& win, std::string& error_message) override;
        int calc_height() override;
    };
}
