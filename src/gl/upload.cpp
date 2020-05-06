// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"
#include "../include/librealsense2-gl/rs_processing_gl.hpp"

#include "proc/synthetic-stream.h"
#include "proc/colorizer.h"
#include "colorizer-gl.h"
#include "upload.h"
#include "option.h"
#include "context.h"

#define NOMINMAX

#include <glad/glad.h>

#include <iostream>

#include <chrono>
#include <strstream>

#include "synthetic-stream-gl.h"

namespace librealsense
{
    namespace gl
    {
        upload::upload()
            : stream_filter_processing_block("Upload Block (GLSL)")
        {
            _hist = std::vector<int>(librealsense::colorizer::MAX_DEPTH, 0);
            _fhist = std::vector<float>(librealsense::colorizer::MAX_DEPTH, 0.f);
            _hist_data = _hist.data();
            _fhist_data = _fhist.data();

            _source.add_extension<gpu_video_frame>(RS2_EXTENSION_VIDEO_FRAME_GL);
            _source.add_extension<gpu_depth_frame>(RS2_EXTENSION_DEPTH_FRAME_GL);

            initialize();
        }

        upload::~upload()
        {
            perform_gl_action([&]()
            {
                cleanup_gpu_resources();
            }, [] {});
        }

        void upload::cleanup_gpu_resources()
        {
            _enabled = false;
        }
        void upload::create_gpu_resources()
        {
            _enabled = true;
        }

        rs2::frame upload::process_frame(const rs2::frame_source& source, const rs2::frame& f)
        {
            auto res = f;

            if (_enabled)
            {
                if (f.get_profile().format() == RS2_FORMAT_YUYV)
                {
                    auto vf = f.as<rs2::video_frame>();
                    auto width = vf.get_width();
                    auto height = vf.get_height();
                    auto new_f = source.allocate_video_frame(f.get_profile(), f,
                        vf.get_bytes_per_pixel(), width, height, vf.get_stride_in_bytes(), RS2_EXTENSION_VIDEO_FRAME_GL);

                    if (new_f) perform_gl_action([&]()
                    {
                        auto gf = dynamic_cast<gpu_addon_interface*>((frame_interface*)new_f.get());

                        uint32_t output_yuv;
                        gf->get_gpu_section().output_texture(0, &output_yuv, TEXTYPE_UINT16);
                        glBindTexture(GL_TEXTURE_2D, output_yuv);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, f.get_data());
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

                        gf->get_gpu_section().set_size(width, height);

                        res = new_f;
                    }, [&]() {
                        _enabled = false;
                    });
                }

                if (f.is<rs2::depth_frame>() && (RS2_FORMAT_Z16 == f.get_profile().format()))
                {
                    auto vf = f.as<rs2::depth_frame>();
                    auto width = vf.get_width();
                    auto height = vf.get_height();
                    auto new_f = source.allocate_video_frame(f.get_profile(), f,
                        vf.get_bytes_per_pixel(), width, height, vf.get_stride_in_bytes(), RS2_EXTENSION_DEPTH_FRAME_GL);

                    if (new_f)
                    {
                        auto ptr = dynamic_cast<librealsense::depth_frame*>((librealsense::frame_interface*)new_f.get());

                        auto orig = (librealsense::frame_interface*)f.get();
                        auto depth_data = (uint16_t*)orig->get_frame_data();

                        memcpy(ptr->data.data(), depth_data, ptr->data.size());

                        ptr->set_sensor(orig->get_sensor());
                        orig->acquire();
                        frame_holder h{ orig };
                        ptr->set_original(std::move(h));

                        {
                            librealsense::colorizer::update_histogram(_hist_data,
                                depth_data, width, height);
                            librealsense::gl::colorizer::populate_floating_histogram(
                                _fhist_data, _hist_data);
                        }

                        perform_gl_action([&]()
                        {
                            //scoped_timer t("upload.depth");

                            auto gf = dynamic_cast<gpu_addon_interface*>((frame_interface*)new_f.get());

                            uint32_t depth_texture;
                            gf->get_gpu_section().output_texture(0, &depth_texture, TEXTYPE_UINT16);
                            glBindTexture(GL_TEXTURE_2D, depth_texture);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, f.get_data());
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

                            uint32_t hist_texture;
                            gf->get_gpu_section().output_texture(1, &hist_texture, TEXTYPE_FLOAT_ASSIST);
                            glBindTexture(GL_TEXTURE_2D, hist_texture);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F,
                                0xFF, 0xFF, 0, GL_RED, GL_FLOAT, _fhist_data);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

                            gf->get_gpu_section().set_size(width, height, true);

                            res = new_f;
                        }, [&]() {
                            _enabled = false;
                        });
                    }
                }
            }

            return res;
        }
    }
}

