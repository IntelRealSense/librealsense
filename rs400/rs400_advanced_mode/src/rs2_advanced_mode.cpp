/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#include <thread>
#include <sstream>

#include "../../../src/types.h"
#include <librealsense/rs2_advanced_mode.h>
#include "core/advanced_mode.h"
#include "api.h"

namespace librealsense
{
    RS2_ENUM_HELPERS(rs2_rs400_visual_preset, RS400_VISUAL_PRESET)

    const char* get_string(rs2_rs400_visual_preset value)
    {
        #define CASE(X) case RS2_RS400_VISUAL_PRESET_##X: return #X;
        switch (value)
        {
        CASE(GENERIC_DEPTH)
        CASE(GENERIC_ACCURATE_DEPTH)
        CASE(GENERIC_DENSE_DEPTH)
        CASE(GENERIC_SUPER_DENSE_DEPTH)
        CASE(FLOOR_LOW)
        CASE(3D_BODY_SCAN)
        CASE(INDOOR)
        CASE(OUTDOOR)
        CASE(HAND)
        CASE(SHORT_RANGE)
        CASE(BOX)
        default: assert(!is_valid(value)); return UNKNOWN;
        }
        #undef CASE
    }
}

const char* rs2_advanced_mode_preset_to_string(rs2_rs400_visual_preset preset){ return get_string(preset); }

void rs2_toggle_advanced_mode(rs2_device* dev, int enable, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->toggle_advanced_mode(enable);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, enable)

void rs2_is_enabled(rs2_device* dev, int* enabled, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(enabled);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    *enabled = advanced_mode->is_enabled();
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, enabled)

void rs2_set_depth_control(rs2_device* dev, STDepthControlGroup* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_depth_control_group(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_depth_control(rs2_device* dev, STDepthControlGroup* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_depth_control_group(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_rsm(rs2_device* dev, STRsm* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_rsm(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_rsm(rs2_device* dev, STRsm* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_rsm(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_rau_support_vector_control(rs2_device* dev, STRauSupportVectorControl* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_rau_support_vector_control(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_rau_support_vector_control(rs2_device* dev, STRauSupportVectorControl* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_rau_support_vector_control(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_color_control(rs2_device* dev, STColorControl* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_color_control(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_color_control(rs2_device* dev, STColorControl* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_color_control(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_rau_thresholds_control(rs2_device* dev, STRauColorThresholdsControl* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_rau_color_thresholds_control(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_rau_thresholds_control(rs2_device* dev, STRauColorThresholdsControl* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_rau_color_thresholds_control(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_slo_color_thresholds_control(rs2_device* dev, STSloColorThresholdsControl* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_slo_color_thresholds_control(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_slo_color_thresholds_control(rs2_device* dev, STSloColorThresholdsControl* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_slo_color_thresholds_control(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_slo_penalty_control(rs2_device* dev, STSloPenaltyControl* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_slo_penalty_control(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_slo_penalty_control(rs2_device* dev, STSloPenaltyControl* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_slo_penalty_control(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_hdad(rs2_device* dev, STHdad* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_hdad(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_hdad(rs2_device* dev, STHdad* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_hdad(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_color_correction(rs2_device* dev, STColorCorrection* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_color_correction(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_color_correction(rs2_device* dev, STColorCorrection* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_color_correction(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_depth_table(rs2_device* dev, STDepthTableControl* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_depth_table_control(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_depth_table(rs2_device* dev, STDepthTableControl* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_depth_table_control(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_ae_control(rs2_device* dev, STAEControl* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_ae_control(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_ae_control(rs2_device* dev, STAEControl* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_ae_control(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)

void rs2_set_census(rs2_device* dev, STCensusRadius* group, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->set_census_radius(*group);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group)

void rs2_get_census(rs2_device* dev, STCensusRadius* group, int mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_NOT_NULL(group);
    auto advanced_mode = VALIDATE_INTERFACE(dev->device, librealsense::ds5_advanced_mode_interface);
    advanced_mode->get_census_radius(group, mode);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, group, mode)
