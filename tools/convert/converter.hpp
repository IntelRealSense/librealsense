// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#ifndef __RS_CONVERTER_CONVERTER_H
#define __RS_CONVERTER_CONVERTER_H

#include <unordered_map>
#include <unordered_set>

#include "librealsense2/rs.hpp"


namespace rs2 {
    namespace tools {
        namespace converter {

            typedef unsigned long long frame_number_t;

            class converter_base {
            protected:
                std::thread _worker;
                std::unordered_map<rs2_stream, std::unordered_set<frame_number_t>> _framesMap;

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

                template <typename F> void start_worker(F& f)
                {
                    _worker = std::thread(f);
                }

            public:
                virtual void convert(rs2::frameset& frameset) = 0;

                void wait()
                {
                    _worker.join();
                }
            };

        }
    }
}


#endif
