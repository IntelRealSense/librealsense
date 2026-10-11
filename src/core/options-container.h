// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-24 RealSense, Inc. All Rights Reserved.
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
        auto it = _options_by_id.find( id );
        if( it == _options_by_id.end() )
            return false;
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

    std::shared_ptr<option> get_option_handler( rs2_option id ) const
    {
        auto it = _options_by_id.find( id );
        if( it == _options_by_id.end() )
            return {};
        return it->second;
    }

    void register_option( rs2_option id, std::shared_ptr< option > option );
    void unregister_option( rs2_option id );

    // Composite-option twin of the eight methods above, operating on a SEPARATE map
    // (_composite_options_by_id) - a fully independent registry, never merged with the scalar
    // one. See src/composite-option-interface.h / include/librealsense2/h/rs_composite_option.h.
    bool supports_composite_option(rs2_composite_option_id id) const override
    {
        auto it = _composite_options_by_id.find( id );
        if( it == _composite_options_by_id.end() )
            return false;
        return it->second->is_enabled();
    }

    composite_option_interface& get_composite_option(rs2_composite_option_id id) override
    {
        return const_cast<composite_option_interface&>(const_cast<const options_container*>(this)->get_composite_option(id));
    }

    const composite_option_interface & get_composite_option( rs2_composite_option_id id ) const override;

    std::shared_ptr<composite_option_interface> get_composite_option_handler(rs2_composite_option_id id)
    {
        return (const_cast<const options_container*>(this)->get_composite_option_handler(id));
    }

    std::shared_ptr<composite_option_interface> get_composite_option_handler( rs2_composite_option_id id ) const
    {
        auto it = _composite_options_by_id.find( id );
        if( it == _composite_options_by_id.end() )
            return {};
        return it->second;
    }

    void register_composite_option( rs2_composite_option_id id, std::shared_ptr< composite_option_interface > option );
    void unregister_composite_option( rs2_composite_option_id id );

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

    std::vector<rs2_composite_option_id> get_supported_composite_options() const override;

    std::string const & get_composite_option_name( rs2_composite_option_id option ) const override;

    virtual rsutils::subscription register_options_changed_callback(options_watcher::callback&& cb) override
    { return rsutils::subscription(); }

protected:
    std::vector< rs2_option > _ordered_options;
    std::map< rs2_option, std::shared_ptr< option > > _options_by_id;

    std::vector< rs2_composite_option_id > _ordered_composite_options;
    std::map< rs2_composite_option_id, std::shared_ptr< composite_option_interface > > _composite_options_by_id;

    std::function<void(const options_interface&)> _recording_function = [](const options_interface&) {};
};


}  // namespace librealsense
