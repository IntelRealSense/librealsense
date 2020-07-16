// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include <memory>
#include "error.h"
#include "../../../include/librealsense2/rs.hpp"
#include "../../../include/librealsense2/rsutil.h"

// helper method for retrieving rs2_intrinsics from intrinsic object
rs2_intrinsics intrinsic_jobject2rs(JNIEnv *env, jobject intrinsic) {
    jclass intrinsic_class = env->GetObjectClass(intrinsic);
    jfieldID width_field = env->GetFieldID(intrinsic_class, "mWidth", "I");
    jfieldID height_field = env->GetFieldID(intrinsic_class, "mHeight", "I");
    jfieldID ppx_field = env->GetFieldID(intrinsic_class, "mPpx", "F");
    jfieldID ppy_field = env->GetFieldID(intrinsic_class, "mPpy", "F");
    jfieldID fx_field = env->GetFieldID(intrinsic_class, "mFx", "F");
    jfieldID fy_field = env->GetFieldID(intrinsic_class, "mFy", "F");
    jfieldID model_field = env->GetFieldID(intrinsic_class, "mModelValue", "I");
    jfieldID coeff_field = env->GetFieldID(intrinsic_class, "mCoeffs", "[F");
    rs2_intrinsics intrinsics;
    intrinsics.width = env->GetIntField(intrinsic, width_field);
    intrinsics.height = env->GetIntField(intrinsic, height_field);
    intrinsics.ppx = env->GetFloatField(intrinsic, ppx_field);
    intrinsics.ppy = env->GetFloatField(intrinsic, ppy_field);
    intrinsics.fx = env->GetFloatField(intrinsic, fx_field);
    intrinsics.fy = env->GetFloatField(intrinsic, fy_field);
    intrinsics.model = (rs2_distortion)env->GetIntField(intrinsic, model_field);

    jobject coeffsObject = env->GetObjectField(intrinsic, coeff_field);
    jfloatArray* coeffsArray = reinterpret_cast<jfloatArray *>(&coeffsObject);
    jfloat * coeffs = env->GetFloatArrayElements(*coeffsArray, NULL);
    memcpy(intrinsics.coeffs, coeffs, 5 * sizeof(float));
    env->ReleaseFloatArrayElements(*coeffsArray, coeffs, 0);

    return intrinsics;
}

