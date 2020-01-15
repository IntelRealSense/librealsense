// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include <map>
#include "../include/librealsense2/h/rs_option.h"
#include "extension.h"
#include "types.h"

namespace librealsense
{
    struct LRS_EXTENSION_API option_range
    {
        float min;
        float max;
        float step;
        float def;
    };

    class LRS_EXTENSION_API option : public recordable<option>
    {
    public:
        virtual void set(float value) = 0;
        virtual float query() const = 0;
        virtual option_range get_range() const = 0;
        virtual bool is_enabled() const = 0;
        virtual bool is_read_only() const { return false; }
        virtual const char* get_description() const = 0;
        virtual const char* get_value_description(float) const { return nullptr; }
        virtual void create_snapshot(std::shared_ptr<option>& snapshot) const;

        virtual ~option() = default;
    };

    class options_interface : public recordable<options_interface>
    {
    public:
        virtual option& get_option(rs2_option id) = 0;
        virtual const option& get_option(rs2_option id) const = 0;
        virtual bool supports_option(rs2_option id) const = 0;
        virtual std::vector<rs2_option> get_supported_options() const = 0;
        virtual const char* get_option_name(rs2_option) const = 0;
        virtual ~options_interface() = default;
    };

    MAP_EXTENSION(RS2_EXTENSION_OPTIONS, librealsense::options_interface);

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

        const option& get_option(rs2_option id) const override
        {
            auto it = _options.find(id);
            if (it == _options.end())
            {
                throw invalid_value_exception(to_string()
                    << "Device does not support option "
                    << get_option_name(id) << "!");
            }
            return *it->second;
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

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
            auto ctr = As<options_container>(ext);
            if (!ctr) return;
            for(auto&& opt : ctr->_options)
            {
                _options[opt.first] = opt.second;
            }
        }

        std::vector<rs2_option> get_supported_options() const override;

        virtual const char* get_option_name(rs2_option option) const override
        {
            return get_string(option);
        }

    protected:
        std::map<rs2_option, std::shared_ptr<option>> _options;
        std::function<void(const options_interface&)> _recording_function = [](const options_interface&) {};
    };
}
