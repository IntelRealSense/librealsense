// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "synthetic-source-interface.h"

#include <librealsense2/hpp/rs_types.hpp>
#include <memory>
#include <functional>


namespace librealsense {


struct frame_processor_callback : public rs2_frame_processor_callback
{
    using fn = std::function< void( frame_holder &&, synthetic_source_interface * ) >;

    explicit frame_processor_callback( fn && on_frame )
        : _on_frame( std::move( on_frame ) )
    {
    }

    void on_frame( rs2_frame * f, rs2_source * source ) override
    {
        frame_holder front( (frame_interface *)f );
        _on_frame( std::move( front ), source->source );
    }

    void release() override { delete this; }

private:
    fn _on_frame;
};


inline rs2_frame_processor_callback_sptr
make_frame_processor_callback( frame_processor_callback::fn && callback )
{
    return { new frame_processor_callback( std::move( callback ) ),
             []( rs2_frame_processor_callback * p ) { p->release(); } };
}


}  // namespace librealsense
