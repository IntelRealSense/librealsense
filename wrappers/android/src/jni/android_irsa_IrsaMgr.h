// License: Apache 2.0. See LICENSE file in root directory.

#pragma once

#include "jni.h"

namespace irsa {

extern int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* gMethods, int numMethods);
extern int register_android_irsa_IrsaMgr(JavaVM *vm);
extern void unregister_android_irsa_IrsaMgr(JavaVM *vm);

}
