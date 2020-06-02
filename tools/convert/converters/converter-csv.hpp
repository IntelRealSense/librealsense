// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#ifndef __RS_CONVERTER_CONVERTER_CSV_H
#define __RS_CONVERTER_CONVERTER_CSV_H


#include <fstream>

#include "../converter.hpp"


namespace rs2 {
    namespace tools {
        namespace converter {

            class converter_csv : public converter_base {
            protected:
                rs2_stream _streamType;
                std::string _filePath;

            public:
                converter_csv(const std::string& filePath, rs2_stream streamType = rs2_stream::RS2_STREAM_ANY)
                    : _filePath(filePath)
                    , _streamType(streamType)
                {
                }

                std::string name() const override
                {
                    return "CSV converter";
                }

                void convert(rs2::frame& frame) override
                {
                    auto depthframe = frame.as<rs2::depth_frame>();

                    if (!depthframe || !(_streamType == rs2_stream::RS2_STREAM_ANY || depthframe.get_profile().stream_type() == _streamType)) {
                        return;
                    }

                    if (frames_map_get_and_set(depthframe.get_profile().stream_type(), depthframe.get_frame_number())) {
                        return;
                    }

                    start_worker(
                        [this, &frame] {
                            auto depthframe = frame.as<rs2::depth_frame>();

                            std::stringstream filename;
                            filename << _filePath
                                << "_" << depthframe.get_profile().stream_name()
                                << "_" << std::setprecision(14) << std::fixed << depthframe.get_timestamp()
                                << ".csv";

                            std::stringstream metadata_file;
                            metadata_file << _filePath
                                << "_" << depthframe.get_profile().stream_name()
                                << "_metadata_" << std::setprecision(14) << std::fixed << depthframe.get_timestamp()
                                << ".txt";

                            std::string filenameS = filename.str();
                            std::string metadataS = metadata_file.str();

                            add_sub_worker(
                                [filenameS, metadataS, depthframe] {
                                    std::ofstream fs(filenameS, std::ios::trunc);

                                    if (fs) {
                                        for (int y = 0; y < depthframe.get_height(); y++) {
                                            auto delim = "";

                                            for (int x = 0; x < depthframe.get_width(); x++) {
                                                fs << delim << depthframe.get_distance(x, y);
                                                delim = ",";
                                            }

                                            fs << '\n';
                                        }

                                        fs.flush();
                                    }

                                    metadata_to_txtfile(depthframe, metadataS);
                            });

                            wait_sub_workers();
                    });
                }
            };

        }
    }
}


#endif
