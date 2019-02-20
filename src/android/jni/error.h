#ifndef LIBREALSENSE_JNI_ERROR_H
#define LIBREALSENSE_JNI_ERROR_H

#include <jni.h>
#include "../../../include/librealsense2/rs.h"

void handle_error(JNIEnv *env, rs2_error* error);
#endif
