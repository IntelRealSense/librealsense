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

                void convert(rs2::frameset& frameset) override
                {
                    start_worker(
                        [this, &frameset] {
                            for (size_t i = 0; i < frameset.size(); i++) {
                                auto frame = frameset[i].as<rs2::depth_frame>();

                                if (frame && (_streamType == rs2_stream::RS2_STREAM_ANY || frame.get_profile().stream_type() == _streamType)) {
                                    if (frames_map_get_and_set(frame.get_profile().stream_type(), frame.get_frame_number())) {
                                        continue;
                                    }

                                    std::stringstream filename;
                                    filename << _filePath
                                        << "_" << frame.get_profile().stream_name()
                                        << "_" << frame.get_frame_number()
                                        << ".csv";

                                    std::string filenameS = filename.str();

                                    add_sub_worker(
                                        [filenameS, frame] {
                                            std::ofstream fs(filenameS, std::ios::trunc);

                                            if (fs) {
                                                for (int y = 0; y < frame.get_height(); y++) {
                                                    auto delim = "";

                                                    for (int x = 0; x < frame.get_width(); x++) {
                                                        fs << delim << frame.get_distance(x, y);
                                                        delim = ",";
                                                    }

                                                    fs << '\n';
                                                }

                                                fs.flush();
                                            }
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
