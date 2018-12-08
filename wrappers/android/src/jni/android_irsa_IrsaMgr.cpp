// License: Apache 2.0. See LICENSE file in root directory.

#define LOG_TAG "irsa_IRSA_JNI_MGR"

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
#include "irsa_rs_preview.h"
#ifdef __BUILD_FA__
#include "irsa_fa_preview.h"
#endif


JavaVM *g_jvm;

namespace irsa
{

struct fields_t
{
    jfieldID    context;
    jmethodID   postEventFromNativeID;
    jmethodID   openUVCDevice;
    jmethodID   closeUVCDevice;
    jmethodID   checkUSBVFS;
    jmethodID   getDescriptor;
    jmethodID   getUVCName;
    jclass      gClass;
    jobject     gObject;
    int         uvcFD;
};

static fields_t s_Fields;
static std::mutex s_mutex;
static const char *classPathName = "android/irsa/IrsaMgr";

static void IrsaMgr_native_finalize();

#define IRSA_VERSION "1.6.0"

class IrsaMgrJNIListener: public IrsaMgrListener
{
public:
    IrsaMgrJNIListener(JNIEnv* env, jobject thiz, jobject weak_thiz);
    ~IrsaMgrJNIListener();
    virtual void notify(int msg, int arg1, size_t arg2);
private:
    IrsaMgrJNIListener();
    //jclass      mClass;
    //jobject     mObject;
};


IrsaMgrJNIListener::IrsaMgrJNIListener(JNIEnv* env, jobject thiz, jobject weak_thiz)
{
    LOGD("constructor");
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == nullptr)
    {
        LOGD("Can't find IrsaMgr");
        return;
    }
    //mClass  = (jclass)env->NewGlobalRef(clazz);
    //mObject = env->NewGlobalRef(weak_thiz);
}


IrsaMgrJNIListener::~IrsaMgrJNIListener()
{
    LOGD("destructor");
    //AutoJNIEnv env;
    //env->DeleteGlobalRef(mObject);
    //env->DeleteGlobalRef(mClass);
}


void IrsaMgrJNIListener::notify(int msg, int arg1, size_t arg2)
{
    AutoJNIEnv env;

    jobject obj = nullptr;
    bool releaseLocalRef = false;

    if ((msg == IRSA_INFO) || (msg == IRSA_ERROR))
    {
        const char *strInfo = (const char*)arg2;
        if (strInfo != nullptr)
        {
            CHECK(env.get() != nullptr);
            obj  = (jobject)env->NewStringUTF(strInfo);
            releaseLocalRef = true;
        }
    }

    if (s_Fields.postEventFromNativeID != nullptr && s_Fields.context != nullptr)
    {
        CHECK(env.get() != nullptr);
        env->CallStaticVoidMethod(s_Fields.gClass, s_Fields.postEventFromNativeID, s_Fields.gObject, msg, arg1, arg2, obj);
    }

    if (releaseLocalRef)
    {
        if( obj != nullptr )
        {
            env->DeleteLocalRef(obj);
        }
    }
}


static void setIrsaMgr(JNIEnv* env, jobject thiz, const IrsaMgr *mgr)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    void *tmpP = static_cast<void*>(const_cast<IrsaMgr*>(mgr));
    //need support arm32/arm64/x86/x86_64 toolchain, so:
    size_t  tmpI = reinterpret_cast<size_t>(tmpP);
    env->SetIntField(thiz, s_Fields.context, tmpI);
}


static void android_irsa_IrsaMgr_native_init(JNIEnv *env)
{
    jclass clazz;

    LOGD("native_init");
    clazz = env->FindClass(classPathName);
    CHECK(clazz != nullptr);

    memset(&s_Fields, 0, sizeof(s_Fields));

    s_Fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
    s_Fields.postEventFromNativeID = env->GetStaticMethodID(clazz, "postEventFromNative",
                                     "(Ljava/lang/Object;IIILjava/lang/Object;)V");

    s_Fields.openUVCDevice  = env->GetStaticMethodID(clazz, "openUVCDevice", "()I");
    s_Fields.closeUVCDevice = env->GetStaticMethodID(clazz, "closeUVCDevice", "()V");
    s_Fields.checkUSBVFS    = env->GetStaticMethodID(clazz, "checkUSBVFS", "(Ljava/lang/String;)I");
    s_Fields.getDescriptor  = env->GetStaticMethodID(clazz, "getDescriptor", "([B)I");
    s_Fields.getUVCName     = env->GetStaticMethodID(clazz, "getUVCName", "()Ljava/lang/String;");
    CHECK(s_Fields.postEventFromNativeID != nullptr);
}


