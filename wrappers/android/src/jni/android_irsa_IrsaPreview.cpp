// License: Apache 2.0. See LICENSE file in root directory.

#define LOG_TAG "IRSA_JNI_PREVIEW"

#include <string>
#include <map>
#include <sstream>

#include <jni.h>
#include <android/native_window_jni.h>

#include "cde_log.h"
#include "util.h"
#include "jni_buildversion.h"
#include "jni_utils.h"
#include "irsa_mgr.h"
#include "irsa_event.h"
#include "irsa_fake_preview.h"
#include "irsa_rs_preview.h"
#include "irsa_preview_factory.h"
#include "android_irsa_IrsaMgr.h"
#include "android_irsa_IrsaPreview.h"



namespace irsa {

IrsaPreview *g_preview;

static const char *classPathName = "android/irsa/IrsaMgr";

static void IrsaPreview_native_setup() {
    LOGD("native_setup");

    IrsaPreviewFactory factory;
    factory.registerClass("fake", createFakePreview);
    factory.registerClass("realsense", createRealsensePreview);

    g_preview = factory.create("fake");
    //g_preview = factory.create("realsense");
    CHECK(g_preview != nullptr);
}


void IrsaPreview_native_finalize() {
    LOGV("native_finalize");
    delete g_preview;
    g_preview = nullptr;
}

static int android_irsa_IrsaPreview_getDeviceCounts(JNIEnv *env, jobject thiz) {
    CHECK(g_preview != nullptr);
    int deviceCounts = g_preview->getDeviceCounts();
    LOGD("enter getDeviceCounts, deviceCounts %d", deviceCounts);

    return deviceCounts;
}


static void android_irsa_IrsaPreview_setRenderID(JNIEnv *env, jobject thiz, jint renderID) {
    LOGD("setRenderID");
    CHECK(g_preview != nullptr);
    g_preview->setRenderID(renderID);

#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);
    std::ostringstream info;
    info << "setRenderID " <<  renderID;
    LOGD("notify event");
    mgr->notify(IRSA_INFO, IRSA_INFO_SETRENDERID, (size_t)info.str().c_str());
}


static void android_irsa_IrsaPreview_setPreviewDisplay(JNIEnv *env, jobject thiz, jobject mapsurface) {
    LOGD("setPreviewDisplay");
    if (mapsurface == nullptr) {
        LOGD("invalid mapsurface\n");
        return;
    }

    std::map<int, ANativeWindow *> surfaceMap;
    if (mapsurface) {
        jclass mapClass = env->FindClass("java/util/Map");
        jmethodID entrySet = env->GetMethodID(mapClass, "entrySet", "()Ljava/util/Set;");
        jobject set = env->CallObjectMethod(mapsurface, entrySet);
        jclass setClass = env->FindClass("java/util/Set");
        jmethodID iterator = env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");
        jobject iter = env->CallObjectMethod(set, iterator);
        jclass iteratorClass = env->FindClass("java/util/Iterator");
        jmethodID hasNext = env->GetMethodID(iteratorClass, "hasNext", "()Z");
        jmethodID next = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");
        jclass entryClass = env->FindClass("java/util/Map$Entry");
        jmethodID getKey = env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
        jmethodID getValue = env->GetMethodID(entryClass, "getValue", "()Ljava/lang/Object;");
        while (env->CallBooleanMethod(iter, hasNext)) {
            jobject entry = env->CallObjectMethod(iter, next);
            jobject keyobject = (jobject) env->CallObjectMethod(entry, getKey);
            jobject value = (jobject) env->CallObjectMethod(entry, getValue);

            jclass cls = env->FindClass("java/lang/Integer");
            jmethodID getVal = env->GetMethodID(cls, "intValue", "()I");
            int key = env->CallIntMethod(keyobject, getVal);

            ANativeWindow *_surface = ANativeWindow_fromSurface(env, value);
            surfaceMap.insert(std::map<int, ANativeWindow *>::value_type(key, _surface));

            env->DeleteLocalRef(cls);
            env->DeleteLocalRef(entry);
            env->DeleteLocalRef(keyobject);
            env->DeleteLocalRef(value);
        }

        env->DeleteLocalRef(entryClass);
        env->DeleteLocalRef(iteratorClass);
        env->DeleteLocalRef(iter);
        env->DeleteLocalRef(setClass);
        env->DeleteLocalRef(set);
        env->DeleteLocalRef(mapClass);
    }
    CHECK(g_preview != nullptr);
    g_preview->setPreviewDisplay(surfaceMap);


#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    if (mgr == nullptr ) {
        return;
    }
    std::string info("setSurfaceMap");
    LOGD("notify event");
    mgr->notify(IRSA_INFO, IRSA_INFO_SETPREVIEWDISPLAY, (size_t)info.c_str());
}


