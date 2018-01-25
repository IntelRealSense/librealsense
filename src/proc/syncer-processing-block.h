// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "types.h"
#include "archive.h"

#include <stdint.h>
#include <vector>
#include <mutex>
#include <memory>

namespace librealsense
{
    class processing_block;
    class timestamp_composite_matcher;
    class syncer_process_unit : public processing_block
    {
    public:
        syncer_process_unit();

        ~syncer_process_unit()
        {
            _matcher.reset();
        }
    private:
        std::unique_ptr<timestamp_composite_matcher> _matcher;
        std::mutex _mutex;
    };
}
