// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#ifndef __RS_CONVERTER_CONVERTER_CSV_H
#define __RS_CONVERTER_CONVERTER_CSV_H


#include "../converter.hpp"


namespace rs2 {
    namespace tools {
        namespace converter {

            class converter_csv : public converter_base {
            public:
                std::string name() const override
                {
                    return "CSV converter";
                }

                void convert(rs2::frameset& frameset) override
                {
                }
            };

        }
    }
}


#endif
