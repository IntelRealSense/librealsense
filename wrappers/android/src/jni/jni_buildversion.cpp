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
#define LOG_TAG "IRSA_JNI_BUILDVERSION"

#include <mutex>
#include "jni_buildversion.h"

#define CLASS_NAME          "android.os.Build$VERSION"
#define CLASS_DESCRIPTOR    "android/os/Build$VERSION"

namespace irsa {

//////////////////////////////////////////////////////////////////////////////////////
// Fields / Methods

typedef struct _fields_t {
    jfieldID SDK_INT;
    jfieldID PREVIEW_SDK_INT;
    jfieldID CODENAME;
    jfieldID BASE_OS;
    jfieldID INCREMENTAL;
    jfieldID RELEASE;
    jfieldID SDK;
    jfieldID SECURITY_PATCH;
} fields_t;

static fields_t gFields;

static JNIMemberInfo gMembers[] = {
    // Fields
    MAKE_JNI_STATIC_FIELD(SDK_INT,            "I"),
    MAKE_JNI_STATIC_FIELD_OP(PREVIEW_SDK_INT, "I"),
    MAKE_JNI_STATIC_FIELD(CODENAME,           "Ljava/lang/String;"),
    MAKE_JNI_STATIC_FIELD_OP(BASE_OS,         "Ljava/lang/String;"),
    MAKE_JNI_STATIC_FIELD(INCREMENTAL,        "Ljava/lang/String;"),
    MAKE_JNI_STATIC_FIELD(RELEASE,            "Ljava/lang/String;"),
    MAKE_JNI_STATIC_FIELD(SDK,                "Ljava/lang/String;"),
    MAKE_JNI_STATIC_FIELD_OP(SECURITY_PATCH,  "Ljava/lang/String;"),
};

typedef struct _int32_field_t {
    jfieldID* field;
    int32_t* value;
    bool optional;
} int32_field_t;

#define MAKE_STATIC_FIELD(name) { &gFields.name, &JNIBuildVersion::name, false }
#define MAKE_STATIC_FIELD_OP(name) { &gFields.name, &JNIBuildVersion::name, true }

static int32_field_t gInt32Fields[] = {
    MAKE_STATIC_FIELD(SDK_INT),
    MAKE_STATIC_FIELD_OP(PREVIEW_SDK_INT),
};

#define INIT_STATIC_FIELD_INT32(name) int32_t(JNIBuildVersion::name) = 0
INIT_STATIC_FIELD_INT32(SDK_INT);
INIT_STATIC_FIELD_INT32(PREVIEW_SDK_INT);
INIT_STATIC_FIELD_INT32(SDK_INT_REVISE);


typedef struct _string_field_t {
    jfieldID* field;
    std::string* value;
    bool optional;
} string_field_t;

static string_field_t gStringFields[] = {
    MAKE_STATIC_FIELD(CODENAME),
    MAKE_STATIC_FIELD_OP(BASE_OS),
    MAKE_STATIC_FIELD(INCREMENTAL),
    MAKE_STATIC_FIELD(RELEASE),
    MAKE_STATIC_FIELD(SDK),
    MAKE_STATIC_FIELD_OP(SECURITY_PATCH),
};

#define INIT_STATIC_FIELD_ASTRING(name) std::string JNIBuildVersion::name

INIT_STATIC_FIELD_ASTRING(CODENAME);
INIT_STATIC_FIELD_ASTRING(BASE_OS);
INIT_STATIC_FIELD_ASTRING(INCREMENTAL);
INIT_STATIC_FIELD_ASTRING(RELEASE);
INIT_STATIC_FIELD_ASTRING(SDK);
INIT_STATIC_FIELD_ASTRING(SECURITY_PATCH);

//////////////////////////////////////////////////////////////////////////////////////
// static initializer: get method/field IDs once.

static std::mutex gInitMutex;
static bool gInited = false;

static bool loadStaticValues() {
    size_t i = 0;
    size_t count = sizeof(gInt32Fields) / sizeof(int32_field_t);
    for (i = 0; i < count; i++) {
        int32_field_t& f = gInt32Fields[i];
        CHECK(f.field != NULL);
        CHECK(f.value != NULL);
        if (!JNIClassBase::getStaticFieldValue(CLASS_DESCRIPTOR, *f.field, *f.value)) {
            if (!f.optional) {
                return false;
            }
        }
    }

    count = sizeof(gStringFields) / sizeof(string_field_t);
    for (i = 0; i < count; i++) {
        string_field_t& f = gStringFields[i];

        CHECK(f.field != NULL);
        CHECK(f.value != NULL);

        if (!JNIClassBase::getStaticFieldValue(CLASS_DESCRIPTOR, *f.field, *f.value)) {
            if (!f.optional) {
                return false;
            }
        }
    }

    return true;
}

static void static_init() {
    std::lock_guard<std::mutex> lock(gInitMutex);

    if (!gInited) {
        CHECK(JNIClassBase::loadJNIMembers(CLASS_NAME, CLASS_DESCRIPTOR, gMembers, sizeof(gMembers) / sizeof(JNIMemberInfo)));
        CHECK(loadStaticValues());
        gInited = true;
    }
}

bool JNIBuildVersion::isAvailable() {
    return JNIClassBase::isJavaClassAvailable(CLASS_DESCRIPTOR);
}

void JNIBuildVersion::init() {
    static_init();
    std::size_t pos;
    pos = CODENAME.find_first_of("N");
    //SDK_INT_REVISE = (SDK_INT == 23 && CODENAME.startsWith("N")) ? 24 :SDK_INT;
    SDK_INT_REVISE = (SDK_INT == 23 && pos == 1) ? 24 :SDK_INT;
}

}  // namespace android
