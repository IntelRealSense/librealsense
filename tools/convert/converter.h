// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#ifndef __RS_CONVERTER_CONVERTER_H
#define __RS_CONVERTER_CONVERTER_H


namespace rs2 {
    namespace tools {
        namespace converter {

            class Converter {
            public:
                virtual void convert(rs2::frameset& frameset) = 0;
            };

        }
    }
}


#endif
