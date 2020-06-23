// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include <memory>
#include <vector>
#include <string>
#include "error.h"
#include "../../../include/librealsense2/h/rs_internal.h"
//#include "../fw-logger/rs-fw-logger.h"

// Fw Logger methods

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_FwLogger_nCreateFwLogMsg(JNIEnv *env, jclass clazz,
                                                                   jlong fw_logger_handle) {
    rs2_error* e = NULL;
    rs2_firmware_log_message* msg = rs2_create_fw_log_message(reinterpret_cast<rs2_device*>(fw_logger_handle), &e);
    handle_error(env, e);
    return (jlong)msg;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_FwLogger_nGetFwLog(JNIEnv *env, jclass clazz,
                                                        jlong fw_logger_handle, jlong fw_log_msg_handle) {
    rs2_error* e = NULL;
    rs2_firmware_log_message* log_msg = reinterpret_cast<rs2_firmware_log_message*>(fw_logger_handle);
    int result = rs2_get_fw_log(reinterpret_cast<rs2_device*>(fw_logger_handle), &log_msg, &e);
    handle_error(env, e);

    bool resultBool = (result == 1) ? true : false;
    return (jboolean)resultBool;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_FwLogger_nGetFlashLog(JNIEnv *env, jclass clazz,
                                                         jlong fw_logger_handle, jlong fw_log_msg_handle) {
    rs2_error* e = NULL;
    int result = rs2_get_flash_log(reinterpret_cast<rs2_device*>(fw_logger_handle),
                                reinterpret_cast<rs2_firmware_log_message**>(&fw_log_msg_handle), &e);
    handle_error(env, e);

    bool resultBool = (result == 1) ? true : false;
    return (jboolean)resultBool;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_FwLogger_nCreateFwLogParsedMsg(JNIEnv *env, jclass clazz,
                                                               jlong fw_logger_handle) {
    rs2_error* e = NULL;
    rs2_firmware_log_parsed_message* msg = rs2_create_fw_log_parsed_message(reinterpret_cast<rs2_device*>(fw_logger_handle), &e);
    handle_error(env, e);
    return (jlong)msg;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_FwLogger_nInitParser(JNIEnv *env, jclass clazz,
                                                               jlong fw_logger_handle,
                                                               jstring xml_content) {
    const char* xml_content_arr = env->GetStringUTFChars(xml_content, 0);

    rs2_error* e = NULL;
    int result = rs2_init_fw_log_parser(reinterpret_cast<rs2_device*>(fw_logger_handle), xml_content_arr, &e);
    handle_error(env, e);

    env->ReleaseStringUTFChars(xml_content, xml_content_arr);

    bool resultBool = (result == 1) ? true : false;
    return (jboolean)resultBool;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_FwLogger_nParseFwLog(JNIEnv *env, jclass clazz,
                                                            jlong fw_logger_handle, jlong fw_log_msg_handle,
                                                            jlong parsed_msg_handle) {
    rs2_error* e = NULL;
    int result = rs2_parse_firmware_log(reinterpret_cast<rs2_device*>(fw_logger_handle),
                                   reinterpret_cast<rs2_firmware_log_message*>(fw_logger_handle),
                                   reinterpret_cast<rs2_firmware_log_parsed_message*>(parsed_msg_handle), &e);
    handle_error(env, e);

    bool resultBool = (result == 1) ? true : false;
    return (jboolean)resultBool;
}

// Fw Log Msg methods

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FwLogMsg_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_fw_log_message(reinterpret_cast<rs2_firmware_log_message*>(handle));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_FwLogMsg_nGetSeverity(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    rs2_log_severity severity = rs2_fw_log_message_severity(reinterpret_cast<rs2_firmware_log_message*>(handle), &e);
    handle_error(env, e);
    return (jint)severity;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_FwLogMsg_nGetSeverityStr(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    rs2_log_severity severity = rs2_fw_log_message_severity(reinterpret_cast<rs2_firmware_log_message*>(handle), &e);
    handle_error(env, e);

    const char* severity_string = rs2_log_severity_to_string(severity);
    return env->NewStringUTF(severity_string);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_FwLogMsg_nGetTimestamp(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    int timestamp = rs2_fw_log_message_timestamp(reinterpret_cast<rs2_firmware_log_message*>(handle), &e);
    handle_error(env, e);
    return (jint)timestamp;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_intel_realsense_librealsense_FwLogMsg_nGetData(JNIEnv *env, jclass type,
                                                        jlong handle,jbyteArray input_buffer) {
    jbyte *buffer = env->GetByteArrayElements(input_buffer, NULL);
    jsize length = env->GetArrayLength(input_buffer);
    std::vector<uint8_t> buff(reinterpret_cast<uint8_t*>(buffer), reinterpret_cast<uint8_t*>(buffer) + length);

    rs2_error* e = NULL;
    int size = rs2_fw_log_message_size(reinterpret_cast<rs2_firmware_log_message*>(handle), &e);
    handle_error(env, e);

    const unsigned char* message_data = rs2_fw_log_message_data(reinterpret_cast<rs2_firmware_log_message*>(handle), &e);
    handle_error(env, e);

    env->ReleaseByteArrayElements(input_buffer, buffer, 0);
    jbyteArray rv = env->NewByteArray(size);
    env->SetByteArrayRegion (rv, 0, size, reinterpret_cast<const jbyte *>(message_data));
    return rv;
}


//Parsed Fw Log Msg methods

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FwLogParsedMsg_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_fw_log_parsed_message(reinterpret_cast<rs2_firmware_log_parsed_message*>(handle));
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_FwLogParsedMsg_nGetMessage(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    const char* message = rs2_get_fw_log_parsed_message(reinterpret_cast<rs2_firmware_log_parsed_message*>(handle), &e);
    handle_error(env, e);

    return env->NewStringUTF(message);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_FwLogParsedMsg_nGetFileName(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    const char* file_name = rs2_get_fw_log_parsed_file_name(reinterpret_cast<rs2_firmware_log_parsed_message*>(handle), &e);
    handle_error(env, e);

    return env->NewStringUTF(file_name);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_FwLogParsedMsg_nGetThreadName(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    const char* thread_name = rs2_get_fw_log_parsed_thread_name(reinterpret_cast<rs2_firmware_log_parsed_message*>(handle), &e);
    handle_error(env, e);

    return env->NewStringUTF(thread_name);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_FwLogParsedMsg_nGetSeverity(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    rs2_log_severity severity = rs2_get_fw_log_parsed_severity(reinterpret_cast<rs2_firmware_log_parsed_message*>(handle), &e);
    handle_error(env, e);

    const char* severity_string = rs2_log_severity_to_string(severity);
    return env->NewStringUTF(severity_string);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_FwLogParsedMsg_nGetLine(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    int line = rs2_get_fw_log_parsed_line(reinterpret_cast<rs2_firmware_log_parsed_message*>(handle), &e);
    handle_error(env, e);
    return (jint)line;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_FwLogParsedMsg_nGetTimestamp(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    int timestamp = rs2_get_fw_log_parsed_timestamp(reinterpret_cast<rs2_firmware_log_parsed_message*>(handle), &e);
    handle_error(env, e);
    return (jint)timestamp;
}



/*std::shared_ptr<android_fw_logger> g_fw_logger;
#define TAG "rs_fw_log"

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FwLogger_nStartReadingFwLogs(JNIEnv *env, jclass clazz,
        jstring file_path) {
    const char *filePath = env->GetStringUTFChars(file_path, 0);
    g_fw_logger = std::make_shared<android_fw_logger>(filePath);

    env->ReleaseStringUTFChars(file_path, filePath);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FwLogger_nStopReadingFwLogs(JNIEnv *env, jclass clazz) {
    g_fw_logger.reset();
}*/
