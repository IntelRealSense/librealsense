// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"
#include <fstream>
#include "../../../include/librealsense2/h/rs_internal.h"
#include "../../../common/parser.hpp"   //needed for auto completion in nGetCommands


extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_intel_realsense_librealsense_DebugProtocol_nSendAndReceiveRawData(JNIEnv *env, jclass type,
                                                                           jlong handle,
                                                                           jbyteArray buffer_) {
    jbyte *buffer = env->GetByteArrayElements(buffer_, NULL);
    jsize length = env->GetArrayLength(buffer_);

    rs2_error* e = NULL;
    std::shared_ptr<const rs2_raw_data_buffer> response_bytes(
            rs2_send_and_receive_raw_data(reinterpret_cast<rs2_device*>(handle), (void*)buffer, (int)length, &e),
            rs2_delete_raw_data);

    int response_size = rs2_get_raw_data_size(response_bytes.get(), &e);
    handle_error(env, e);

    const unsigned char* response_start = rs2_get_raw_data(response_bytes.get(), &e);
    handle_error(env, e);

    env->ReleaseByteArrayElements(buffer_, buffer, 0);
    jbyteArray rv = env->NewByteArray(response_size);
    env->SetByteArrayRegion (rv, 0, response_size, reinterpret_cast<const jbyte *>(response_start));

    return rv;
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_intel_realsense_librealsense_DebugProtocol_nSendAndReceiveData(JNIEnv *env, jclass clazz,
                                                                        jlong handle,
                                                                        jstring filePath,
                                                                        jstring command) {
    const char *file_path = env->GetStringUTFChars(filePath, 0);
    const char *line = env->GetStringUTFChars(command, 0);

    std::ifstream f(file_path);
    std::string xml_content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    rs2_error* e = NULL;

    //creating terminal parser
    std::shared_ptr<rs2_terminal_parser> terminal_parser = std::shared_ptr<rs2_terminal_parser>(
            rs2_create_terminal_parser(xml_content.c_str(), &e),
            rs2_delete_terminal_parser);
    handle_error(env, e);

    //using parser to get bytes command from the string command
    int line_length = strlen(line);
    std::shared_ptr<const rs2_raw_data_buffer> command_buffer(
            rs2_terminal_parse_command(terminal_parser.get(), line, line_length, &e),
            rs2_delete_raw_data);
    handle_error(env, e);

    int command_size = rs2_get_raw_data_size(command_buffer.get(), &e);
    handle_error(env, e);

    const unsigned char* command_start = rs2_get_raw_data(command_buffer.get(), &e);
    handle_error(env, e);

    //calling send_receive to get the bytes response
    std::shared_ptr<const rs2_raw_data_buffer> response_bytes(
            rs2_send_and_receive_raw_data(reinterpret_cast<rs2_device*>(handle), (void*)command_start, command_size, &e),
            rs2_delete_raw_data);

    int response_size = rs2_get_raw_data_size(response_bytes.get(), &e);
    handle_error(env, e);

    const unsigned char* response_start = rs2_get_raw_data(response_bytes.get(), &e);
    handle_error(env, e);

    //using parser to get response string from response bytes and string command
    std::shared_ptr<const rs2_raw_data_buffer> response_buffer(
            rs2_terminal_parse_response(terminal_parser.get(), line, line_length,
                                       (void*)response_start, response_size, &e),
            rs2_delete_raw_data);
    handle_error(env, e);

    int response_buffer_size = rs2_get_raw_data_size(response_buffer.get(), &e);
    handle_error(env, e);

    const unsigned char* response_buffer_start = rs2_get_raw_data(response_buffer.get(), &e);
    handle_error(env, e);

    //releasing jni resources
    env->ReleaseStringUTFChars(command, line);
    env->ReleaseStringUTFChars(filePath, file_path);

    //assigning return value
    jbyteArray rv = env->NewByteArray(response_buffer_size);
    env->SetByteArrayRegion(rv, 0, response_buffer_size,
                            reinterpret_cast<const jbyte *>(response_buffer_start));

    return rv;
}

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_intel_realsense_librealsense_DebugProtocol_nGetCommands(JNIEnv *env, jclass clazz,
                                                                 jstring filePath) {
    const char *file_path = env->GetStringUTFChars(filePath, 0);

    jobjectArray rv;
    commands_xml cmd_xml;
    bool sts = parse_xml_from_file(file_path, cmd_xml);
    if(sts)
    {
        rv = (jobjectArray)env->NewObjectArray(cmd_xml.commands.size(), env->FindClass("java/lang/String"),env->NewStringUTF(""));
        int i = 0;
        for(auto&& c : cmd_xml.commands)
            env->SetObjectArrayElement(rv, i++, env->NewStringUTF(c.first.c_str()));
    }
    env->ReleaseStringUTFChars(filePath, file_path);

    return(rv);
}

