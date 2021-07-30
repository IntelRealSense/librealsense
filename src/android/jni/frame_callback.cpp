// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/hpp/rs_frame.hpp"

#include "jni_logging.h"
#include "frame_callback.h"

bool rs_jni_callback_init(JNIEnv *env, jobject jcb, frame_callback_data* ud)
{
    // get the Java VM interface associated with the current thread
    int status = env->GetJavaVM(&(ud->jvm));
    if (status != 0)
    {
        LRS_JNI_LOGE("Failed to get JVM in rs_jni_callback_init at line %d", __LINE__);
        return false;
    }

    // get JVM version
    ud->version = env->GetVersion();

    // Creates a new global reference to the java callback object referred to by the jcb argument
    jobject callback = env->NewGlobalRef(jcb);
    if (callback == NULL)
    {
        LRS_JNI_LOGE("Failed to create global reference to java callback in rs_jni_callback_init at line %d", __LINE__);
        return false;
    }
    ud->frame_cb = callback;

    // find Frame class
    jclass frameclass = env->FindClass("com/intel/realsense/librealsense/Frame");
    if(frameclass == NULL)
    {
        LRS_JNI_LOGE("Failed to find Frame java class in rs_jni_callback_init at line %d", __LINE__);
        return false;
    }

    // Creates a new global reference to the java Frame class
    jclass fclass = (jclass) env->NewGlobalRef(frameclass);
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
    JNIEnv *cb_thread_env = NULL;
    int env_state = ud->jvm->GetEnv((void **)&cb_thread_env, ud->version);

    if(env_state == JNI_EDETACHED){
        if(ud->jvm->AttachCurrentThread(&cb_thread_env,NULL) != JNI_OK){
            return false;
        }
        ud->attached = JNI_TRUE;
    }

    jobject callback = ud->frame_cb;

    if (callback == NULL)
    {
        ud->jvm->DetachCurrentThread();
        return false;
    }

    if(cb_thread_env){
        jclass usercb = cb_thread_env->GetObjectClass(callback);
        if(usercb == NULL){
            LRS_JNI_LOGE("cannot find user callback class ...");
            ud->jvm->DetachCurrentThread();
            return false;
        }

        jmethodID methodid = cb_thread_env->GetMethodID(usercb, "onFrame", "(Lcom/intel/realsense/librealsense/Frame;)V");
        if(methodid == NULL){
            LRS_JNI_LOGE("cannot find method onFrame in user java callback");
            ud->jvm->DetachCurrentThread();
            return false;
        }

        jclass frameclass = ud->frameclass;
        if(frameclass == NULL){
            LRS_JNI_LOGE("cannot find java Frame class...");
            ud->jvm->DetachCurrentThread();
            return false;
        }

        // get the Frame class constructor
        jmethodID frame_constructor = cb_thread_env->GetMethodID(frameclass, "<init>", "(J)V");

        // create a Frame object
        jobject frame = cb_thread_env->NewObject(frameclass, frame_constructor, (jlong) f.get());

        // invoke the java callback with the frame
        cb_thread_env->CallVoidMethod(callback,methodid, frame);
    }

    if(ud->attached){
        ud->jvm->DetachCurrentThread();
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
