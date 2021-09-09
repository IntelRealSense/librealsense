// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/hpp/rs_frame.hpp"

#include "jni_logging.h"
#include "frame_callback.h"

bool rs_jni_callback_init(JNIEnv *env, jlong handle, jobject jcb, frame_callback_data* ud)
{
    if (ud == NULL)
    {
        LRS_JNI_LOGE("rs_jni_callback_init callback data NULL");
        return false;
    }

    ud->handle = handle;

    // get the Java VM interface associated with the current thread
    int status = env->GetJavaVM(&(ud->jvm));
    if (status != 0)
    {
        LRS_JNI_LOGE("Failed to get JVM in rs_jni_callback_init at line %d", __LINE__);
        return false;
    }

    // get JVM version
    jint version = env->GetVersion();

    if (version == JNI_EDETACHED || version == JNI_EVERSION)
        return false;
    else
        ud->version = version;

    // Creates a new global reference to the java callback object referred to by the jcb argument
    jobject callback = env->NewGlobalRef(jcb);
    if (callback == NULL)
    {
        LRS_JNI_LOGE("Failed to create global reference to java callback in rs_jni_callback_init at line %d", __LINE__);
        return false;
    }
    ud->frame_cb = callback;

    // find Frame class
    jclass frameclass = NULL;

    frameclass = env->FindClass("com/intel/realsense/librealsense/Frame");

    // handling exception
    if(env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    if(frameclass == NULL)
    {
        LRS_JNI_LOGE("Failed to find Frame java class in rs_jni_callback_init at line %d", __LINE__);
        return false;
    }

    // Creates a new global reference to the java Frame class
    jclass fclass = reinterpret_cast<jclass>(env->NewGlobalRef(frameclass));
    if (fclass == NULL)
    {
        LRS_JNI_LOGE("Failed to global reference to the Frame java class in rs_jni_callback_init at line %d", __LINE__);
        return false;
    }
    ud->frameclass = fclass;

    return true;
}

bool rs_jni_cb(rs2::frame f, frame_callback_data* ud)
{
    if (ud == NULL)
    {
        LRS_JNI_LOGE("rs_jni_cb callback data NULL");
        return false;
    }

    if (ud->jvm == NULL)
    {
        LRS_JNI_LOGE("rs_jni_cb jvm NULL");
        return false;
    }

    if (ud->frame_cb == NULL)
    {
        LRS_JNI_LOGE("rs_jni_cb frame callback NULL");
        return false;
    }

    if (ud->frameclass == NULL)
    {
        LRS_JNI_LOGE("rs_jni_cb frame class NULL");
        return false;
    }

    JNIEnv *cb_thread_env = NULL;
    int env_state = ud->jvm->GetEnv((void **)&cb_thread_env, ud->version);

    bool attached = JNI_FALSE;

    if(env_state == JNI_EDETACHED)
    {
        if(ud->jvm->AttachCurrentThread(&cb_thread_env,NULL) != JNI_OK)
        {
            LRS_JNI_LOGE("rs_jni_cb failed to attach Java VM to current thread");
            return false;
        }
        attached = JNI_TRUE;
    }
    else if (env_state == JNI_OK)
    {
        attached = JNI_TRUE;
    }
    else
    {
        LRS_JNI_LOGE("rs_jni_cb fail to get Java VM: %d", env_state);
        return false;
    }

    if(cb_thread_env){
        jobject callback = ud->frame_cb;

        jclass usercb = cb_thread_env->GetObjectClass(callback);
        if(usercb == NULL){
            LRS_JNI_LOGE("cannot find user callback class ...");
            ud->jvm->DetachCurrentThread();
            return false;
        }

        jmethodID methodid = cb_thread_env->GetMethodID(usercb, "onFrame", "(Lcom/intel/realsense/librealsense/Frame;)V");

        if(cb_thread_env->ExceptionCheck())
        {
            cb_thread_env->ExceptionDescribe();
            cb_thread_env->ExceptionClear();
        }

        if(methodid == NULL){
            LRS_JNI_LOGE("cannot find method onFrame in user java callback");
            ud->jvm->DetachCurrentThread();
            return false;
        }

        jclass frameclass = ud->frameclass;

        // get the Frame class constructor
        jmethodID frame_constructor = cb_thread_env->GetMethodID(frameclass, "<init>", "(J)V");

        if(cb_thread_env->ExceptionCheck())
        {
            cb_thread_env->ExceptionDescribe();
            cb_thread_env->ExceptionClear();
        }

        if(frame_constructor == NULL){
            LRS_JNI_LOGE("cannot find frame java class constructor");
            ud->jvm->DetachCurrentThread();
            return false;
        }

        // create a Frame object
        jobject frame = cb_thread_env->NewObject(frameclass, frame_constructor, (jlong) f.get());

        if(cb_thread_env->ExceptionCheck())
        {
            cb_thread_env->ExceptionDescribe();
            cb_thread_env->ExceptionClear();
        }

        if(frame == NULL){
            LRS_JNI_LOGE("Failed create frame java object for user callback");
            ud->jvm->DetachCurrentThread();
            return false;
        }

        // invoke the java callback with the frame
        cb_thread_env->CallVoidMethod(callback,methodid, frame);

        if(cb_thread_env->ExceptionCheck())
        {
            cb_thread_env->ExceptionDescribe();
            cb_thread_env->ExceptionClear();
        }
    }

    if(attached){
        ud->jvm->DetachCurrentThread();
        attached = JNI_FALSE;
    }

    cb_thread_env = NULL;
    return true;
}

void rs_jni_cleanup(JNIEnv *env, frame_callback_data* ud)
{
    if (ud)
    {
        if (ud->frame_cb) { env->DeleteGlobalRef(ud->frame_cb); ud->frame_cb = NULL; }
        if (ud->frameclass) { env->DeleteGlobalRef(ud->frameclass); ud->frameclass = NULL; }
    }
}