static void android_irsa_IrsaPreview_setStreamFormat(JNIEnv *env, jobject thiz, jint stream,
                                                           jint width, jint height, jint fps,
                                                           jint format) {
    LOGD("setStreamFormat %d, %d %d %d, %d", stream, width, height, fps, format);
    CHECK(g_preview != nullptr);
    g_preview->setStreamFormat(stream, width, height, fps, format);
}


static void android_irsa_IrsaPreview_open(JNIEnv *env, jobject thiz) {
    LOGD("enter JNI open");
    CHECK(g_preview != nullptr);
    g_preview->open();

#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);
    std::string info("open");
    LOGD("notify event");
    mgr->notify(IRSA_INFO, IRSA_INFO_OPEN, (size_t)info.c_str());
}


static void android_irsa_IrsaPreview_close(JNIEnv *env, jobject thiz) {
    LOGD("enter JNI close");
    CHECK(g_preview != nullptr);
    g_preview->close();

#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);
    std::string info("close");
    LOGD("notify event");
    mgr->notify(IRSA_INFO, IRSA_INFO_CLOSE, (size_t)info.c_str());
}


static void android_irsa_IrsaPreview_startPreview(JNIEnv *env, jobject thiz) {
    LOGD("enter JNI startPreview");
    CHECK(g_preview != nullptr);
    g_preview->startPreview();

#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);
    std::string info("startPreview");
    LOGD("notify event");
    mgr->notify(IRSA_INFO, IRSA_INFO_PREVIEW_START, (size_t)info.c_str());
}


static void android_irsa_IrsaPreview_stopPreview(JNIEnv *env, jobject thiz) {
    LOGD("enter JNI stopPreview");
    CHECK(g_preview != nullptr);
    g_preview->stopPreview();

#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);
    std::string info("stopPreview");
    LOGD("notify event");
    mgr->notify(IRSA_INFO, IRSA_INFO_PREVIEW_STOP, (size_t)info.c_str());
}


static JNINativeMethod sMethods[] = {
    {"native_open",             "()V",                     (void *)android_irsa_IrsaPreview_open},
    {"native_close",            "()V",                     (void *)android_irsa_IrsaPreview_close},
    {"startPreview",            "()V",                     (void *)android_irsa_IrsaPreview_startPreview},
    {"stopPreview",             "()V",                     (void *)android_irsa_IrsaPreview_stopPreview},
    {"getDeviceCounts",         "()I",                     (void *)android_irsa_IrsaPreview_getDeviceCounts},
    {"setRenderID",             "(I)V",                    (void *)android_irsa_IrsaPreview_setRenderID},
    {"setStreamFormat",         "(IIIII)V",                (void *)android_irsa_IrsaPreview_setStreamFormat},
    {"setPreviewDisplay",       "(Ljava/util/Map;)V",      (void *)android_irsa_IrsaPreview_setPreviewDisplay}
};


int register_android_irsa_IrsaPreview(JavaVM *vm) {
    LOGD("register JNI functions");

    IrsaPreview_native_setup();

    JNIEnv* env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv failed");
        return JNI_FALSE;
    }

    if (!registerNativeMethods(env, classPathName,
                sMethods, sizeof(sMethods) / sizeof(sMethods[0]))) {
        return JNI_FALSE;
    }

    return JNI_VERSION_1_6;
}


void unregister_android_irsa_IrsaPreview(JavaVM *vm)
{
    LOGD("unregister");

    JNIEnv* env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("GetEnv failed");
        return;
    }

    if(env) {
        jclass clazz = env->FindClass(classPathName);
        if (clazz != nullptr) {
            env->UnregisterNatives(clazz);
            env->DeleteLocalRef(clazz);
        }
    }
}

}

