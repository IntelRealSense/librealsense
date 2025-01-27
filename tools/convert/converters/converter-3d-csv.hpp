// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#pragma once


#include <fstream>
#include <map>
#include <mutex>
#include <condition_variable>
#include "../converter.hpp"


namespace rs2 {
    namespace tools {
        namespace converter {

            class converter_3d_csv : public converter_base {
            protected:

                rs2_stream _streamType;
                std::string _filePath;

            public:

                converter_3d_csv(const std::string& filePath, rs2_stream streamType = rs2_stream::RS2_STREAM_ANY);

                void convert(rs2::frame& frame) override;

                std::string name() const override
                {
                    return "CSV 3D converter";
                }

                void convert_depth(rs2::depth_frame& depthframe);

            };
        } // namespace converter
    } // namespace tools
} // namespace rs2
