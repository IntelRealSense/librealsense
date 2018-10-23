// License: Apache 2.0. See LICENSE file in root directory.

#define LOG_TAG "IRSA_JNI_MGR"

#include <string>
#include <thread>
#include <array>
#include <map>
#include <sstream>
#include <mutex>

#include <string.h>
#include <jni.h>
#include <android/native_window_jni.h>

#include "cde_log.h"
#include "util.h"
#include "jni_buildversion.h"
#include "jni_utils.h"
#include "irsa_mgr.h"
#include "irsa_event.h"
#include "android_irsa_IrsaMgr.h"
#include "android_irsa_IrsaPreview.h"


JavaVM *g_jvm;

namespace irsa {

struct fields_t
{
    jfieldID    context;
    jmethodID   postEventFromNativeID;
};

static fields_t sFields;
static std::mutex _mutex;
static int sDeviceCounts = 0;
static const char *classPathName = "android/irsa/IrsaMgr";

static void IrsaMgr_native_finalize();

#define IRSA_VERSION "1.0.0"

class IrsaMgrJNIListener: public IrsaMgrListener {
public:
    IrsaMgrJNIListener(JNIEnv* env, jobject thiz, jobject weak_thiz);
    ~IrsaMgrJNIListener();
    virtual void notify(int msg, int arg1, size_t arg2);
private:
    IrsaMgrJNIListener();
    jclass      mClass;  
    jobject     mObject;
};


IrsaMgrJNIListener::IrsaMgrJNIListener(JNIEnv* env, jobject thiz, jobject weak_thiz) {
    LOGV("constructor");
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == nullptr) {
        LOGD("Can't find IrsaMgr");
        return;
    }
    mClass  = (jclass)env->NewGlobalRef(clazz);
    mObject = env->NewGlobalRef(weak_thiz);
}

IrsaMgrJNIListener::~IrsaMgrJNIListener() {
    LOGV("destructor");
    AutoJNIEnv env;
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
}


void IrsaMgrJNIListener::notify(int msg, int arg1, size_t arg2) {
    AutoJNIEnv env;

    jobject obj = nullptr;
    bool releaseLocalRef = false;

    if ((msg == IRSA_INFO) || (msg == IRSA_ERROR)) {
        const char *strInfo = (const char*)arg2;
        if (strInfo != nullptr) {
            CHECK(env.get() != nullptr);
            obj  = (jobject)env->NewStringUTF(strInfo);
            releaseLocalRef = true;
        }
    }

    if (sFields.postEventFromNativeID != nullptr && sFields.context != nullptr) {
        CHECK(env.get() != nullptr);
        env->CallStaticVoidMethod(mClass, sFields.postEventFromNativeID, mObject, msg, arg1, arg2, obj);
    } 

    if (releaseLocalRef) {
        if( obj != nullptr ) {
            env->DeleteLocalRef(obj);
        }
    }
}


static void setIrsaMgr(JNIEnv* env, jobject thiz, const IrsaMgr *mgr) {
    std::lock_guard<std::mutex> lock(_mutex);
    void *tmpP = static_cast<void*>(const_cast<IrsaMgr*>(mgr));
    //need support arm32/arm64/x86/x86_64 toolchain, so:
    size_t  tmpI = reinterpret_cast<size_t>(tmpP);
    env->SetIntField(thiz, sFields.context, tmpI);
}


