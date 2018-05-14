// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#ifndef __RS_CONVERTER_CONVERTER_PNG_H
#define __RS_CONVERTER_CONVERTER_PNG_H


#include <string>

#include "librealsense2/rs.hpp"

// 3rd party header for writing png files
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "../converter.h"


namespace rs2 {
    namespace tools {
        namespace converter {

            class ConverterPng : public Converter  {
                std::string m_filePath;
                rs2::colorizer m_colorizer;

            public:
                ConverterPng(const std::string& filePath)
                    : m_filePath(filePath)
                {
                }

                void convert(rs2::frameset& frameset) override
                {
                    for (auto frame : frameset) {
                        if (auto f = frame.as<rs2::video_frame>()) {
                            auto stream = frame.get_profile().stream_type();

                            if (f.is<rs2::depth_frame>()) {
                                f = m_colorizer(frame);
                            }

                            std::stringstream filename;
                            filename << m_filePath << "_" << f.get_profile().stream_name() << "_" << f.get_frame_number() << ".png";
                            stbi_write_png(
                                filename.str().c_str()
                                , f.get_width()
                                , f.get_height()
                                , f.get_bytes_per_pixel()
                                , f.get_data()
                                , f.get_stride_in_bytes()
                            );
                        }
                    }
                }
            };

        }
    }
}


#endif
