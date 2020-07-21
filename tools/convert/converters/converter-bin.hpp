// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#ifndef __RS_CONVERTER_CONVERTER_BIN_H
#define __RS_CONVERTER_CONVERTER_BIN_H


#include <fstream>
#include <cmath>

#include "../converter.hpp"


namespace rs2 {
    namespace tools {
        namespace converter {

            class converter_bin : public converter_base {
                rs2_stream _streamType;
                std::string _filePath;

            protected:
                static void* to_ieee754_32(float f, uint8_t* buffer)
                {
                    uint32_t ieee754 = 0;

                    if (f != 0.) {
                        int sign = 0;

                        if (f < 0) {
                            sign = 1;
                            f = -f;
                        }

                        int shift = 0;

                        while (f >= 2.0) {
                            f /= 2.0;
                            shift++;
                        }

                        while (f < 1.0) {
                            f *= 2.0;
                            shift--;
                        }

                        f = f - 1.f;
                        uint32_t mantissa = uint32_t(f * ((1U << 23) + 0.5f));
                        uint32_t exponent = shift + ((1 << 7) - 1);

                        ieee754 = (sign << 31) | (exponent << 23) | mantissa;
                    }

                    buffer[0] = ieee754 & 0xff;
                    buffer[1] = (ieee754 >> 8) & 0xff;
                    buffer[2] = (ieee754 >> 16) & 0xff;
                    buffer[3] = (ieee754 >> 24) & 0xff;

                    return buffer;
                }

            public:
                converter_bin(const std::string& filePath, rs2_stream streamType = rs2_stream::RS2_STREAM_ANY)
                    : _filePath(filePath)
                    , _streamType(streamType)
                {
                }

                std::string name() const override
                {
                    return "BIN converter";
                }

                void convert(rs2::frame& frame) override
                {
                    rs2::depth_frame depthframe = frame.as<rs2::depth_frame>();

                    if (!depthframe || !(_streamType == rs2_stream::RS2_STREAM_ANY || depthframe.get_profile().stream_type() == _streamType)) {
                        return;
                    }

                    if (frames_map_get_and_set(depthframe.get_profile().stream_type(), depthframe.get_frame_number())) {
                        return;
                    }

                    start_worker(
                        [this, &frame] {
                            rs2::depth_frame depthframe = frame.as<rs2::depth_frame>();

                            std::stringstream filename;
                            filename << _filePath
                                << "_" << depthframe.get_profile().stream_name()
                                << "_" << std::setprecision(14) << std::fixed << depthframe.get_timestamp()
                                << ".bin";

                            std::stringstream metadata_file;
                            metadata_file << _filePath
                                << "_" << depthframe.get_profile().stream_name()
                                << "_metadata_" << std::setprecision(14) << std::fixed << depthframe.get_timestamp()
                                << ".txt";

                            std::string filenameS = filename.str();
                            std::string metadataS = metadata_file.str();

                            add_sub_worker(
                                [filenameS, metadataS, depthframe] {
                                    std::ofstream fs(filenameS, std::ios::binary | std::ios::trunc);

                                    if (fs) {
                                        uint8_t buffer[4];

                                        for (int y = 0; y < depthframe.get_height(); y++) {
                                            for (int x = 0; x < depthframe.get_width(); x++) {
                                                fs.write(
                                                    static_cast<const char *>(to_ieee754_32(depthframe.get_distance(x, y), buffer))
                                                    , sizeof buffer);
                                            }
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
