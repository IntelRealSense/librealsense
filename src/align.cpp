// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "../include/librealsense/rs2.hpp"

#include "core/video.h"
#include "align.h"
#include "archive.h"

namespace librealsense
{
    void processing_block::set_processing_callback(frame_processor_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _callback = callback;
    }

    void processing_block::set_output_callback(frame_callback_ptr callback)
    {
        _source.set_callback(callback);
    }

    processing_block::processing_block(std::shared_ptr<platform::time_service> ts)
            : _source(ts), _source_wrapper(_source)
    {
        _source.init(std::make_shared<metadata_parser_map>());
    }

    void processing_block::invoke(frame_holder f)
    {
        auto callback = _source.begin_callback();
        try
        {
            if (_callback)
            {
                frame_interface* ptr = nullptr;
                std::swap(f.frame, ptr);

                _callback->on_frame((rs2_frame*)ptr, _source_wrapper.get_c_wrapper());
            }
        }
        catch(...)
        {
            LOG_ERROR("Exception was thrown during user processing callback!");
        }
    }

    void synthetic_source::frame_ready(frame_holder result)
    {
        _actual_source.invoke_callback(std::move(result));
    }

    frame_interface* synthetic_source::allocate_video_frame(rs2_stream stream, frame_interface* original,
                                                          rs2_format new_format,
                                                          int new_bpp,
                                                          int new_width,
                                                          int new_height,
                                                          int new_stride)
    {
        video_frame* vf = nullptr;

        if (new_bpp == 0 || (new_width == 0 && new_stride == 0) || new_height == 0)
        {
            // If the user wants to delegate width, height and etc to original frame, it must be a video frame
            if (!rs2_is_frame((rs2_frame*)original, RS2_EXTENSION_TYPE_VIDEO_FRAME, nullptr))
            {
                throw std::runtime_error("If original frame is not video frame, you must specify new bpp, width/stide and height!");
            }
            vf = (video_frame*)original;
        }

        frame_additional_data data{};
        data.stream_type = stream;
        data.format = (new_format == RS2_FORMAT_ANY) ? original->get_format() : new_format;
        data.frame_number = original->get_frame_number();
        data.timestamp = original->get_frame_timestamp();
        data.timestamp_domain = original->get_frame_timestamp_domain();
        data.fps = original->get_framerate();
        data.metadata_size = 0;
        data.system_time = _actual_source.get_time();

        int width = new_width;
        int height = new_height;
        int bpp = new_bpp * 8;
        int stride = new_stride;

        if (bpp == 0)
        {
            bpp = vf->get_bpp();
        }

        if (width == 0 && stride == 0)
        {
            width = vf->get_width();
            stride = width * bpp / 8;
        }
        else if (width == 0)
        {
            width = stride * 8 / bpp;
        }
        else if (stride == 0)
        {
            stride = width * bpp / 8;
        }

        if (height == 0)
        {
            height = vf->get_height();
        }

        auto res = _actual_source.alloc_frame(RS2_EXTENSION_TYPE_VIDEO_FRAME, stride * height, data, true);
        if (!res) throw wrong_api_call_sequence_exception("Out of frame resources!");
        vf = (video_frame*)res;
        vf->assign(width, height, stride, bpp);
        vf->set_sensor(original->get_sensor());

        return res;
    }

    int get_embeded_frames_size(frame_interface* f)
    {
        if (f == nullptr) return 0;
        if (auto c = dynamic_cast<composite_frame*>(f))
            return c->get_embeded_frames_count();
        return 1;
    }

    void copy_frames(frame_holder from, frame_interface**& target)
    {
        if (auto comp = dynamic_cast<composite_frame*>(from.frame))
        {
            auto frame_buff = comp->get_frames();
            for (int i = 0; i < comp->get_embeded_frames_count(); i++)
            {
                std::swap(*target, frame_buff[i]);
                target++;
            }
            from.frame->disable_continuation();
        }
        else
        {
            *target = nullptr; // "move" the frame ref into target
            std::swap(*target, from.frame);
            target++;
        }
    }