// helper method for retrieving rs2_extrinsics from extrinsic object
rs2_extrinsics extrinsic_jobject2rs(JNIEnv *env, jobject extrinsic) {
    rs2_extrinsics extrinsics;
    jclass extrinsic_class = env->GetObjectClass(extrinsic);
    //fill rotation
    jfieldID rotation_field = env->GetFieldID(extrinsic_class, "mRotation", "[F");
    jobject rotationObject = env->GetObjectField(extrinsic, rotation_field);
    jfloatArray* rotationArray = reinterpret_cast<jfloatArray *>(&rotationObject);
    jfloat * rotation = env->GetFloatArrayElements(*rotationArray, NULL);
    memcpy(extrinsics.rotation, rotation, 9 * sizeof(float));
    env->ReleaseFloatArrayElements(*rotationArray, rotation, 0);
    //fill translation
    jfieldID translation_field = env->GetFieldID(extrinsic_class, "mTranslation", "[F");
    jobject translationObject = env->GetObjectField(extrinsic, translation_field);
    jfloatArray* translationArray = reinterpret_cast<jfloatArray *>(&translationObject);
    jfloat * translation = env->GetFloatArrayElements(*translationArray, NULL);
    memcpy(extrinsics.translation, translation, 3 * sizeof(float));
    env->ReleaseFloatArrayElements(*translationArray, translation, 0);

    return extrinsics;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Utils_nProjectPointToPixel(JNIEnv *env, jclass type,
                                                              jobject pixel_2D, jobject intrinsic,
                                                              jobject point_3D) {
    // retrieving float[3] from point_3D
    jclass point_3D_class = env->GetObjectClass(point_3D);
    jfieldID point_x_field = env->GetFieldID(point_3D_class, "mX", "F");
    jfieldID point_y_field = env->GetFieldID(point_3D_class, "mY", "F");
    jfieldID point_z_field = env->GetFieldID(point_3D_class, "mZ", "F");
    float point[3];
    point[0] =  env->GetFloatField(point_3D, point_x_field);
    point[1] =  env->GetFloatField(point_3D, point_y_field);
    point[2] =  env->GetFloatField(point_3D, point_z_field);

    // retrieving rs2_intrinsics from intrinsic object
    rs2_intrinsics intrinsics = intrinsic_jobject2rs(env, intrinsic);

    // preparing struct to be filled by API function
    float pixel[2] = {0.f, 0.f};

    rs2_project_point_to_pixel(pixel, &intrinsics, point);

    // returning pixel into pixel_2D struct
    jclass pixel_2D_class = env->GetObjectClass(pixel_2D);
    jfieldID pixel_x_field = env->GetFieldID(pixel_2D_class, "mX", "F");
    jfieldID pixel_y_field = env->GetFieldID(pixel_2D_class, "mY", "F");
    env->SetFloatField(pixel_2D, pixel_x_field, pixel[0]);
    env->SetFloatField(pixel_2D, pixel_y_field, pixel[1]);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Utils_nDeprojectPixelToPoint(JNIEnv *env, jclass type,
                                                                 jobject point_3D, jobject intrinsic,
                                                                 jobject pixel_2D, jfloat depth) {

    // retrieving float[2] from pixel_2D
    jclass pixel_2D_class = env->GetObjectClass(pixel_2D);
    jfieldID pixel_x_field = env->GetFieldID(pixel_2D_class, "mX", "F");
    jfieldID pixel_y_field = env->GetFieldID(pixel_2D_class, "mY", "F");
    float pixel[2];
    pixel[0] = env->GetFloatField(pixel_2D, pixel_x_field);
    pixel[1] = env->GetFloatField(pixel_2D, pixel_y_field);

    // retrieving rs2_intrinsics from intrinsic object
    rs2_intrinsics intrinsics = intrinsic_jobject2rs(env, intrinsic);

    // preparing struct to be filled by API function
    float point[3] = {0.f, 0.f, 0.f};

    rs2_deproject_pixel_to_point(point, &intrinsics, pixel, depth);

    // returning point into point_3D struct
    jclass point_3D_class = env->GetObjectClass(point_3D);
    jfieldID point_x_field = env->GetFieldID(point_3D_class, "mX", "F");
    jfieldID point_y_field = env->GetFieldID(point_3D_class, "mY", "F");
    jfieldID point_z_field = env->GetFieldID(point_3D_class, "mZ", "F");
    env->SetFloatField(point_3D, point_x_field, point[0]);
    env->SetFloatField(point_3D, point_y_field, point[1]);
    env->SetFloatField(point_3D, point_z_field, point[2]);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Utils_nTransformPointToPoint(JNIEnv *env, jclass type,
                                                                   jobject to_point_3D, jobject extrinsic,
                                                                   jobject from_point_3D) {
    // retrieving float[3] from from_point_3D
    jclass point_3D_class = env->GetObjectClass(from_point_3D);
    jfieldID point_x_field = env->GetFieldID(point_3D_class, "mX", "F");
    jfieldID point_y_field = env->GetFieldID(point_3D_class, "mY", "F");
    jfieldID point_z_field = env->GetFieldID(point_3D_class, "mZ", "F");
    float from_point[3];
    from_point[0] =  env->GetFloatField(from_point_3D, point_x_field);
    from_point[1] =  env->GetFloatField(from_point_3D, point_y_field);
    from_point[2] =  env->GetFloatField(from_point_3D, point_z_field);

    // retrieving rs2_extrinsics from extrinsic object
    rs2_extrinsics extrinsics = extrinsic_jobject2rs(env, extrinsic);

    // preparing struct to be filled by API function
    float to_point[3] = {0.f, 0.f, 0.f};

    //api call
    rs2_transform_point_to_point(to_point, &extrinsics, from_point);

    // returning point into point_3D struct
    jclass to_point_3D_class = env->GetObjectClass(to_point_3D);
    jfieldID to_point_x_field = env->GetFieldID(to_point_3D_class, "mX", "F");
    jfieldID to_point_y_field = env->GetFieldID(to_point_3D_class, "mY", "F");
    jfieldID to_point_z_field = env->GetFieldID(to_point_3D_class, "mZ", "F");
    env->SetFloatField(to_point_3D, to_point_x_field, to_point[0]);
    env->SetFloatField(to_point_3D, to_point_y_field, to_point[1]);
    env->SetFloatField(to_point_3D, to_point_z_field, to_point[2]);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Utils_nGetFov(JNIEnv *env, jclass type,
                                                    jobject fov, jobject intrinsic) {
    // retrieving rs2_intrinsics from intrinsic object
    rs2_intrinsics intrinsics = intrinsic_jobject2rs(env, intrinsic);

    float fov_cpp[2] = {0.f, 0.f};
    rs2_fov(&intrinsics, fov_cpp);

    // returning fov into fov struct
    jclass fov_class = env->GetObjectClass(fov);
    jfieldID fov_x_field = env->GetFieldID(fov_class, "mFovX", "F");
    jfieldID fov_y_field = env->GetFieldID(fov_class, "mFovY", "F");
    env->SetFloatField(fov, fov_x_field, fov_cpp[0]);
    env->SetFloatField(fov, fov_y_field, fov_cpp[1]);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Utils_nProject2dPixelToDepthPixel(JNIEnv *env, jclass type,
                                                    jobject toPixel, jlong depthFrameHandle, jfloat depthScale,
                                                    jfloat depthMin, jfloat depthMax,
                                                    jobject depthIntrinsic, jobject otherIntrinsic,
                                                    jobject otherToDepth, jobject depthToOther,
                                                    jobject fromPixel) {


    // preparing struct to be filled by API function
    float to_pixel[2] = {0.f, 0.f};

    // preparing depth frame data
    rs2_frame* depthFrame = reinterpret_cast<rs2_frame*>(depthFrameHandle);
    rs2_error* e = nullptr;
    const uint16_t* depthFrameData = (const uint16_t*)rs2_get_frame_data(depthFrame, &e);
    // retrieving intrinsics objects
    rs2_intrinsics depth_intrinsics = intrinsic_jobject2rs(env, depthIntrinsic);
    rs2_intrinsics other_intrinsics = intrinsic_jobject2rs(env, otherIntrinsic);
    // retrieving extrinsics objects
    rs2_extrinsics other_to_depth_extrinsics = extrinsic_jobject2rs(env, otherToDepth);
    rs2_extrinsics depth_to_other_extrinsics = extrinsic_jobject2rs(env, depthToOther);
    // retrieving float[2] from fromPixel
    jclass from_pixel_class = env->GetObjectClass(fromPixel);
    jfieldID from_pixel_x_field = env->GetFieldID(from_pixel_class, "mX", "F");
    jfieldID from_pixel_y_field = env->GetFieldID(from_pixel_class, "mY", "F");
    float from_pixel[2];
    from_pixel[0] = env->GetFloatField(fromPixel, from_pixel_x_field);
    from_pixel[1] = env->GetFloatField(fromPixel, from_pixel_y_field);

    // api call
    rs2_project_color_pixel_to_depth_pixel(to_pixel, depthFrameData, depthScale, depthMin, depthMax,
            &depth_intrinsics, &other_intrinsics, &other_to_depth_extrinsics, &depth_to_other_extrinsics, from_pixel);

    // returning pixel into pixel_2D struct
    jclass to_pixel_class = env->GetObjectClass(toPixel);
    jfieldID to_pixel_x_field = env->GetFieldID(to_pixel_class, "mX", "F");
    jfieldID to_pixel_y_field = env->GetFieldID(to_pixel_class, "mY", "F");
    env->SetFloatField(toPixel, to_pixel_x_field, to_pixel[0]);
    env->SetFloatField(toPixel, to_pixel_y_field, to_pixel[1]);
}