static void android_irsa_IrsaMgr_native_init(JNIEnv *env) {
    jclass clazz;

    LOGD("native_init");
    clazz = env->FindClass(classPathName);
    CHECK(clazz != nullptr);

    sFields.context = env->GetFieldID(clazz, "mNativeContext", "I");
    sFields.postEventFromNativeID = env->GetStaticMethodID(clazz, "postEventFromNative",
                       "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    CHECK(sFields.postEventFromNativeID != nullptr);
}


static void android_irsa_IrsaMgr_native_release(JNIEnv *env, jobject thiz) {
    LOGV("native_release");
    static bool bReleased = false;
    if (!bReleased) {
        IrsaPreview_native_finalize();
        IrsaMgr_native_finalize();
        bReleased = true;
    }
}


static void IrsaMgr_native_finalize() {
    LOGV("native_finalize");
#ifndef __SMART_POINTER__
    delete IrsaMgr::getInstance();
#else
    //IrsaMgr::getInstance().reset();
    //jniListener need to be destroyed
    delete IrsaMgr::getInstance().get();
#endif
}


static void android_irsa_IrsaMgr_native_setup(JNIEnv *env, jobject thiz, jobject weak_this) {
    LOGV("native_setup");
#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);

#ifndef __SMART_POINTER__
    IrsaMgrJNIListener *jniListener = new IrsaMgrJNIListener(env, thiz, weak_this);
    mgr->setListener(dynamic_cast<IrsaMgrListener*>(jniListener));
#else
    mgr->setListener(std::make_shared<IrsaMgrJNIListener>(env, thiz, weak_this));
#endif

#ifndef __SMART_POINTER__
    setIrsaMgr(env, thiz, mgr);
#else
    setIrsaMgr(env, thiz, mgr.get());
#endif

    JNIBuildVersion::init();
}


static int android_irsa_IrsaMgr_getVersionDescription(JNIEnv *env, jobject thiz, jbyteArray byteVersionDescription, jint bufSize) {
    LOGD("getVersionDescription");
    if(nullptr == byteVersionDescription) {
        return 0;
    }

    jbyte * pDescription = env->GetByteArrayElements(byteVersionDescription, JNI_FALSE);
    if(nullptr == pDescription) {
        return 0;
    }

    strncpy((char *)pDescription, IRSA_VERSION, bufSize);
    if(nullptr != pDescription) {
        env->ReleaseByteArrayElements(byteVersionDescription, pDescription, JNI_FALSE);
    }

    return strlen(IRSA_VERSION);
}


static JNINativeMethod sMethods[] = {
    {"native_init",             "()V",                     (void *)android_irsa_IrsaMgr_native_init},
    {"native_setup",            "(Ljava/lang/Object;)V",   (void *)android_irsa_IrsaMgr_native_setup},
    {"native_release",          "()V",                     (void *)android_irsa_IrsaMgr_native_release},
    {"getVersionDescription",   "([BI)I",                  (void *)android_irsa_IrsaMgr_getVersionDescription}
};



int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* gMethods, int numMethods) {

    jclass clazz;
    LOGD("registerNativeMethods for '%s'", className);
    clazz = env->FindClass(className);
    if (clazz == nullptr) {
        LOGD("unable to find class '%s'", className);
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGD("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


static void dumpBuildInformation(JNIEnv *env) {
    jclass clazz = env->FindClass("android/os/Build");
    if (clazz == nullptr) {
        return;
    }

    std::map<std::string, std::string> buildInfo;
    static const char* BUILD_FIELDS[] = { "BOARD",
                                          "BOOTLOADER",
                                          "BRAND",
                                          "CPU_ABI",
                                          "CPU_ABI2",
                                          "DEVICE",
                                          "DISPLAY",
                                          "FINGERPRINT",
                                          "HARDWARE",
                                          "HOST",
                                          "ID",
                                          "MANUFACTURER",
                                          "MODEL",
                                          "PRODUCT",
                                          "SERIAL",
                                          "TAGS",
                                          "TYPE",
                                          "USER"
                                        };

    for( int i = 0 ; i < sizeof(BUILD_FIELDS) / sizeof(const char*); ++i ) {
        jfieldID id = env->GetStaticFieldID(clazz, BUILD_FIELDS[i], "Ljava/lang/String;");
        jstring jstr = (jstring)env->GetStaticObjectField(clazz, id);
        const char* str = env->GetStringUTFChars(jstr, nullptr);
        buildInfo.insert(std::pair<std::string, std::string>(BUILD_FIELDS[i], str));
        env->ReleaseStringUTFChars(jstr,str);
        env->DeleteLocalRef(jstr);
    }

    for (auto &item : buildInfo) {
        LOGV("%s: %s\n", item.first.c_str(), item.second.c_str());
    }

    env->DeleteLocalRef( clazz );
    buildInfo.clear();
}


int register_android_irsa_IrsaMgr(JavaVM *vm) {
    LOGD("register JNI functions");

    JNIEnv* env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv failed");
        return JNI_FALSE;
    }

    dumpBuildInformation(env);

    if (!registerNativeMethods(env, classPathName,
                sMethods, sizeof(sMethods) / sizeof(sMethods[0]))) {
        return JNI_FALSE;
    }

    return JNI_VERSION_1_6;
}


void unregister_android_irsa_IrsaMgr(JavaVM *vm) {
    LOGD("unregister");

    JNIEnv* env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) {
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


extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGV("JNI_OnLoad");
    g_jvm = vm;

    int retValue = 0;

    rs2::log_to_console(RS2_LOG_SEVERITY_INFO);
    irsa::JNIEnvManager::init();

    retValue = irsa::register_android_irsa_IrsaMgr(vm);
    CHECK(retValue == JNI_VERSION_1_6);

    retValue = irsa::register_android_irsa_IrsaPreview(vm);
    CHECK(retValue == JNI_VERSION_1_6);

    return retValue;
}


extern "C" void JNI_OnUnLoad(JavaVM* vm, void* reserved) {
    LOGV("JNI_OnUnLoad");
    irsa::unregister_android_irsa_IrsaPreview(vm);
    irsa::unregister_android_irsa_IrsaMgr(vm);
}
