#include <jni.h>
#ifndef NO_JDK
#include "../include/librealsense/rs.h"

static jobject make_object(JNIEnv * env, const char * classname, void * handle)
{
    jclass cl = (*env)->FindClass(env, classname);
    jmethodID mid = (*env)->GetMethodID(env, cl, "<init>", "(J)V");
    return (*env)->NewObject(env, cl, mid, (jlong)handle);
}

static void * get_object(JNIEnv * env, jobject obj)
{
    jclass cl = (*env)->GetObjectClass(env, obj);
    jfieldID fid = (*env)->GetFieldID(env, cl, "handle", "J");
    return (void *)(*env)->GetLongField(env, obj, fid);
}

static jobject make_enum(JNIEnv * env, const char * classname, int code)
{
    char sig[1024];
    sprintf(sig, "()[L%s;", classname);
    jclass cl = (*env)->FindClass(env, classname);
    jmethodID mid = (*env)->GetStaticMethodID(env, cl, "values", sig);
    jobject values = (*env)->CallStaticObjectMethod(env, cl, mid);
    return (*env)->GetObjectArrayElement(env, values, code); // Note: code MUST match ordinal value for now
}

static int get_enum(JNIEnv * env, jobject obj)
{
    jclass cl = (*env)->GetObjectClass(env, obj);
    jfieldID fid = (*env)->GetFieldID(env, cl, "code", "I");
    return (*env)->GetIntField(env, obj, fid);
}

static jobject make_integer(JNIEnv * env, int value)
{
    jclass cl = (*env)->FindClass(env, "java/lang/Integer");
    jmethodID mid = (*env)->GetMethodID(env, cl, "<init>", "(I)V");
    return (*env)->NewObject(env, cl, mid, (jint)value);
}

static jobject make_float_array(JNIEnv * env, const float * values, int count)
{
    jarray arr = (*env)->NewFloatArray(env, count);
    (*env)->SetFloatArrayRegion(env, arr, 0, count, values);
    return arr;
}

static void set_out_param(JNIEnv * env, jobject param, jobject value)
{
    jclass cl = (*env)->GetObjectClass(env, param);
    jfieldID fid = (*env)->GetFieldID(env, cl, "value", "Ljava/lang/Object;");
    (*env)->SetObjectField(env, param, fid, value);
}

static void write_intrinsics(JNIEnv * env, jobject obj, const rs_intrinsics * val)
{
    jclass cl = (*env)->GetObjectClass(env, obj);
    (*env)->SetIntField(env, obj, (*env)->GetFieldID(env, cl, "width", "I"), val->width);
    (*env)->SetIntField(env, obj, (*env)->GetFieldID(env, cl, "height", "I"), val->height);
    (*env)->SetFloatField(env, obj, (*env)->GetFieldID(env, cl, "PPX", "F"), val->ppx);
    (*env)->SetFloatField(env, obj, (*env)->GetFieldID(env, cl, "PPY", "F"), val->ppy);
    (*env)->SetFloatField(env, obj, (*env)->GetFieldID(env, cl, "FX", "F"), val->fx);
    (*env)->SetFloatField(env, obj, (*env)->GetFieldID(env, cl, "FY", "F"), val->fy);
    (*env)->SetObjectField(env, obj, (*env)->GetFieldID(env, cl, "model", "Lcom/intel/rs/Distortion;"), make_enum(env, "com/intel/rs/Distortion", val->model));
    (*env)->SetObjectField(env, obj, (*env)->GetFieldID(env, cl, "coeffs", "[F"), make_float_array(env, val->coeffs, 5));
}

static void write_extrinsics(JNIEnv * env, jobject obj, const rs_extrinsics * val)
{
    jclass cl = (*env)->GetObjectClass(env, obj);
    (*env)->SetObjectField(env, obj, (*env)->GetFieldID(env, cl, "rotation", "[F"), make_float_array(env, val->rotation, 9));
    (*env)->SetObjectField(env, obj, (*env)->GetFieldID(env, cl, "translation", "[F"), make_float_array(env, val->translation, 3));
}

static void handle_error(JNIEnv * env, rs_error * e)
{
    if(e)
    {
        char buffer[2048];
        sprintf(buffer, "error calling %s(%s):\n  %s\n", rs_get_failed_function(e), rs_get_failed_args(e), rs_get_error_message(e));
        rs_free_error(e);
        jclass cl = (*env)->FindClass(env, "java/lang/Exception");
        (*env)->ThrowNew(env, cl, buffer);
    }
}

