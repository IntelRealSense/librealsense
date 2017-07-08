// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "align.h"

namespace librealsense
{
    void processing_block::set_processing_callback(frame_processor_callback callback)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _callback = callback;
    }

    void processing_block::set_output_callback(frame_callback_ptr callback)
    {
        _source.set_callback(callback);
    }

    void processing_block::invoke(frame_holder frame)
    {
        if (frame)
        {
            auto callback = _source.begin_callback();
            try
            {
                if (_callback)
                {
                    rs2_frame* ref = nullptr;
                    std::swap(frame.frame, ref);
                    _source.invoke_callback(_callback->on_frame(ref, nullptr));
                }
            }
            catch(...)
            {
                LOG_ERROR("Exception was thrown during user processing callback!");
            }
        }
    }
}

