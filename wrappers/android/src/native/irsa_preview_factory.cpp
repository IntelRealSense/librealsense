// License: Apache 2.0. See LICENSE file in root directory.

#include "irsa_preview_factory.h"

#include "irsa_fake_preview.h"
#include "irsa_rs_preview.h"

namespace irsa {

IrsaPreview *createFakePreview() {
    return new FakePreview();
}


IrsaPreview *createRealsensePreview() {
    return new RealsensePreview();
}


}
