// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#ifndef __RS_CONVERTER_CONVERTER_PLY_H
#define __RS_CONVERTER_CONVERTER_PLY_H


#include "../converter.hpp"


namespace rs2 {
    namespace tools {
        namespace converter {

            class converter_ply : public converter_base {
            protected:
                std::string _filePath;

            public:
                converter_ply(const std::string& filePath)
                    : _filePath(filePath)
                {
                }

                std::string name() const override
                {
                    return "PLY converter";
                }

                void convert(rs2::frameset& frameset) override
                {
                    rs2::pointcloud pc;
                    start_worker(
                        [this, &frameset, pc]() mutable {
                            auto frameDepth = frameset.get_depth_frame();
                            auto frameColor = frameset.get_color_frame();

                            if (frameDepth && frameColor) {
                                if (frames_map_get_and_set(rs2_stream::RS2_STREAM_ANY, frameDepth.get_frame_number())) {
                                    return;
                                }

                                pc.map_to(frameColor);

                                auto points = pc.calculate(frameDepth);

                                std::stringstream filename;
                                filename << _filePath
                                    << "_" << frameDepth.get_frame_number()
                                    << ".ply";

                                points.export_to_ply(filename.str(), frameColor);
                            }
                        });
                }
            };

        }
    }
}


#endif
