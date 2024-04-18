// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "frame-interface.h"
#include <src/basics.h>

#include <string>
#include <utility>  // swap


namespace librealsense {


class LRS_EXTENSION_API frame_holder
{
public:
    frame_interface * frame = nullptr;

    frame_holder() = default;
    frame_holder( const frame_holder & other ) = delete;
    frame_holder( frame_holder && other ) { std::swap( frame, other.frame ); }
    // non-acquiring ctor: will assume the frame has already been acquired!
    frame_holder( frame_interface * const f ) { frame = f; }
    ~frame_holder() { reset(); }

    // return a new holder after acquiring the frame
    frame_holder clone() const { return acquire( frame ); }
    static frame_holder acquire( frame_interface * const f )
    {
        if( f )
            f->acquire();
        return frame_holder( f );
    }

    operator frame_interface *() const { return frame; }
    frame_interface * operator->() const { return frame; }
    operator bool() const { return frame != nullptr; }

    frame_holder & operator=( const frame_holder & other ) = delete;
    frame_holder & operator=( frame_holder && other )
    {
        reset();
        std::swap( frame, other.frame );
        return *this;
    }

    void reset()
    {
        if( frame )
        {
            frame->release();
            frame = nullptr;
        }
    }
};


std::ostream & operator<<( std::ostream &, const frame_holder & );


}  // namespace librealsense
