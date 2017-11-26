// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#include "../include/librealsense2/rs.hpp"

#include "option.h"
#include "proc/synthetic-stream.h"
#include "proc/decimation_filter.h"

void downsample_depth(const uint16_t * frame_data_in, uint16_t * frame_data_out, size_t width_in, size_t height_in, size_t scale);

namespace librealsense
{
    decimation_filter::decimation_filter():
        _decimation_filter(rs2_median),
        _decimation_factor(5),
        _kernel_size(_decimation_factor*_decimation_factor),
        _width(0), _height(0), _enable_filter(true)
    {
        auto decimation_control = std::make_shared<ptr_option<uint8_t>>(1, 5, 1, 4, &_decimation_factor, "Decimation magnitude");
        decimation_control->on_set([this](float val)
        {
            _kernel_size = 0x1 << uint8_t(val-1);
            _kernel_size *= _kernel_size;
        });
        register_option(RS2_OPTION_FILTER_MAGNITUDE, decimation_control);
        unregister_option(RS2_OPTION_FRAMES_QUEUE_SIZE);

        // TODO - a candidate to base class refactoring
        auto enable_control = std::make_shared<ptr_option<bool>>(false,true,true,true, &_enable_filter, "Apply decimation");
        register_option(RS2_OPTION_FILTER_ENABLED, enable_control);

        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            rs2::frame out = f, tgt, depth;

            if (this->_enable_filter)
            {
                bool composite = f.is<rs2::frameset>();

                depth = (composite) ? f.as<rs2::frameset>().first_or_default(RS2_STREAM_DEPTH) : f;

                if (depth) // Processing required
                {
                    update_internals(depth);
                    if (tgt = prepare_target_frame(depth, source))
                    {
                        auto src = depth.as<rs2::video_frame>();
                        downsample_depth(static_cast<const uint16_t*>(src.get_data()),
                            static_cast<uint16_t*>(const_cast<void*>(tgt.get_data())),
                            src.get_width(), src.get_height(), this->_decimation_factor);
                    }
                }

                out = composite ? source.allocate_composite_frame({ tgt }) : tgt;
            }

            source.frame_ready(out);
        };

        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }

    void  decimation_filter::update_internals(const rs2::frame& f)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_source_stream_profile)
        {
            _source_stream_profile = rs2::stream_profile(f.get_profile().clone(RS2_STREAM_DEPTH, 0, RS2_FORMAT_Z16));
            _target_stream_profile = _source_stream_profile;
        }
        auto vf = f.as<rs2::video_frame>();
        auto vp = _target_stream_profile.as<rs2::video_stream_profile>();

        if ((vp.width()%_decimation_factor) || (vp.height() % _decimation_factor))
            throw invalid_value_exception(to_string() << "Unsupported decimation factor: " << _decimation_factor
                << " for frame size [" << vp.width() << "," << vp.height() << "]");

        //TODO re-generate video_stream_profile with correct intrinsic data
    }

    rs2::frame decimation_filter::prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source)
    {
        auto vf = f.as<rs2::video_frame>();
        return source.allocate_video_frame(_target_stream_profile, f,
            vf.get_bytes_per_pixel(),
            vf.get_width() / _decimation_factor,
            vf.get_height()/ _decimation_factor,
            vf.get_stride_in_bytes() / _decimation_factor,
            RS2_EXTENSION_DEPTH_FRAME);
    }
}

void downsample_depth(const uint16_t * frame_data_in, uint16_t * frame_data_out,
    size_t width_in, size_t height_in, size_t scale)
{
    // The frame size must be a multiple of the filter's kernel unit
    assert(0 == (width_in%scale));
    assert(0 == (height_in%scale));

    auto width_out = width_in / scale;
    auto height_out = height_in / scale;
    auto kernel_size = scale * scale;

    // Use median filtering
    std::vector<uint16_t> working_kernel(kernel_size);
    auto wk_begin = working_kernel.data();
    std::vector<uint16_t*> pixel_raws(scale);
    uint16_t* block_start = const_cast<uint16_t*>(frame_data_in);

    //#pragma omp parallel for schedule(dynamic) //Using OpenMP to try to parallelise the loop
    //TODO Evgeni
    for (int j = 0; j < height_out; j++)
    {
        // Mark the beginning of each of the N lines that the filter will run upon
        for (size_t i = 0; i < pixel_raws.size(); i++)
            pixel_raws[i] = block_start + (width_in*i);

        for (size_t i = 0, chunk_offset=0; i < width_out; i++)
        {
            // extract data the kernel to prrocess
            for (size_t n = 0; n < scale; ++n)
            {
                for (size_t m = 0; m < scale; ++m)
                    working_kernel[n*scale+m] = *(pixel_raws[n]+ chunk_offset +m);
            }

            std::nth_element(wk_begin, wk_begin + (kernel_size / 2), wk_begin + kernel_size);
            //std::sort(working_kernel.begin(),working_kernel.end());
            *frame_data_out++ = working_kernel[kernel_size / 2];

            chunk_offset += scale;
        }

        // Skip N lines to the beginnig of the next processing segment
        block_start += width_in*scale;
    }
}