static void android_irsa_IrsaMgr_native_release(JNIEnv *env, jobject thiz)
{
    LOGD("native_release");
    static bool bReleased = false;
    if (!bReleased)
    {
        IrsaPreview_native_finalize();
        IrsaMgr_native_finalize();
        bReleased = true;
    }
    else
    {
        LOGD("already released");
    }
}


static void IrsaMgr_native_finalize()
{
    LOGD("native_finalize");
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
    CHECK(mgr != nullptr);
    mgr->setListener(nullptr);

    LOGD("delete global JNI ref");
    AutoJNIEnv env;
    env->DeleteGlobalRef(s_Fields.gClass);
    env->DeleteGlobalRef(s_Fields.gObject);

    IrsaMgr::getInstance().reset();
}


static void android_irsa_IrsaMgr_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
    LOGD("native_setup");
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
    CHECK(mgr != nullptr);

    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == nullptr)
    {
        LOGD("Can't find IrsaMgr");
        return;
    }

    s_Fields.gClass  = (jclass)env->NewGlobalRef(clazz);
    s_Fields.gObject = env->NewGlobalRef(weak_this);

    mgr->setListener(std::make_shared<IrsaMgrJNIListener>(env, thiz, weak_this));

    setIrsaMgr(env, thiz, mgr.get());

    JNIBuildVersion::init();
}


static int android_irsa_IrsaMgr_getVersionDescription(JNIEnv *env, jobject thiz, jbyteArray byteVersionDescription, jint bufSize)
{
    LOGD("getVersionDescription");
    if(nullptr == byteVersionDescription)
    {
        return 0;
    }

    jbyte * pDescription = env->GetByteArrayElements(byteVersionDescription, JNI_FALSE);
    if(nullptr == pDescription)
    {
        return 0;
    }

    strncpy((char *)pDescription, IRSA_VERSION, bufSize);
    if(nullptr != pDescription)
    {
        env->ReleaseByteArrayElements(byteVersionDescription, pDescription, JNI_FALSE);
    }

    return strlen(IRSA_VERSION);
}


static jstring android_irsa_IrsaMgr_formatToString(JNIEnv *env, jobject thiz, jint format)
{
    const char *name = rs2_format_to_string(static_cast<rs2_format>(format));
    if (name != nullptr)
        return env->NewStringUTF(name);
    else
        return env->NewStringUTF("unknown");
}


static jint android_irsa_IrsaMgr_formatFromString(JNIEnv *env, jobject thiz, jstring strFormat)
{
    int value = -1;
    if (strFormat == nullptr)
        return value;

    const char *format = env->GetStringUTFChars(strFormat, 0);
    for (int i = rs2_format::RS2_FORMAT_ANY; i < rs2_format::RS2_FORMAT_COUNT; i++)
    {
        if (0 == strcmp(format, rs2_format_to_string(static_cast<rs2_format>(i))))
        {
            value = i;
            break;
        }
    }

    env->ReleaseStringUTFChars(strFormat, format);
    return value;
}


static void android_irsa_IrsaMgr_setBAGFile(JNIEnv *env, jobject thiz, jstring fileName)
{
    if (fileName == nullptr)
        return;

    const char *name = env->GetStringUTFChars(fileName, 0);
    LOGD("bag file %s", name);

    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
    CHECK(mgr != nullptr);
    std::string bagFile(name);
    mgr->setBAGFile(bagFile);

    env->ReleaseStringUTFChars(fileName, name);
    return;
}


