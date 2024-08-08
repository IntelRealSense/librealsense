// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <src/frame.h>
#include "extension.h"


namespace librealsense {


class video_frame : public frame
{
public:
    video_frame()
        : frame()
        , _width( 0 )
        , _height( 0 )
        , _bpp( 0 )
        , _stride( 0 )
    {
    }

    int get_width() const { return _width; }
    int get_height() const { return _height; }
    int get_stride() const { return _stride; }
    int get_bpp() const { return _bpp; }

    void assign( int width, int height, int stride, int bpp )
    {
        _width = width;
        _height = height;
        _stride = stride;
        _bpp = bpp;
    }

private:
    int _width, _height, _bpp, _stride;
};

MAP_EXTENSION( RS2_EXTENSION_VIDEO_FRAME, librealsense::video_frame );


}  // namespace librealsense
