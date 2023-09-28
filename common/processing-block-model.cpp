// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include <string>
#include "subdevice-model.h"
#include "processing-block-model.h"


namespace rs2
{
    processing_block_model::processing_block_model(
        subdevice_model* owner,
        const std::string& name,
        std::shared_ptr<rs2::filter> block,
        std::function<rs2::frame(rs2::frame)> invoker,
        std::string& error_message, bool enable)
        : _owner(owner), _name(name), _block(block), _invoker(invoker), _enabled(enable)
    {
        std::stringstream ss;
        ss << "##" << ((owner) ? owner->dev.get_info(RS2_CAMERA_INFO_NAME) : _name)
            << "/" << ((owner) ? (*owner->s).get_info(RS2_CAMERA_INFO_NAME) : "_")
            << "/" << (long long)this;

        if (_owner)
            _full_name = get_device_sensor_name(_owner) + "." + _name;
        else
            _full_name = _name;

        _enabled = restore_processing_block(_full_name.c_str(),
            block, _enabled);

        populate_options(ss.str().c_str(), owner, owner ? &owner->_options_invalidated : nullptr, error_message);
    }

    void processing_block_model::save_to_config_file()
    {
        save_processing_block_to_config_file(_full_name.c_str(), _block, _enabled);
    }

    option_model& processing_block_model::get_option(rs2_option opt)
    {
        if (options_metadata.find(opt) != options_metadata.end())
            return options_metadata[opt];

        std::string error_message;
        options_metadata[opt] = create_option_model(opt, get_name(), _owner, _block, _owner ? &_owner->_options_invalidated : nullptr, error_message);
        return options_metadata[opt];
    }

    void processing_block_model::populate_options(const std::string& opt_base_label,
        subdevice_model* model,
        bool* options_invalidated,
        std::string& error_message)
    {
        for (auto opt : _block->get_supported_options())
        {
            options_metadata[opt] = create_option_model(opt, opt_base_label, model, _block, model ? &model->_options_invalidated : nullptr, error_message);
        }
    }

    void save_processing_block_to_config_file(const char* name,
        std::shared_ptr<rs2::processing_block> pb, bool enable)
    {
        for (auto opt : pb->get_supported_options())
        {
            auto val = pb->get_option(opt);
            std::string key = name;
            key += ".";
            key += pb->get_option_name(opt);
            config_file::instance().set(key.c_str(), val);
        }

        std::string key = name;
        key += ".enabled";
        config_file::instance().set(key.c_str(), enable);
    }
}
