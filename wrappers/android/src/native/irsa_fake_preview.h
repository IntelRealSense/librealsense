// License: Apache 2.0. See LICENSE file in root directory.
//
// purpose of the FakePreview:
// 1. used to validate framework(such as event mechanism...) in IRSA
// 2. used to development JNI relative stuffs with a general Android 
//    phone(without real Intel depth camera hardware)
// 3. UT & CT without real Intel depth camera hardware

#pragma once

#include "irsa_preview.h"
#include "cde_log.h"

namespace irsa {

class FakePreview : public IrsaPreview {

public:
    FakePreview() { }
    virtual ~FakePreview() { }

public:
    int  getDeviceCounts() { return 1; }
};

}
