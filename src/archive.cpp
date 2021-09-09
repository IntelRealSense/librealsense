// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "archive.h"
#include "metadata-parser.h"
#include "frame-archive.h"

namespace librealsense
{
   
    std::shared_ptr<archive_interface> make_archive(rs2_extension type,
        std::atomic<uint32_t>* in_max_frame_queue_size,
        std::shared_ptr<platform::time_service> ts,
        std::shared_ptr<metadata_parser_map> parsers)
    {
        switch (type)
        {
        case RS2_EXTENSION_VIDEO_FRAME:
            return std::make_shared<frame_archive<video_frame>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_COMPOSITE_FRAME:
            return std::make_shared<frame_archive<composite_frame>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_MOTION_FRAME:
            return std::make_shared<frame_archive<motion_frame>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_POINTS:
            return std::make_shared<frame_archive<points>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_DEPTH_FRAME:
            return std::make_shared<frame_archive<depth_frame>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_POSE_FRAME:
            return std::make_shared<frame_archive<pose_frame>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_DISPARITY_FRAME:
            return std::make_shared<frame_archive<disparity_frame>>(in_max_frame_queue_size, ts, parsers);

        default:
            throw std::runtime_error("Requested frame type is not supported!");
        }
    }
}
