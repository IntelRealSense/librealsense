// License: Apache 2.0. See LICENSE file in root directory.

#pragma once

#include "jni.h"

namespace irsa {

extern int register_android_irsa_IrsaPreview(JavaVM *vm);
extern void unregister_android_irsa_IrsaPreview(JavaVM *vm);
extern void IrsaPreview_native_finalize();

}
