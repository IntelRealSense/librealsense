// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

namespace rsimpl2
{
    class extension_interface
    {
    public:
        virtual void* extend_to(rs2_extension_type extension_type) = 0;
    };
}