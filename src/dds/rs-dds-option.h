// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <src/option.h>

#include <functional>
#include <memory>


namespace realdds {
class dds_option;
}  // namespace realdds


namespace librealsense {


// A facade for a realdds::dds_option exposing librealsense interface
class rs_dds_option : public option_base
{
    std::shared_ptr< realdds::dds_option > _dds_opt;
    rs2_option_type const _rs_type;

public:
    typedef std::function< void( rsutils::json value ) > set_option_callback;
    typedef std::function< rsutils::json() > query_option_callback;

private:
    set_option_callback _set_opt_cb;
    query_option_callback _query_opt_cb;

public:
    rs_dds_option( const std::shared_ptr< realdds::dds_option > & dds_opt,
                   set_option_callback set_opt_cb,
                   query_option_callback query_opt_cb );

    rsutils::json get_value() const noexcept override;
    rs2_option_type get_value_type() const noexcept override { return _rs_type; }
    void set_value( rsutils::json ) override;

    void set( float value ) override;

    float query() const override;

    bool is_read_only() const override;
    bool is_enabled() const override;
    const char * get_description() const override;
    const char * get_value_description( float ) const override;
};


}  // namespace librealsense
