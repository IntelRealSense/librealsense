// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#ifndef __RS_CONVERTER_CONVERTER_RAW_H
#define __RS_CONVERTER_CONVERTER_RAW_H


#include <fstream>

#include "../converter.hpp"


namespace rs2 {
    namespace tools {
        namespace converter {

            class converter_raw : public converter_base {
                rs2_stream _streamType;
                std::string _filePath;

            public:
                converter_raw(const std::string& filePath, rs2_stream streamType = rs2_stream::RS2_STREAM_ANY)
                    : _filePath(filePath)
                    , _streamType(streamType)
                {
                }

                std::string name() const override
                {
                    return "RAW converter";
                }

                void convert(rs2::frame& frame) override
                {
                    rs2::video_frame videoframe = frame.as<rs2::video_frame>();

                    if (!videoframe || !(_streamType == rs2_stream::RS2_STREAM_ANY || videoframe.get_profile().stream_type() == _streamType)) {
                        return;
                    }

                    if (frames_map_get_and_set(videoframe.get_profile().stream_type(), videoframe.get_frame_number())) {
                        return;
                    }

                    start_worker(
                        [this, &frame] {
                            rs2::video_frame videoframe = frame.as<rs2::video_frame>();

                            std::stringstream filename;
                            filename << _filePath
                                << "_" << videoframe.get_profile().stream_name()
                                << "_" << std::setprecision(14) << std::fixed << videoframe.get_timestamp()
                                << ".raw";

                            std::stringstream metadata_file;
                            metadata_file << _filePath
                                << "_" << videoframe.get_profile().stream_name()
                                << "_metadata_" << std::setprecision(14) << std::fixed << videoframe.get_timestamp()
                                << ".txt";

                            std::string filenameS = filename.str();
                            std::string metadataS = metadata_file.str();

                            add_sub_worker(
                                [filenameS, metadataS, videoframe] {
                                    std::ofstream fs(filenameS, std::ios::binary | std::ios::trunc);

                                    if (fs) {
                                        fs.write(
                                            static_cast<const char *>(videoframe.get_data())
                                            , videoframe.get_stride_in_bytes() * videoframe.get_height());

                                        fs.flush();
                                    }

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
