// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include <memory>
#include <vector>
#include "error.h"

#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/hpp/rs_device.hpp"
#include "../../../include/librealsense2/hpp/rs_frame.hpp"
#include "../../api.h"

#include "jni_logging.h"
#include "jni_user.h"

void check_error(rs2_error* e)
{
    if (e)
    {
        LRS_JNI_LOGE("check_error at line %d. rs_error was raised when calling %s(%s):", __LINE__, rs2_get_failed_function(e), rs2_get_failed_args(e));
        LRS_JNI_LOGE("check_error at line %d.     %s", __LINE__, rs2_get_error_message(e));
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nOpen(JNIEnv *env, jclass type, jlong handle, jlong sp) {
    rs2_error* e = nullptr;
    rs2_open(reinterpret_cast<rs2_sensor *>(handle), reinterpret_cast<rs2_stream_profile *>(sp), &e);
    handle_error(env, e);
}

static rs_jni_userdata sdata = {NULL, 0, JNI_FALSE, NULL, NULL};

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nStart(JNIEnv *env, jclass type, jlong handle, jobject jcb) {
    rs2_error* e = nullptr;

    // get the Java VM interface associated with the current thread
    int status = env->GetJavaVM(&(sdata.jvm));
    if (status != 0)
    {
        LRS_JNI_LOGE("Failed to get JVM in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return;
    }

    // get JVM version
    sdata.version = env->GetVersion();

    // Creates a new global reference to the java callback object referred to by the jcb argument
    jobject callback = env->NewGlobalRef(jcb);
    if (callback == NULL)
    {
        LRS_JNI_LOGE("Failed to create global reference to java callback in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return;
    }
    sdata.jcb = callback;

    // find Frame class
    jclass frameclass = env->FindClass("com/intel/realsense/librealsense/Frame");
    if(frameclass == NULL)
    {
        LRS_JNI_LOGE("Failed to find Frame java class in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return;
    }

    // Creates a new global reference to the java Frame class
    jclass fclass = (jclass) env->NewGlobalRef(frameclass);
    if (fclass == NULL)
    {
        LRS_JNI_LOGE("Failed to global reference to the Frame java class in Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);
        return;
    }
    sdata.frameclass = fclass;

    auto cb = [&](rs2::frame f) {
        rs2_error* e = nullptr;

        unsigned long long int fn = f.get_frame_number();
        check_error(e);

        int size = f.get_data_size();
        check_error(e);

        LRS_JNI_LOGD("on_frame at line %d. frame number:%llu, size:%d", __LINE__, fn, size);

        JNIEnv *cb_thread_env = NULL;
        int env_state = sdata.jvm->GetEnv((void **)&cb_thread_env,sdata.version);

        if(env_state == JNI_EDETACHED){
             if(sdata.jvm->AttachCurrentThread(&cb_thread_env,NULL) != JNI_OK){
                return NULL;
            }
            sdata.attached = JNI_TRUE;
        }

        jobject callback = sdata.jcb;

        if(cb_thread_env){
            jclass usercb = cb_thread_env->GetObjectClass(callback);
            if(usercb == NULL){
                LRS_JNI_LOGE("cannot find user callback class ...");
                sdata.jvm->DetachCurrentThread();
                return NULL;
            }

            jmethodID methodid = cb_thread_env->GetMethodID(usercb, "onFrame", "(Lcom/intel/realsense/librealsense/Frame;)V");
            if(methodid == NULL){
                LRS_JNI_LOGE("cannot find method onFrame in user callback");
                sdata.jvm->DetachCurrentThread();
                return NULL;
            }

            jclass frameclass = sdata.frameclass;
            if(frameclass == NULL){
                LRS_JNI_LOGE("cannot find Frame class...");
                sdata.jvm->DetachCurrentThread();
                return NULL;
            }

            // get the Frame class constructor
            jmethodID frame_constructor = cb_thread_env->GetMethodID(frameclass, "<init>", "(J)V");

            // create a Frame object
            jobject frame = cb_thread_env->NewObject(frameclass, frame_constructor, (jlong) f.get());

            // invoke the java callback with the frame
            cb_thread_env->CallVoidMethod(callback,methodid, frame);
        }

        if(sdata.attached){
            sdata.jvm->DetachCurrentThread();
        }

        LRS_JNI_LOGD("after DetachCurrentThread...");
        cb_thread_env = NULL;
        return NULL;
    };

    LRS_JNI_LOGD("Java_com_intel_realsense_librealsense_Sensor_nStart at line %d", __LINE__);

    rs2_start_cpp(reinterpret_cast<rs2_sensor *>(handle), new rs2::frame_callback<decltype(cb)>(cb), &e);
    handle_error(env, e);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nStop(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = nullptr;
    rs2_stop(reinterpret_cast<rs2_sensor *>(handle), &e);
    handle_error(env, e);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nClose(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = nullptr;
    rs2_close(reinterpret_cast<rs2_sensor *>(handle), &e);
    handle_error(env, e);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Sensor_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_sensor(reinterpret_cast<rs2_sensor *>(handle));
}

extern "C"
JNIEXPORT jlongArray JNICALL
Java_com_intel_realsense_librealsense_Sensor_nGetStreamProfiles(JNIEnv *env, jclass type,
                                                                jlong handle) {
    rs2_error* e = nullptr;
    std::shared_ptr<rs2_stream_profile_list> list(
            rs2_get_stream_profiles(reinterpret_cast<rs2_sensor *>(handle), &e),
            rs2_delete_stream_profiles_list);
    handle_error(env, e);

    auto size = rs2_get_stream_profiles_count(list.get(), &e);
    handle_error(env, e);

    std::vector<const rs2_stream_profile*> profiles;

    for (auto i = 0; i < size; i++)
    {
        auto sp = rs2_get_stream_profile(list.get(), i, &e);
        handle_error(env, e);
        profiles.push_back(sp);
    }

    // jlong is 64-bit, but pointer in profiles could be 32-bit, copy element by element
    jlongArray rv = env->NewLongArray(profiles.size());
    for (auto i = 0; i < size; i++)
    {
        env->SetLongArrayRegion(rv, i, 1, reinterpret_cast<const jlong *>(&profiles[i]));
    }
    return rv;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Sensor_nIsSensorExtendableTo(JNIEnv *env, jclass type,
                                                                   jlong handle, jint extension) {
    rs2_error *e = NULL;
    int rv = rs2_is_sensor_extendable_to(reinterpret_cast<const rs2_sensor *>(handle),
                                         static_cast<rs2_extension>(extension), &e);
    handle_error(env, e);
    return rv > 0;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_RoiSensor_nSetRegionOfInterest(JNIEnv *env, jclass clazz,
                                                                     jlong handle, jint min_x,
                                                                     jint min_y, jint max_x,
                                                                     jint max_y) {
    rs2_error* e = nullptr;
    rs2_set_region_of_interest(reinterpret_cast<rs2_sensor *>(handle), min_x, min_y, max_x, max_y, &e);
    handle_error(env, e);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_RoiSensor_nGetRegionOfInterest(JNIEnv *env, jclass type,
                                                                     jlong handle, jobject roi) {
    int min_x, min_y, max_x, max_y;
    rs2_error *e = nullptr;
    rs2_get_region_of_interest(reinterpret_cast<rs2_sensor *>(handle), &min_x, &min_y, &max_x, &max_y, &e);
    handle_error(env, e);

    if(e)
        return;

    jclass clazz = env->GetObjectClass(roi);

    jfieldID min_x_field = env->GetFieldID(clazz, "minX", "I");
    jfieldID min_y_field = env->GetFieldID(clazz, "minY", "I");
    jfieldID max_x_field = env->GetFieldID(clazz, "maxX", "I");
    jfieldID max_y_field = env->GetFieldID(clazz, "maxY", "I");

    env->SetIntField(roi, min_x_field, min_x);
    env->SetIntField(roi, min_y_field, min_y);
    env->SetIntField(roi, max_x_field, max_x);
    env->SetIntField(roi, max_y_field, max_y);
}

extern "C"
JNIEXPORT jfloat JNICALL
Java_com_intel_realsense_librealsense_DepthSensor_nGetDepthScale(JNIEnv *env, jclass clazz,
                                                                     jlong handle) {
    rs2_error* e = nullptr;
    float depthScale = rs2_get_depth_scale(reinterpret_cast<rs2_sensor *>(handle), &e);
    handle_error(env, e);
    return depthScale;
}