static jboolean android_irsa_IrsaMgr_initIRFA(JNIEnv *env, jobject thiz, jstring jniDataDir)
{
    LOGD("enter JNI initIRFA");
    const char *nativeDataDir = env->GetStringUTFChars(jniDataDir, 0);
    std::string dataDir(nativeDataDir);
    if (dataDir.empty())
        return false;

    jboolean ret = false;

    LOGD("dir %s", dataDir.c_str());
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
    CHECK(mgr != nullptr);
    mgr->setFAAssetsDir(dataDir);

    if (mgr->getPreviewInstance())
    {
        ret =  mgr->getPreviewInstance()->initIRFA(dataDir);
    }
    LOGD("initIRFA return %d", ret);
    return ret;
}


static JNINativeMethod sMethods[] =
{
    {"native_init",             "()V",                     (void *)android_irsa_IrsaMgr_native_init},
    {"native_setup",            "(Ljava/lang/Object;)V",   (void *)android_irsa_IrsaMgr_native_setup},
    {"native_release",          "()V",                     (void *)android_irsa_IrsaMgr_native_release},
    {"getVersionDescription",   "([BI)I",                  (void *)android_irsa_IrsaMgr_getVersionDescription},
    {"initIRFA",                "(Ljava/lang/String;)Z",   (void *)android_irsa_IrsaMgr_initIRFA},
    {"formatToString",          "(I)Ljava/lang/String;",   (void *)android_irsa_IrsaMgr_formatToString},
    {"formatFromString",        "(Ljava/lang/String;)I",   (void *)android_irsa_IrsaMgr_formatFromString},
    {"setBAGFile",              "(Ljava/lang/String;)V",   (void *)android_irsa_IrsaMgr_setBAGFile},
};


int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;
    LOGD("registerNativeMethods for '%s'", className);
    clazz = env->FindClass(className);
    if (clazz == nullptr)
    {
        LOGD("unable to find class '%s'", className);
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0)
    {
        LOGD("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


static void dumpBuildInformation(JNIEnv *env)
{
    jclass clazz = env->FindClass("android/os/Build");
    if (clazz == nullptr)
    {
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

    for( int i = 0 ; i < sizeof(BUILD_FIELDS) / sizeof(const char*); ++i )
    {
        jfieldID id = env->GetStaticFieldID(clazz, BUILD_FIELDS[i], "Ljava/lang/String;");
        jstring jstr = (jstring)env->GetStaticObjectField(clazz, id);
        const char* str = env->GetStringUTFChars(jstr, nullptr);
        buildInfo.insert(std::pair<std::string, std::string>(BUILD_FIELDS[i], str));
        env->ReleaseStringUTFChars(jstr,str);
        env->DeleteLocalRef(jstr);
    }

    for (auto &item : buildInfo)
    {
        LOGD("%s: %s\n", item.first.c_str(), item.second.c_str());
    }

    env->DeleteLocalRef( clazz );
    buildInfo.clear();
}


int register_android_irsa_IrsaMgr(JavaVM *vm)
{
    LOGD("register JNI functions");

    JNIEnv* env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK)
    {
        LOGE("GetEnv failed");
        return JNI_FALSE;
    }

    dumpBuildInformation(env);

    if (!registerNativeMethods(env, classPathName,
                               sMethods, sizeof(sMethods) / sizeof(sMethods[0])))
    {
        return JNI_FALSE;
    }

    return JNI_VERSION_1_6;
}


void unregister_android_irsa_IrsaMgr(JavaVM *vm)
{
    LOGD("unregister");

    JNIEnv* env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK)
    {
        LOGE("GetEnv failed");
        return;
    }

    if(env)
    {
        jclass clazz = env->FindClass(classPathName);
        if (clazz != nullptr)
        {
            env->UnregisterNatives(clazz);
            env->DeleteLocalRef(clazz);
        }
    }
}

}


extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOGD("JNI_OnLoad");
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


