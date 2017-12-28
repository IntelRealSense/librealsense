// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"

#include "option.h"
#include "context.h"
#include "proc/synthetic-stream.h"
#include "proc/decimation-filter.h"
#include "environment.h"

namespace librealsense
{
    const uint8_t decimation_min_val = 1;
    const uint8_t decimation_max_val = 5;
    const uint8_t decimation_default_val = 2;
    const uint8_t decimation_step = 1;    // The filter suppors kernel sizes [2^0...2^5]

    decimation_filter::decimation_filter() :
        _decimation_factor(decimation_default_val),
        _patch_size(0x1 << (uint8_t(decimation_default_val - 1))),
        _kernel_size(_patch_size*_patch_size),
         _recalc_profile(false)
    {
        auto decimation_control = std::make_shared<ptr_option<uint8_t>>(
            decimation_min_val,
            decimation_max_val,
            decimation_step,
            decimation_default_val,
            &_decimation_factor, "Decimation magnitude");
        decimation_control->on_set([this, decimation_control](float val)
        {
            if (!decimation_control->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported decimation factor value " << val << " is out of range.");

            // The decimation factor represents the 2's exponent
            _decimation_factor = static_cast<uint8_t>(val);
            _patch_size = 0x1 << uint8_t(_decimation_factor - 1);
            _kernel_size = _patch_size*_patch_size;
            _recalc_profile = true;
        });

        register_option(RS2_OPTION_FILTER_MAGNITUDE, decimation_control);

        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            rs2::frame out = f, tgt, depth;

            bool composite = f.is<rs2::frameset>();

            depth = (composite) ? f.as<rs2::frameset>().first_or_default(RS2_STREAM_DEPTH) : f;

            if (depth) // Processing required
            {
                update_output_profile(depth);
                if (tgt = prepare_target_frame(depth, source))
                {
                    auto src = depth.as<rs2::video_frame>();
                    decimate_depth(static_cast<const uint16_t*>(src.get_data()),
                        static_cast<uint16_t*>(const_cast<void*>(tgt.get_data())),
                        src.get_width(), src.get_height(), this->_patch_size);
                }
            }

            out = composite ? source.allocate_composite_frame({ tgt }) : tgt;

            source.frame_ready(out);
        };

        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }

    void  decimation_filter::update_output_profile(const rs2::frame& f)
    {
        if (f.get_profile().get() != _source_stream_profile.get())
        {
            _source_stream_profile = f.get_profile();
            _recalc_profile = true;
        }

        // Buld a new target profile for every system/filter change
        if (_recalc_profile)
        {
            // Verify decimation
            auto vp = _source_stream_profile.as<rs2::video_stream_profile>();
            if ((vp.width() % _patch_size) || (vp.height() % _patch_size))
                throw invalid_value_exception(to_string() << "Unsupported decimation patch: " << _patch_size
                    << " for frame size [" << vp.width() << "," << vp.height() << "]");

            _target_stream_profile = _source_stream_profile.clone(RS2_STREAM_DEPTH, 0, _source_stream_profile.format());
            environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*(stream_interface*)(_source_stream_profile.get()->profile), *(stream_interface*)(_target_stream_profile.get()->profile));
            auto src_vspi = dynamic_cast<video_stream_profile_interface*>(_source_stream_profile.get()->profile);
            auto tgt_vspi = dynamic_cast<video_stream_profile_interface*>(_target_stream_profile.get()->profile);
            rs2_intrinsics src_intrin   = src_vspi->get_intrinsics();
            rs2_intrinsics tgt_intrin   = tgt_vspi->get_intrinsics();
            tgt_intrin.width            = src_vspi->get_width()/_patch_size;
            tgt_intrin.height           = src_vspi->get_height()/_patch_size;
            tgt_intrin.fx               = src_intrin.fx/_patch_size;
            tgt_intrin.fy               = src_intrin.fy/_patch_size;
            tgt_intrin.ppx              = src_intrin.ppx/_patch_size;
            tgt_intrin.ppy              = src_intrin.ppy/_patch_size;

            tgt_vspi->set_intrinsics([tgt_intrin]() { return tgt_intrin; });
            tgt_vspi->set_dims(tgt_intrin.width, tgt_intrin.height);

            _recalc_profile = false;
        }
    }

    rs2::frame decimation_filter::prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source)
    {
        auto vf = f.as<rs2::video_frame>();
        rs2_extension tgt_type = f.is<rs2::disparity_frame>() ? RS2_EXTENSION_DISPARITY_FRAME : RS2_EXTENSION_DEPTH_FRAME;
        return source.allocate_video_frame(_target_stream_profile, f,
            vf.get_bytes_per_pixel(),
            vf.get_width() / _patch_size,
            vf.get_height() / _patch_size,
            vf.get_stride_in_bytes() / _patch_size,
            tgt_type);
    }


    void decimation_filter::decimate_depth(const uint16_t * frame_data_in, uint16_t * frame_data_out,
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
        auto wk_itr = wk_begin;
        std::vector<uint16_t*> pixel_raws(scale);
        uint16_t* block_start = const_cast<uint16_t*>(frame_data_in);

        //#pragma omp parallel for schedule(dynamic) //Using OpenMP to try to parallelise the loop
        //TODO SSE Optimizations
        for (int j = 0; j < height_out; j++)
        {
            uint16_t *p{};
            // Mark the beginning of each of the N lines that the filter will run upon
            for (size_t i = 0; i < pixel_raws.size(); i++)
                pixel_raws[i] = block_start + (width_in*i);

            for (size_t i = 0, chunk_offset = 0; i < width_out; i++)
            {
                wk_itr = wk_begin;
                // extract data the kernel to process
                for (size_t n = 0; n < scale; ++n)
                {
                    p = pixel_raws[n] + chunk_offset;
                    for (size_t m = 0; m < scale; ++m)
                    {
                        *wk_itr++ = *(p + m);
                        //*wk_itr++ = *(pixel_raws[n] + chunk_offset + m);
                        //working_kernel[n*scale + m] = *(pixel_raws[n] + chunk_offset + m);
                    }
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
}
