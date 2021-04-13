// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/h/rs_pipeline.h"
#include "../../../include/librealsense2/hpp/rs_device.hpp"
#include "../../../include/librealsense2/hpp/rs_frame.hpp"
#include "../../api.h"

#include "jni_logging.h"
#include "jni_user.h"

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nCreate(JNIEnv *env, jclass type,
                                                       jlong context) {
    rs2_error* e = NULL;
    rs2_pipeline* rv = rs2_create_pipeline(reinterpret_cast<rs2_context *>(context), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStart(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    auto rv = rs2_pipeline_start(reinterpret_cast<rs2_pipeline *>(handle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

static rs_jni_userdata pdata = {NULL, 0, JNI_FALSE, NULL, NULL};

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStartWithCallback(JNIEnv *env, jclass type, jlong handle, jobject jcb) {
    rs2_error* e = NULL;

    // get the Java VM interface associated with the current thread
    int status = env->GetJavaVM(&(pdata.jvm));
    if (status != 0)
    {
        LRS_JNI_LOGE("Failed to get JVM in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return NULL;
    }

    // get JVM version
    pdata.version = env->GetVersion();

    // Creates a new global reference to the java callback object referred to by the jcb argument
    jobject callback = env->NewGlobalRef(jcb);
    if (callback == NULL)
    {
        LRS_JNI_LOGE("Failed to create global reference to java callback in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return NULL;
    }
    pdata.jcb = callback;

    // find Frame class
    jclass frameclass = env->FindClass("com/intel/realsense/librealsense/Frame");
    if(frameclass == NULL)
    {
        LRS_JNI_LOGE("Failed to find Frame java class in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return NULL;
    }

    // Creates a new global reference to the java Frame class
    jclass fclass = (jclass) env->NewGlobalRef(frameclass);
    if (fclass == NULL)
    {
        LRS_JNI_LOGE("Failed to global reference to the Frame java class in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return NULL;
    }
    pdata.frameclass = fclass;

    auto cb = [&](rs2::frame f) {
        rs2_error* e = nullptr;

        unsigned long long int fn = f.get_frame_number();
//        check_error(e);

        int size = f.get_data_size();
//        check_error(e);

        LRS_JNI_LOGD("on_frame at line %d. frame number:%llu, size:%d", __LINE__, fn, size);

        JNIEnv *cb_thread_env = NULL;
        int env_state = pdata.jvm->GetEnv((void **)&cb_thread_env,pdata.version);

        if(env_state == JNI_EDETACHED){
            if(pdata.jvm->AttachCurrentThread(&cb_thread_env,NULL) != JNI_OK){
                return NULL;
            }
            pdata.attached = JNI_TRUE;
        }

        jobject callback = pdata.jcb;

        if(cb_thread_env){
            jclass usercb = cb_thread_env->GetObjectClass(callback);
            if(usercb == NULL){
                LRS_JNI_LOGE("cannot find user callback class ...");
                pdata.jvm->DetachCurrentThread();
                return NULL;
            }

            jmethodID methodid = cb_thread_env->GetMethodID(usercb, "onFrame", "(Lcom/intel/realsense/librealsense/Frame;)V");
            if(methodid == NULL){
                LRS_JNI_LOGE("cannot find method onFrame in user callback");
                pdata.jvm->DetachCurrentThread();
                return NULL;
            }

            jclass frameclass = pdata.frameclass;
            if(frameclass == NULL){
                LRS_JNI_LOGE("cannot find Frame class...");
                pdata.jvm->DetachCurrentThread();
                return NULL;
            }

            // get the Frame class constructor
            jmethodID frame_constructor = cb_thread_env->GetMethodID(frameclass, "<init>", "(J)V");

            // create a Frame object
            jobject frame = cb_thread_env->NewObject(frameclass, frame_constructor, (jlong) f.get());

            // invoke the java callback with the frame
            cb_thread_env->CallVoidMethod(callback,methodid, frame);
        }

        if(pdata.attached){
            pdata.jvm->DetachCurrentThread();
        }

        cb_thread_env = NULL;
        return NULL;
    };

    LRS_JNI_LOGD("Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);

    auto rv = rs2_pipeline_start_with_callback_cpp(reinterpret_cast<rs2_pipeline *>(handle), new rs2::frame_callback<decltype(cb)>(cb), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStartWithConfig(JNIEnv *env, jclass type,
                                                                jlong handle, jlong configHandle) {
    rs2_error *e = NULL;
    auto rv = rs2_pipeline_start_with_config(reinterpret_cast<rs2_pipeline *>(handle),
                                             reinterpret_cast<rs2_config *>(configHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStartWithConfigAndCallback(JNIEnv *env, jclass type,
                                                                jlong handle, jlong configHandle, jobject jcb) {
    rs2_error *e = NULL;

    // get the Java VM interface associated with the current thread
    int status = env->GetJavaVM(&(pdata.jvm));
    if (status != 0)
    {
        LRS_JNI_LOGE("Failed to get JVM in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return NULL;
    }

    // get JVM version
    pdata.version = env->GetVersion();

    // Creates a new global reference to the java callback object referred to by the jcb argument
    jobject callback = env->NewGlobalRef(jcb);
    if (callback == NULL)
    {
        LRS_JNI_LOGE("Failed to create global reference to java callback in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return NULL;
    }
    pdata.jcb = callback;

    // find Frame class
    jclass frameclass = env->FindClass("com/intel/realsense/librealsense/FrameSet");
    if(frameclass == NULL)
    {
        LRS_JNI_LOGE("Failed to find Frame java class in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return NULL;
    }

    // Creates a new global reference to the java Frame class
    jclass fclass = (jclass) env->NewGlobalRef(frameclass);
    if (fclass == NULL)
    {
        LRS_JNI_LOGE("Failed to global reference to the Frame java class in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return NULL;
    }
    pdata.frameclass = fclass;

    auto cb = [&](rs2::frame f) {
        rs2_error* e = nullptr;

        unsigned long long int fn = f.get_frame_number();
//        check_error(e);

        int size = f.get_data_size();
//        check_error(e);

        LRS_JNI_LOGD("on_frame at line %d. frame number:%llu, size:%d", __LINE__, fn, size);

        JNIEnv *cb_thread_env = NULL;
        int env_state = pdata.jvm->GetEnv((void **)&cb_thread_env,pdata.version);

        if(env_state == JNI_EDETACHED){
            if(pdata.jvm->AttachCurrentThread(&cb_thread_env,NULL) != JNI_OK){
                return NULL;
            }
            pdata.attached = JNI_TRUE;
        }

        jobject callback = pdata.jcb;

        if(cb_thread_env){
            jclass usercb = cb_thread_env->GetObjectClass(callback);
            if(usercb == NULL){
                LRS_JNI_LOGE("cannot find user callback class ...");
                pdata.jvm->DetachCurrentThread();
                return NULL;
            }

            jmethodID methodid = cb_thread_env->GetMethodID(usercb, "onFrame", "(Lcom/intel/realsense/librealsense/Frame;)V");
            if(methodid == NULL){
                LRS_JNI_LOGE("cannot find method onFrame in user callback");
                pdata.jvm->DetachCurrentThread();
                return NULL;
            }

            jclass frameclass = pdata.frameclass;
            if(frameclass == NULL){
                LRS_JNI_LOGE("cannot find Frame class...");
                pdata.jvm->DetachCurrentThread();
                return NULL;
            }

            // get the Frame class constructor
            jmethodID frame_constructor = cb_thread_env->GetMethodID(frameclass, "<init>", "(J)V");

            // create a Frame object
            jobject frame = cb_thread_env->NewObject(frameclass, frame_constructor, (jlong) f.get());

            // invoke the java callback with the frame
            cb_thread_env->CallVoidMethod(callback,methodid, frame);
        }

        if(pdata.attached){
            pdata.jvm->DetachCurrentThread();
        }

        cb_thread_env = NULL;
        return NULL;
    };

    LRS_JNI_LOGD("Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);

    auto rv = rs2_pipeline_start_with_config_and_callback_cpp(reinterpret_cast<rs2_pipeline *>(handle),
                                             reinterpret_cast<rs2_config *>(configHandle), new rs2::frame_callback<decltype(cb)>(cb), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStop(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    rs2_pipeline_stop(reinterpret_cast<rs2_pipeline *>(handle), &e);
    handle_error(env, e);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nDelete(JNIEnv *env, jclass type,
                                                       jlong handle) {
    rs2_delete_pipeline(reinterpret_cast<rs2_pipeline *>(handle));
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nWaitForFrames(JNIEnv *env, jclass type,
                                                              jlong handle, jint timeout) {
    rs2_error* e = NULL;
    rs2_frame *rv = rs2_pipeline_wait_for_frames(reinterpret_cast<rs2_pipeline *>(handle), timeout, &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_PipelineProfile_nDelete(JNIEnv *env, jclass type,
                                                              jlong handle) {
    rs2_delete_pipeline_profile(reinterpret_cast<rs2_pipeline_profile *>(handle));
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_PipelineProfile_nGetDevice(JNIEnv *env, jclass type,
                                                              jlong handle) {
    rs2_error* e = NULL;
    rs2_device *rv = rs2_pipeline_profile_get_device(reinterpret_cast<rs2_pipeline_profile *>(handle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}
