// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "streaming.h"
#include "archive.h"

namespace librealsense
{
    std::string frame_holder_to_string(const frame_holder & f)
    {
        return frame_to_string(*f.frame);
    }

    std::string frame_to_string(const frame_interface & f)
    {
        std::ostringstream s;
        auto composite = dynamic_cast<const composite_frame *>(&f);
        if (composite)
        {
            s << "[";
            for (int i = 0; i < composite->get_embedded_frames_count(); i++)
            {
                s << *composite->get_frame(i);
            }
            s << "]";
        }
        else
        {
            s << "[" << f.get_stream()->get_stream_type();
            s << " " << f.get_stream()->get_unique_id();
            s << " " << f.get_frame_number();
            s << " " << std::fixed << (double)f.get_frame_timestamp();
            s << "]";
        }
        return s.str();
    }
}

