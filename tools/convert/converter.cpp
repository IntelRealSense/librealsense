// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include <fstream>
#include "converter.hpp"

using namespace rs2::tools::converter;

void rs2::tools::converter::metadata_to_txtfile(const rs2::frame& frm, const std::string& filename)
{
    std::ofstream file;

    file.open(filename);

    file << "Stream: " << rs2_stream_to_string(frm.get_profile().stream_type()) << "\n";

    // Record all the available metadata attributes
    for (size_t i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
    {
        rs2_frame_metadata_value metadata_val = (rs2_frame_metadata_value)i;
        if (frm.supports_frame_metadata(metadata_val))
        {
            file << rs2_frame_metadata_to_string(metadata_val) << ": "
                << frm.get_frame_metadata(metadata_val) << "\n";
        }
    }

    file.close();
}

bool converter_base::frames_map_get_and_set(rs2_stream streamType, frame_number_t frameNumber)
{
    if (_framesMap.find(streamType) == _framesMap.end()) {
        _framesMap.emplace(streamType, std::unordered_set<frame_number_t>());
    }

    auto& set = _framesMap[streamType];
    bool result = (set.find(frameNumber) != set.end());

    if (!result) {
        set.emplace(frameNumber);
    }

    return result;
}

void converter_base::wait_sub_workers()
{
    for_each(_subWorkers.begin(), _subWorkers.end(),
        [](std::thread& t) {
            t.join();
        });

    _subWorkers.clear();
}

void converter_base::wait()
{
    _worker.join();
}

std::string converter_base::get_statistics()
{
    std::stringstream result;
    result << name() << '\n';

    for (auto& i : _framesMap) {
        result << '\t'
            << i.second.size() << ' '
            << (static_cast<rs2_stream>(i.first) != rs2_stream::RS2_STREAM_ANY ? rs2_stream_to_string(static_cast<rs2_stream>(i.first)) : "")
            << " frame(s) processed"
            << '\n';
    }

    return (result.str());
}

