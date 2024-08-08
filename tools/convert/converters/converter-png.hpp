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

                void convert(rs2::frame& frame) override
                {
                    rs2::video_frame videoframe = frame.as<rs2::video_frame>();
                    if (!videoframe || !(_streamType == rs2_stream::RS2_STREAM_ANY || videoframe.get_profile().stream_type() == _streamType)) {
                        return;
                    }

                    if (frames_map_get_and_set(videoframe.get_profile().stream_type(), 
                        static_cast<rs2::tools::converter::frame_number_t>(videoframe.get_timestamp()))) {
                        return;
                    }

                    start_worker(
                        [this, &frame] {
                            rs2::video_frame videoframe = frame.as<rs2::video_frame>();

                            if (videoframe.get_profile().stream_type() == rs2_stream::RS2_STREAM_DEPTH) {
                                videoframe = _colorizer.process(videoframe);
                            }

                            std::stringstream filename;
                            filename << _filePath
                                << "_" << videoframe.get_profile().stream_name()
                                << "_" << std::setprecision(14) << std::fixed << videoframe.get_timestamp()
                                << ".png";

                            std::stringstream metadata_file;
                            metadata_file << _filePath
                                << "_" << videoframe.get_profile().stream_name()
                                << "_metadata_" << std::setprecision(14) << std::fixed << videoframe.get_timestamp()
                                << ".txt";

                            std::string filenameS = filename.str();
                            std::string metadataS = metadata_file.str();

                            add_sub_worker(
                                [filenameS, metadataS, videoframe] {
                                    stbi_write_png(
                                        filenameS.c_str()
                                        , videoframe.get_width()
                                        , videoframe.get_height()
                                        , videoframe.get_bytes_per_pixel()
                                        , videoframe.get_data()
                                        , videoframe.get_stride_in_bytes()
                                    );

                                    metadata_to_txtfile(videoframe, metadataS);
                            });

                            wait_sub_workers();
                    });
                }
            };

        }
    }
}


#endif