JNIEXPORT jlong JNICALL Java_com_intel_rs_Context_create(JNIEnv * env, jobject self)
{
    rs_error * e = NULL;
    rs_context * r = rs_create_context(3, &e);
    handle_error(env, e);
    return (jlong)r;
}

JNIEXPORT void JNICALL Java_com_intel_rs_Context_close(JNIEnv * env, jobject self)
{
    rs_delete_context(get_object(env, self), 0);
}

JNIEXPORT jint JNICALL Java_com_intel_rs_Context_getDeviceCount(JNIEnv * env, jobject self)
{
    rs_error * e = NULL;
    int r = rs_get_device_count(get_object(env, self), &e);
    handle_error(env, e);
    return r;
}

JNIEXPORT jobject JNICALL Java_com_intel_rs_Context_getDevice(JNIEnv * env, jobject self, jint index)
{
    rs_error * e = NULL;
    rs_device * r = rs_get_device(get_object(env, self), index, &e);
    handle_error(env, e);
    return make_object(env, "com/intel/rs/Device", r);
}

JNIEXPORT jstring JNICALL Java_com_intel_rs_Device_getName(JNIEnv * env, jobject self)
{
    rs_error * e = NULL;
    const char * r = rs_get_device_name(get_object(env, self), &e);
    handle_error(env, e);
    return (*env)->NewStringUTF(env, r);
}

JNIEXPORT jstring JNICALL Java_com_intel_rs_Device_getSerial(JNIEnv * env, jobject self)
{
    rs_error * e = NULL;
    const char * r = rs_get_device_serial(get_object(env, self), &e);
    handle_error(env, e);
    return (*env)->NewStringUTF(env, r);
}

JNIEXPORT jstring JNICALL Java_com_intel_rs_Device_getFirmwareVersion(JNIEnv * env, jobject self)
{
    rs_error * e = NULL;
    const char * r = rs_get_device_firmware_version(get_object(env, self), &e);
    handle_error(env, e);
    return (*env)->NewStringUTF(env, r);
}

JNIEXPORT void JNICALL Java_com_intel_rs_Device_getExtrinsics(JNIEnv * env, jobject self, jobject fromStream, jobject toStream, jobject extrin)
{
    rs_error * e = NULL;
    rs_extrinsics c_extrin;
    rs_get_device_extrinsics(get_object(env, self), get_enum(env, fromStream), get_enum(env, toStream), &c_extrin, &e);
    handle_error(env, e);
    write_extrinsics(env, extrin, &c_extrin);
}

JNIEXPORT jfloat JNICALL Java_com_intel_rs_Device_getDepthScale(JNIEnv * env, jobject self)
{
    rs_error * e = NULL;
    float r = rs_get_device_depth_scale(get_object(env, self), &e);
    handle_error(env, e);
    return r;
}

JNIEXPORT jboolean JNICALL Java_com_intel_rs_Device_supportsOption(JNIEnv * env, jobject self, jobject option)
{
    rs_error * e = NULL;
    int r = rs_device_supports_option(get_object(env, self), get_enum(env, option), &e);
    handle_error(env, e);
    return r;
}

JNIEXPORT jint JNICALL Java_com_intel_rs_Device_getStreamModeCount(JNIEnv * env, jobject self, jobject stream)
{
    rs_error * e = NULL;
    int r = rs_get_stream_mode_count(get_object(env, self), get_enum(env, stream), &e);
    handle_error(env, e);
    return r;
}

JNIEXPORT void JNICALL Java_com_intel_rs_Device_getStreamMode(JNIEnv * env, jobject self, jobject stream, jint index, jobject width, jobject height, jobject format, jobject framerate)
{
    rs_error * e = NULL;
    int c_width;
    int c_height;
    rs_format c_format;
    int c_framerate;
    rs_get_stream_mode(get_object(env, self), get_enum(env, stream), index, &c_width, &c_height, &c_format, &c_framerate, &e);
    handle_error(env, e);
    set_out_param(env, width, make_integer(env, c_width));
    set_out_param(env, height, make_integer(env, c_height));
    set_out_param(env, format, make_enum(env, "com/intel/rs/Format", c_format));
    set_out_param(env, framerate, make_integer(env, c_framerate));
}

JNIEXPORT void JNICALL Java_com_intel_rs_Device_enableStream(JNIEnv * env, jobject self, jobject stream, jint width, jint height, jobject format, jint framerate)
{
    rs_error * e = NULL;
    rs_enable_stream(get_object(env, self), get_enum(env, stream), width, height, get_enum(env, format), framerate, &e);
    handle_error(env, e);
}

