// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#ifndef __RS_CONVERTER_CONVERTER_PNG_H
#define __RS_CONVERTER_CONVERTER_PNG_H


// 3rd party header for writing png files
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "../converter.hpp"


namespace rs2 {
    namespace tools {
        namespace converter {

            class converter_png : public converter_base {
                rs2_stream _streamType;
                std::string _filePath;
                rs2::colorizer _colorizer;

            public:
                converter_png(const std::string& filePath, rs2_stream streamType = rs2_stream::RS2_STREAM_ANY)
                    : _filePath(filePath)
                    , _streamType(streamType)
                {
                }

                std::string name() const override
                {
                    return "PNG converter";
                }

                void convert(rs2::frameset& frameset) override
                {
                    start_worker(
                        [this, &frameset] {
                            for (size_t i = 0; i < frameset.size(); i++) {
                                rs2::video_frame frame = frameset[i].as<rs2::video_frame>();

                                if (frame && (_streamType == rs2_stream::RS2_STREAM_ANY || frame.get_profile().stream_type() == _streamType)) {
                                    if (frames_map_get_and_set(frame.get_profile().stream_type(), frame.get_frame_number())) {
                                        continue;
                                    }

                                    if (frame.get_profile().stream_type() == rs2_stream::RS2_STREAM_DEPTH) {
                                        frame = _colorizer.process(frame);
                                    }

                                    std::stringstream filename;
                                    filename << _filePath
                                        << "_" << frame.get_profile().stream_name()
                                        << "_" << frame.get_frame_number()
                                        << ".png";

                                    std::string filenameS = filename.str();

                                    add_sub_worker(
                                        [filenameS, frame] {
                                            stbi_write_png(
                                                filenameS.c_str()
                                                , frame.get_width()
                                                , frame.get_height()
                                                , frame.get_bytes_per_pixel()
                                                , frame.get_data()
                                                , frame.get_stride_in_bytes()
                                            );
                                        });
                                }
                            }

                            wait_sub_workers();
                        });
                }
            };

        }
    }
}


#endif
