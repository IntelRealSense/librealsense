// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "rs-dds-depth-sensor-proxy.h"
#include "rs-dds-option.h"

#include <realdds/topics/dds-topic-names.h>

#include <src/librealsense-exception.h>
#include <rsutils/json.h>


namespace librealsense {


float dds_depth_sensor_proxy::get_depth_scale() const
{
    if( auto opt = get_option_handler( RS2_OPTION_DEPTH_UNITS ) )
        return opt->get_value();

    // Rather than throwing an exception, we try to be a little more helpful: without depth units, the depth image will
    // show black, prompting bug reports. The D400 units min/max are taken from the HW, but the default is set to:
    return 0.001f;
}


void dds_depth_sensor_proxy::add_no_metadata( frame * const f, streaming_impl & streaming )
{
    f->additional_data.depth_units = get_depth_scale();
    super::add_no_metadata( f, streaming );
}


void dds_depth_sensor_proxy::add_frame_metadata( frame * const f, rsutils::json const & dds_md, streaming_impl & streaming )
{
    if( auto du = dds_md.nested( realdds::topics::metadata::key::header, realdds::topics::metadata::header::key::depth_units ) )
    {
        try
        {
            f->additional_data.depth_units = du.get< float >();
        }
        catch( rsutils::json::exception const & )
        {
            f->additional_data.depth_units = get_depth_scale();
        }
    }
    else
    {
        f->additional_data.depth_units = get_depth_scale();
    }

    super::add_frame_metadata( f, dds_md, streaming );
}


}  // namespace librealsense
