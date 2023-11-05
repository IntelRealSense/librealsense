// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "recommended-proccesing-blocks-interface.h"
#include "extension.h"


namespace librealsense
{
    class recommended_proccesing_blocks_base : public virtual recommended_proccesing_blocks_interface, public virtual recordable<recommended_proccesing_blocks_interface>
    {
    public:
        recommended_proccesing_blocks_base(recommended_proccesing_blocks_interface* owner)
            :_owner(owner)
        {}

        virtual processing_blocks get_recommended_processing_blocks() const override { return _owner->get_recommended_processing_blocks(); };

        virtual void create_snapshot(std::shared_ptr<recommended_proccesing_blocks_interface>& snapshot) const override
        {
            snapshot = std::make_shared<recommended_proccesing_blocks_snapshot>(get_recommended_processing_blocks());
        }

        virtual void enable_recording(std::function<void(const recommended_proccesing_blocks_interface&)> recording_function)  override {}

    private:
        recommended_proccesing_blocks_interface* _owner;
    };


}
