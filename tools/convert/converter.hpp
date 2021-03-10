// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#ifndef __RS_CONVERTER_CONVERTER_H
#define __RS_CONVERTER_CONVERTER_H

#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <string>
#include <sstream>

#include "librealsense2/rs.hpp"


namespace rs2 {
    namespace tools {
        namespace converter {

            void metadata_to_txtfile(const rs2::frame& frm, const std::string& filename)
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

            typedef unsigned long long frame_number_t;

            class converter_base {
            protected:
                std::thread _worker;
                std::vector<std::thread> _subWorkers;
                std::unordered_map<int, std::unordered_set<frame_number_t>> _framesMap;

            protected:
                bool frames_map_get_and_set(rs2_stream streamType, frame_number_t frameNumber)
                {
                    if (_framesMap.find(streamType) == _framesMap.end()) {
                        _framesMap.emplace(streamType, std::unordered_set<frame_number_t>());
                    }

                    auto & set = _framesMap[streamType];
                    bool result = (set.find(frameNumber) != set.end());

                    if (!result) {
                        set.emplace(frameNumber);
                    }

                    return result;
                }

                template <typename F> void start_worker(const F& f)
                {
                    _worker = std::thread(f);
                }

                template <typename F> void add_sub_worker(const F& f)
                {
                    _subWorkers.emplace_back(f);
                }

                void wait_sub_workers()
                {
                    for_each(_subWorkers.begin(), _subWorkers.end(),
                        [] (std::thread& t) {
                            t.join();
                        });

                    _subWorkers.clear();
                }

            public:
                virtual void convert(rs2::frame& frame) = 0;
                virtual std::string name() const = 0;

                virtual std::string get_statistics()
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

                void wait()
                {
                    _worker.join();
                }
            };

        }
    }
}


#endif
