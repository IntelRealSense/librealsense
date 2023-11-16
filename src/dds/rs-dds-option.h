// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

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

public:
    typedef std::function< void( const std::string & name, float value ) > set_option_callback;
    typedef std::function< float( const std::string & name ) > query_option_callback;

private:
    set_option_callback _set_opt_cb;
    query_option_callback _query_opt_cb;

public:
    rs_dds_option( const std::shared_ptr< realdds::dds_option > & dds_opt,
                   set_option_callback set_opt_cb,
                   query_option_callback query_opt_cb );

    void set( float value ) override;

    float get_last_known_value() const;

    float query() const override;

    bool is_enabled() const override { return true; }
    const char * get_description() const override;
};


}  // namespace librealsense
