// reference to some AOSP source code
//
// replace all AOSP's c++ classes with c++ 11 STL
//
// this file used to illustrate how to access AOSP internal class in native
// layer. you could add more similar c++ source code in src/jni directory
//
// TODO: try to enable librealsense running well on non-rooted Android devices

/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "jni_utils.h"
#include <string>

namespace irsa {

/**
 * Native wrapper class for Java Build.VERSION class.
 * http://developer.android.com/reference/android/os/Build.VERSION.html
 *
 * Note: Java Build.VERSION class is available beginning from API level 1.
 */
struct JNIBuildVersion : public JNIClassBase {
public:
    static bool isAvailable();
    static void init();

    static int32_t SDK_INT;
    static int32_t PREVIEW_SDK_INT;
    static int32_t SDK_INT_REVISE;

    static std::string CODENAME;
    static std::string BASE_OS;
    static std::string INCREMENTAL;
    static std::string RELEASE;
    static std::string SDK;
    static std::string SECURITY_PATCH;

protected:
    virtual ~JNIBuildVersion() {}

private:
    JNIBuildVersion();
    JNIBuildVersion(jobject thiz);

    JNIBuildVersion(const JNIBuildVersion &); 
    JNIBuildVersion &operator=(const JNIBuildVersion &);
};

}  // namespace android

