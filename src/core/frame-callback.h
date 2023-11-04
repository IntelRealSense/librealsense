// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/hpp/rs_types.hpp>
#include <memory>
#include <functional>


namespace librealsense {


class frame_interface;


struct frame_callback : public rs2_frame_callback
{
    using fn = std::function< void( frame_interface * ) >;

    explicit frame_callback( fn && on_frame )
        : _on_frame( std::move( on_frame ) )
    {
    }

    void on_frame( rs2_frame * fref ) override { _on_frame( (frame_interface *)( fref ) ); }

    void release() override { delete this; }

private:
    fn _on_frame;
};


inline rs2_frame_callback_sptr make_frame_callback( std::function< void( frame_interface * ) > && callback )
{
    return { new frame_callback( std::move( callback ) ),
             []( rs2_frame_callback * p ) { p->release(); } };
}


}  // namespace librealsense