    frame_interface* synthetic_source::allocate_composite_frame(std::vector<frame_holder> holders)
    {
        frame_additional_data d {};

        auto req_size = 0;
        for (auto&& f : holders)
            req_size += get_embeded_frames_size(f.frame);

        auto res = _actual_source.alloc_frame(RS2_EXTENSION_TYPE_COMPOSITE_FRAME, req_size * sizeof(rs2_frame*), d, true);
        if (!res) throw wrong_api_call_sequence_exception("Out of frame resources!");
        auto cf = (composite_frame*)res;

        auto frames = cf->get_frames();
        for (auto&& f : holders)
            copy_frames(std::move(f), frames);
        frames -= req_size;

        auto releaser = [frames, req_size]()
        {
            for (int i = 0; i < req_size; i++)
            {
                frames[i]->release();
                frames[i] = nullptr;
            }
        };
        frame_continuation release_frames(releaser, nullptr);
        cf->attach_continuation(std::move(release_frames));

        return res;
    }

    pointcloud::pointcloud(std::shared_ptr<platform::time_service> ts,
                           const rs2_intrinsics* depth_intrinsics,
                           const float* depth_units,
                           rs2_stream mapped_stream_type,
                           const rs2_intrinsics* mapped_intrinsics,
                           const rs2_extrinsics* extrinsics)
        : processing_block(ts),
          _depth_intrinsics_ptr(depth_intrinsics),
          _depth_units_ptr(depth_units),
          _mapped_intrinsics_ptr(mapped_intrinsics),
          _extrinsics_ptr(extrinsics),
          _expected_mapped_stream(mapped_stream_type)
    {
        if (depth_intrinsics) _depth_intrinsics = *depth_intrinsics;
        if (depth_units) _depth_units = *depth_units;
        if (mapped_intrinsics) _mapped_intrinsics = *mapped_intrinsics;
        if (extrinsics) _extrinsics = *extrinsics;

        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            std::lock_guard<std::mutex> lock(_mutex);

            auto on_depth_frame = [this](const rs2::frame& depth)
            {
                auto depth_frame = (frame_interface*)depth.get();
                std::lock_guard<std::mutex> lock(_mutex);

                bool found_depth_intrinsics = false;
                bool found_depth_units = false;

                if (!_depth_intrinsics_ptr)
                {
                    auto sensor = depth_frame->get_sensor();
                    if (auto video = dynamic_cast<video_sensor_interface*>(sensor.get()))
                    {
                        if (auto vf = dynamic_cast<video_frame*>(depth_frame))
                        {
                            stream_profile sp {
                                    vf->get_stream_type(),
                                    (uint32_t)vf->get_width(), (uint32_t)vf->get_height(), (uint32_t)vf->get_framerate(),
                                    vf->get_format() };

                            _depth_intrinsics = video->get_intrinsics(sp);
                            _depth_intrinsics_ptr = &_depth_intrinsics;
                            found_depth_intrinsics = true;
                        }
                    }
                }

                if (!_depth_units_ptr)
                {
                    auto sensor = depth_frame->get_sensor();
                    _depth_units = sensor->get_option(RS2_OPTION_DEPTH_UNITS).query();
                    _depth_units_ptr = &_depth_units;
                    found_depth_units = true;
                }

                if (found_depth_units != found_depth_intrinsics)
                {
                    throw wrong_api_call_sequence_exception("Received depth frame that doesn't provide either intrinsics or depth units!");
                }
            };

            auto on_other_frame = [this](const rs2::frame& other)
            {
                auto other_frame = (frame_interface*)other.get();
                std::lock_guard<std::mutex> lock(_mutex);

                bool assigned_stream_type = false;
                if (_expected_mapped_stream == RS2_STREAM_ANY)
                {
                    _expected_mapped_stream = other.get_stream_type();
                    assigned_stream_type = true;
                }

                if (_expected_mapped_stream == other.get_stream_type())
                {
                    bool found_mapped_intrinsics = false;
                    bool found_extrinsics = false;

                    if (!_mapped_intrinsics_ptr)
                    {
                        auto sensor = other_frame->get_sensor();
                        if (auto video = dynamic_cast<video_sensor_interface*>(sensor.get()))
                        {
                            if (auto vf = dynamic_cast<video_frame*>(other_frame))
                            {
                                stream_profile sp {
                                        vf->get_stream_type(),
                                        (uint32_t)vf->get_width(), (uint32_t)vf->get_height(), (uint32_t)vf->get_framerate(),
                                        vf->get_format() };

                                _mapped_intrinsics = video->get_intrinsics(sp);
                                _mapped_intrinsics_ptr = &_mapped_intrinsics;
                                found_mapped_intrinsics = true;
                            }
                        }
                        if (!found_mapped_intrinsics && assigned_stream_type)
                        {
                            // If unable to get extrinsics for this stream, undo stream assignment
                            _expected_mapped_stream = RS2_STREAM_ANY;
                        }
                    }

                    if (!_extrinsics_ptr)
                    {
                        auto sensor = other_frame->get_sensor();
                        if (sensor)
                        {
                            auto&& device = sensor->get_device();
                            size_t mapped_idx = -1;
                            size_t depth_idx = -1;
                            for (size_t i = 0; i < device.get_sensors_count(); i++)
                            {
                                if (&device.get_sensor(i) == sensor.get())
                                {
                                    mapped_idx = i;
                                }
                                if (device.get_sensor(i).supports_option(RS2_OPTION_DEPTH_UNITS))
                                {
                                    depth_idx = i;
                                }
                            }

                            if (mapped_idx != -1 && depth_idx != -1)
                            {
                                _extrinsics = device.get_extrinsics(depth_idx, RS2_STREAM_DEPTH, mapped_idx, _expected_mapped_stream);
                                _extrinsics_ptr = &_extrinsics;
                                found_extrinsics = true;
                            }
                        }
                        if (!found_extrinsics && assigned_stream_type)
                        {
                            // If unable to get extrinsics for this stream, undo stream assignment
                            _expected_mapped_stream = RS2_STREAM_ANY;
                        }
                    }
                }
            };

            if (auto composite = f.as<rs2::composite_frame>())
            {
                auto depth = composite.first_or_default(RS2_STREAM_DEPTH);
                if (depth)
                {
                    on_depth_frame(depth);
                }
                else
                {
                    composite.foreach(on_other_frame);
                }
            }
            else
            {
                if (f.get_stream_type() == RS2_STREAM_DEPTH)
                {
                    on_depth_frame(f);
                }
                else
                {
                    on_other_frame(f);
                }
            }


        };
        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }

    //void colorize::set_color_map(rs2_color_map cm)
    //{
    //    std::lock_guard<std::mutex> lock(_mutex);
    //    switch(cm)
    //    {
    //    case RS2_COLOR_MAP_CLASSIC:
    //        _cm = &classic;
    //        break;
    //    case RS2_COLOR_MAP_JET:
    //        _cm = &jet;
    //        break;
    //    case RS2_COLOR_MAP_HSV:
    //        _cm = &hsv;
    //        break;
    //    default:
    //        _cm = &classic;
    //    }
    //}

    //void colorize::histogram_equalization(bool enable)
    //{
    //    std::lock_guard<std::mutex> lock(_mutex);
    //    _equalize = enable;
    //}

    //colorize::colorize(std::shared_ptr<uvc::time_service> ts)
    //    : processing_block(RS2_EXTENSION_TYPE_VIDEO_FRAME, ts), _cm(&classic), _equalize(true)
    //{
    //    auto on_frame = [this](std::vector<rs2::frame> frames, const rs2::frame_source& source)
    //    {
    //        std::lock_guard<std::mutex> lock(_mutex);

    //        for (auto&& f : frames)
    //        {
    //            if (f.get_stream_type() == RS2_STREAM_DEPTH)
    //            {
    //                const auto max_depth = 0x10000;

    //                static uint32_t histogram[max_depth];
    //                memset(histogram, 0, sizeof(histogram));

    //                auto vf = f.as<video_frame>();
    //                auto width = vf.get_width();
    //                auto height = vf.get_height();

    //                auto depth_image = vf.get_frame_data();

    //                for (auto i = 0; i < width*height; ++i) ++histogram[depth_image[i]];
    //                for (auto i = 2; i < max_depth; ++i) histogram[i] += histogram[i - 1]; // Build a cumulative histogram for the indices in [1,0xFFFF]
    //                for (auto i = 0; i < width*height; ++i)
    //                {
    //                    auto d = depth_image[i];

    //                    //if (d)
    //                    //{
    //                    //    auto f = histogram[d] / (float)histogram[0xFFFF]; // 0-255 based on histogram location

    //                    //    auto c = map.get(f);
    //                    //    rgb_image[i * 3 + 0] = c.x;
    //                    //    rgb_image[i * 3 + 1] = c.y;
    //                    //    rgb_image[i * 3 + 2] = c.z;
    //                    //}
    //                    //else
    //                    //{
    //                    //    rgb_image[i * 3 + 0] = 0;
    //                    //    rgb_image[i * 3 + 1] = 0;
    //                    //    rgb_image[i * 3 + 2] = 0;
    //                    //}
    //                }
    //            }
    //        }
    //    };
    //    auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
    //    set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    //}
}

