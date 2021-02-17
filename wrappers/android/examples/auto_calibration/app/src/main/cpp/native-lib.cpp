#include <jni.h>
#include <unistd.h>
#include <librealsense2/hpp/rs_pipeline.hpp>
#include "librealsense2/rs.h"
#include "librealsense2/h/rs_pipeline.h"
#include "error.h"
#include <vector>
#include <filesystem>

void handle_error(JNIEnv *env, rs2_error *error) {
    if (error) {
        const char *message = rs2_get_error_message(error);
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), message);
    }
}

void handle_error(JNIEnv *env, rs2::error error) {

    const char *message = error.what();
    env->ThrowNew(env->FindClass("java/lang/RuntimeException"), message);
}

rs2::pipeline_profile get_profile(JNIEnv *env, jlong handle) {
    auto _pipeline = reinterpret_cast<rs2_pipeline *>((long) handle);
    rs2_error *e = nullptr;
    auto p = std::shared_ptr<rs2_pipeline_profile>(
            rs2_pipeline_get_active_profile(_pipeline, &e),
            rs2_delete_pipeline_profile);
    handle_error(env, e);
    return p;
}

void
copy_vector_to_jbytebuffer(JNIEnv *env, rs2::calibration_table vector, jobject target_jbuffer) {
    auto new_table_buffer = (uint8_t *) (*env).GetDirectBufferAddress(target_jbuffer);
    auto new_table_buffer_size = (size_t) (*env).GetDirectBufferCapacity(target_jbuffer);
    if (vector.size() <= new_table_buffer_size)
        std::copy(vector.begin(), vector.end(), new_table_buffer);
}



extern "C"
JNIEXPORT jint JNICALL
Java_com_intel_realsense_auto_1calibration_MainActivity_nGetTable(JNIEnv *env, jclass clazz,
                                                                       jlong pipeline_handle,
                                                                       jobject table) {
    try {
        auto profile = get_profile(env, pipeline_handle);
        auto calib_dev = rs2::auto_calibrated_device(profile.get_device());
        auto current_table = calib_dev.get_calibration_table();
        copy_vector_to_jbytebuffer(env, current_table, table);
        return current_table.size();
    } catch (const rs2::error &e) {
        handle_error(env, e);
        return -1;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_auto_1calibration_MainActivity_nSetTable(JNIEnv *env, jclass clazz,
                                                                       jlong pipeline_handle,
                                                                       jobject table,
                                                                       jboolean write) {
    auto profile = get_profile(env, pipeline_handle);
    rs2_error *e = nullptr;
    auto calib_dev = rs2::auto_calibrated_device(profile.get_device());

    uint8_t *new_table_buffer = (uint8_t *) (*env).GetDirectBufferAddress(table);
    size_t new_table_buffer_size = (*env).GetDirectBufferCapacity(table);

    if (new_table_buffer != NULL) {
        rs2_set_calibration_table(profile.get_device().get().get(), new_table_buffer,
                                  new_table_buffer_size, &e);
        handle_error(env, e);
        if (write) {
            calib_dev.write_calibration();
        }
    }
}

extern "C"
JNIEXPORT jfloat JNICALL
Java_com_intel_realsense_auto_1calibration_MainActivity_nRunSelfCal(JNIEnv *env, jclass clazz,
                                                                         jlong pipeline_handle,
                                                                         jobject target_buffer,
                                                                         jstring json_cont) {

    try {
        auto profile = get_profile(env, pipeline_handle);
        float health = MAXFLOAT;
        auto sensor = profile.get_device().first<rs2::depth_sensor>();
        // Set the device to High Accuracy preset of the D400 stereoscopic cameras
        sensor.set_option(RS2_OPTION_VISUAL_PRESET, RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
        auto calib_dev = rs2::auto_calibrated_device(profile.get_device());
        auto json_ptr = (*env).GetStringUTFChars(json_cont, NULL);
        auto new_table_vector = calib_dev.run_on_chip_calibration(std::string(json_ptr), &health);
        copy_vector_to_jbytebuffer(env, new_table_vector, target_buffer);
        return health;
    } catch (const rs2::error &e) {
        handle_error(env, e);
        return -1;
    }

}
extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_auto_1calibration_MainActivity_nTare(JNIEnv *env, jclass clazz,
                                                                   jlong pipeline_handle,
                                                                   jobject target_buffer,
                                                                   jint ground_truth,
                                                                   jstring json_cont) {
    try {
        auto profile = get_profile(env, pipeline_handle);
        auto sensor = profile.get_device().first<rs2::depth_sensor>();
        sensor.set_option(RS2_OPTION_VISUAL_PRESET, RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
        auto calib_dev = rs2::auto_calibrated_device(profile.get_device());
        auto json_ptr = (*env).GetStringUTFChars(json_cont, NULL);
        auto new_table = calib_dev.run_tare_calibration(ground_truth, std::string(json_ptr));
        copy_vector_to_jbytebuffer(env, new_table, target_buffer);
    } catch (const rs2::error &e) {
        handle_error(env, e);
    }
}