extern "C" void JNI_OnUnLoad(JavaVM* vm, void* reserved)
{
    LOGD("JNI_OnUnLoad");
    irsa::unregister_android_irsa_IrsaPreview(vm);
    irsa::unregister_android_irsa_IrsaMgr(vm);
}


namespace irsa
{


extern "C" int irsa_openUVCDevice()
{
    AutoJNIEnv env;
    LOGD("enter");

    if (s_Fields.openUVCDevice != nullptr)
    {
        CHECK(env.get() != nullptr);
        LOGV("open");
        s_Fields.uvcFD = env->CallStaticIntMethod(s_Fields.gClass, s_Fields.openUVCDevice);
        LOGV("uvc fd: %d", s_Fields.uvcFD);
    }
    return s_Fields.uvcFD;
}


extern "C" void irsa_closeUVCDevice()
{
    LOGD("enter");
    AutoJNIEnv env;
    if (s_Fields.closeUVCDevice != nullptr)
    {
        CHECK(env.get() != nullptr);
        env->CallStaticVoidMethod(s_Fields.gClass, s_Fields.closeUVCDevice);
        s_Fields.uvcFD = -1;
    }
}


extern "C" int _openUVCDevice()
{
    AutoJNIEnv env;
    LOGD("enter");

    if (s_Fields.uvcFD > 0) 
    {
        //return s_Fields.uvcFD;
    }

    return irsa_openUVCDevice();
}


extern "C" void _closeUVCDevice()
{
    LOGD("enter");
    //return;

    irsa_closeUVCDevice();
}


extern "C" int _checkUSBVFS(const char *path)
{
    AutoJNIEnv env;
    int found = 0;
    LOGD("path:%s", path);
    if (s_Fields.checkUSBVFS != nullptr)
    {
        CHECK(env.get() != nullptr);
        jstring  _path = env->NewStringUTF(path);
        found =  env->CallStaticIntMethod(s_Fields.gClass, s_Fields.checkUSBVFS, _path);
        env->DeleteLocalRef(_path);
    }
    LOGD("found:%d", found);
    return found;
}


static const int MAX_DESC_LEN = 4096;
static char mDescriptor[MAX_DESC_LEN];
static int  iDescriptor = 0;

extern "C" int _getDescriptor(char *pool)
{
    LOGD("getDescriptor");
    AutoJNIEnv env;
    if (pool == nullptr)
        return 0;

    CHECK(env.get() != nullptr);
    if (iDescriptor > 0)
    {
        LOGD("descriptor length %d", iDescriptor);
        memcpy(pool, mDescriptor, iDescriptor);
        return iDescriptor;
    }

    jbyteArray byteDescription = env->NewByteArray(MAX_DESC_LEN);
    CHECK(byteDescription != nullptr);


    jint bufSize;
    if (s_Fields.getDescriptor != nullptr)
    {
        bufSize = env->CallStaticIntMethod(s_Fields.gClass, s_Fields.getDescriptor, byteDescription);
        LOGD("bufSize %d", bufSize);
        if (bufSize <= 0)
        {
            return 0;
        }
        jbyte * pDescription = env->GetByteArrayElements(byteDescription, JNI_FALSE);
        CHECK(pDescription != nullptr);
        iDescriptor = bufSize;
        LOGD("desc len %d", bufSize);
        memcpy(pool, pDescription, bufSize);
        memcpy(mDescriptor, pDescription, bufSize);
        env->ReleaseByteArrayElements(byteDescription, pDescription, JNI_FALSE);
        return bufSize;
    }

    return 0;
}


extern "C" char* _getUVCName()
{
    LOGD("getUVCName");
    AutoJNIEnv env;
    CHECK(env.get() != nullptr);

    if (s_Fields.getUVCName != nullptr)
    {
        jstring  strName = (jstring)env->CallStaticObjectMethod(s_Fields.gClass, s_Fields.getUVCName);
        if (strName != nullptr)
        {
            const char *name = env->GetStringUTFChars(strName, 0);
            LOGD("uvc name: %s", name);
            return (char*)name;
        }
        else
        {
            LOGD("cann't get uvc name now");
        }
    }

    return NULL;
}


}


