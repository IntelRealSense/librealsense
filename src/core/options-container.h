// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "options-interface.h"
#include "extension.h"

#include <librealsense2/h/rs_option.h>
#include <src/basics.h>
#include "enum-helpers.h"
#include <src/librealsense-exception.h>

#include <map>
#include <vector>
#include <functional>
#include <memory>


namespace librealsense {


class LRS_EXTENSION_API options_container : public virtual options_interface, public extension_snapshot
{
public:
    bool supports_option(rs2_option id) const override
    {
        auto it = _options.find(id);
        if (it == _options.end()) return false;
        return it->second->is_enabled();
    }

    option& get_option(rs2_option id) override
    {
        return const_cast<option&>(const_cast<const options_container*>(this)->get_option(id));
    }

    const option & get_option( rs2_option id ) const override;

    std::shared_ptr<option> get_option_handler(rs2_option id)
    {
        return (const_cast<const options_container*>(this)->get_option_handler(id));
    }

    std::shared_ptr<option> get_option_handler(rs2_option id) const
    {
        auto it = _options.find(id);
        return (it == _options.end() ? std::shared_ptr<option>(nullptr) : it->second);
    }

    void register_option(rs2_option id, std::shared_ptr<option> option)
    {
        _options[id] = option;
        _recording_function(*this);
    }

    void unregister_option(rs2_option id)
    {
        _options.erase(id);
    }

    void create_snapshot(std::shared_ptr<options_interface>& snapshot) const override
    {
        snapshot = std::make_shared<options_container>(*this);
    }

    void enable_recording(std::function<void(const options_interface&)> record_action) override
    {
        _recording_function = record_action;
    }

    void update( std::shared_ptr<extension_snapshot> ext ) override;

    std::vector<rs2_option> get_supported_options() const override;

    std::string const & get_option_name( rs2_option option ) const override;

protected:
    std::map<rs2_option, std::shared_ptr<option>> _options;
    std::function<void(const options_interface&)> _recording_function = [](const options_interface&) {};
};


}  // namespace librealsense
