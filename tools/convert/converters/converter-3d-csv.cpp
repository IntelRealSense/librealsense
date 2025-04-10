// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.


#include "converter-3d-csv.hpp"
#include <iomanip>
#include <algorithm>
#include <iostream>

using namespace rs2::tools::converter;

converter_3d_csv::converter_3d_csv(const std::string& filePath, rs2_stream streamType)
    : _filePath(filePath)
    , _streamType(streamType)
{
}

void converter_3d_csv::convert_depth(rs2::depth_frame& depthframe)
{
    if (frames_map_get_and_set(depthframe.get_profile().stream_type(), depthframe.get_profile().stream_index(), depthframe.get_frame_number())) {
        return;
    }

    start_worker(
        [this, depthframe] {

            std::stringstream filename;
            std::stringstream filename_3d_dist;
            filename << _filePath << "3d_dist"
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
                        // Write the title row
                        fs << "i,j,x (meters),y (meters),depth (meters)\n";

                        // Get the intrinsic parameters of the depth frame
                        rs2_intrinsics intrinsics = depthframe.get_profile().as<rs2::video_stream_profile>().get_intrinsics();

                        for (int y = 0; y < depthframe.get_height(); y++) {

                            for (int x = 0; x < depthframe.get_width(); x++) {
                                float distance = depthframe.get_distance(x, y);

                                // Write to the 3D distance file if the distance is non-zero
                                if (distance != 0) {
                                    float pixel[2] = { static_cast<float>(x), static_cast<float>(y) };
                                    float point[3];
                                    rs2_deproject_pixel_to_point(point, &intrinsics, pixel, distance);
                                    fs << x << "," << y << "," << point[0] << "," << point[1] << "," << point[2] << '\n';
                                }
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

void converter_3d_csv::convert(rs2::frame& frame)
{
    if (!(_streamType == rs2_stream::RS2_STREAM_ANY || frame.get_profile().stream_type() == _streamType))
        return;

    auto depthframe = frame.as<rs2::depth_frame>();
    if (depthframe)
    {
        convert_depth(depthframe);
    }
}