JNIEXPORT void JNICALL Java_com_intel_rs_Device_enableStreamPreset(JNIEnv * env, jobject self, jobject stream, jobject preset)
{
    rs_error * e = NULL;
    rs_enable_stream_preset(get_object(env, self), get_enum(env, stream), get_enum(env, preset), &e);
    handle_error(env, e);
}

JNIEXPORT void JNICALL Java_com_intel_rs_Device_disableStream(JNIEnv * env, jobject self, jobject stream)
{
    rs_error * e = NULL;
    rs_disable_stream(get_object(env, self), get_enum(env, stream), &e);
    handle_error(env, e);
}

JNIEXPORT jboolean JNICALL Java_com_intel_rs_Device_isStreamEnabled(JNIEnv * env, jobject self, jobject stream)
{
    rs_error * e = NULL;
    int r = rs_is_stream_enabled(get_object(env, self), get_enum(env, stream), &e);
    handle_error(env, e);
    return r;
}

JNIEXPORT void JNICALL Java_com_intel_rs_Device_getStreamIntrinsics(JNIEnv * env, jobject self, jobject stream, jobject intrin)
{
    rs_error * e = NULL;
    rs_intrinsics c_intrin;
    rs_get_stream_intrinsics(get_object(env, self), get_enum(env, stream), &c_intrin, &e);
    handle_error(env, e);
    write_intrinsics(env, intrin, &c_intrin);
}

JNIEXPORT jobject JNICALL Java_com_intel_rs_Device_getStreamFormat(JNIEnv * env, jobject self, jobject stream)
{
    rs_error * e = NULL;
    rs_format r = rs_get_stream_format(get_object(env, self), get_enum(env, stream), &e);
    handle_error(env, e);
    return make_enum(env, "com/intel/rs/Format", r);
}

JNIEXPORT jint JNICALL Java_com_intel_rs_Device_getStreamFramerate(JNIEnv * env, jobject self, jobject stream)
{
    rs_error * e = NULL;
    int r = rs_get_stream_framerate(get_object(env, self), get_enum(env, stream), &e);
    handle_error(env, e);
    return r;
}

JNIEXPORT void JNICALL Java_com_intel_rs_Device_start(JNIEnv * env, jobject self)
{
    rs_error * e = NULL;
    rs_start_device(get_object(env, self), &e);
    handle_error(env, e);
}

JNIEXPORT void JNICALL Java_com_intel_rs_Device_stop(JNIEnv * env, jobject self)
{
    rs_error * e = NULL;
    rs_stop_device(get_object(env, self), &e);
    handle_error(env, e);
}

JNIEXPORT jboolean JNICALL Java_com_intel_rs_Device_isStreaming(JNIEnv * env, jobject self)
{
    rs_error * e = NULL;
    int r = rs_is_device_streaming(get_object(env, self), &e);
    handle_error(env, e);
    return r;
}

JNIEXPORT void JNICALL Java_com_intel_rs_Device_setOption(JNIEnv * env, jobject self, jobject option, jint value)
{
    rs_error * e = NULL;
    rs_set_device_option(get_object(env, self), get_enum(env, option), value, &e);
    handle_error(env, e);
}

JNIEXPORT jint JNICALL Java_com_intel_rs_Device_getOption(JNIEnv * env, jobject self, jobject option)
{
    rs_error * e = NULL;
    int r = rs_get_device_option(get_object(env, self), get_enum(env, option), &e);
    handle_error(env, e);
    return r;
}

JNIEXPORT void JNICALL Java_com_intel_rs_Device_waitForFrames(JNIEnv * env, jobject self)
{
    rs_error * e = NULL;
    rs_wait_for_frames(get_object(env, self), &e);
    handle_error(env, e);
}

JNIEXPORT jint JNICALL Java_com_intel_rs_Device_getFrameTimestamp(JNIEnv * env, jobject self, jobject stream)
{
    rs_error * e = NULL;
    int r = rs_get_frame_timestamp(get_object(env, self), get_enum(env, stream), &e);
    handle_error(env, e);
    return r;
}

JNIEXPORT jlong JNICALL Java_com_intel_rs_Device_getFrameData(JNIEnv * env, jobject self, jobject stream)
{
    rs_error * e = NULL;
    const void * r = rs_get_frame_data(get_object(env, self), get_enum(env, stream), &e);
    handle_error(env, e);
    return (jlong)r;
}
#endif
