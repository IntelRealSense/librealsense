// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "auto-exposure-processor.h"

librealsense::auto_exposure_processor::auto_exposure_processor(rs2_stream stream, enable_auto_exposure_option& enable_ae_option)
    : auto_exposure_processor("Auto Exposure Processor", stream, enable_ae_option) {}

librealsense::auto_exposure_processor::auto_exposure_processor(const char * name, rs2_stream stream, enable_auto_exposure_option& enable_ae_option)
    : generic_processing_block(name),
      _stream(stream),
      _enable_ae_option(enable_ae_option) {}

bool librealsense::auto_exposure_processor::should_process(const rs2::frame & frame)
{
    return _enable_ae_option.to_add_frames() && _stream == RS2_STREAM_FISHEYE;
}

rs2::frame librealsense::auto_exposure_processor::process_frame(const rs2::frame_source & source, const rs2::frame & f)
{
    // We dont actually modify the frame, only calculate and process the exposure values.
    auto&& fi = (frame_interface*)f.get();
    ((librealsense::frame*)fi)->additional_data.fisheye_ae_mode = true;

    fi->acquire();
    auto&& auto_exposure = _enable_ae_option.get_auto_exposure();
    if (auto_exposure)
        auto_exposure->add_frame(fi);

    return f;
}
