// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023-4 Intel Corporation. All Rights Reserved.
#pragma once

#include "stream-profile.h"
#include <rsutils/type/fourcc.h>
#include <ostream>


namespace librealsense {
namespace platform {


// Both UVC and HID sensors implement video-/motion-stream-profiles and need to provide an implementation of the backend
// profile with which the sensor needs to be opened.
//
template< class super >
class stream_profile_impl : public super
{
    stream_profile _backend_stream_profile;

public:
    explicit stream_profile_impl( platform::stream_profile const & bsp )
        : _backend_stream_profile( bsp )
    {
    }

    stream_profile const & get_backend_profile() const override { return _backend_stream_profile; }

    void to_stream( std::ostream & os ) const override
    {
        os << " [";
        os << _backend_stream_profile.width << 'x' << _backend_stream_profile.height;
        os << ' ' << rsutils::type::fourcc( _backend_stream_profile.format );
        os << " @ " << _backend_stream_profile.fps << " Hz";
        os << "]";
    }
};


}  // namespace platform
}  // namespace librealsense
