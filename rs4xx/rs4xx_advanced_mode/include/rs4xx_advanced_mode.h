/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#ifndef RS4XX_ADVANCED_MODE_H
#define RS4XX_ADVANCED_MODE_H

#include <stdint.h>

#define RS4XX_ADVANCED_MODE_HPP
#include "advanced_mode_command.h"
#undef RS4XX_ADVANCED_MODE_HPP

#include <librealsense/rs2.h>

#ifdef __cplusplus
extern "C" {
#endif

int rs2_go_to_advanced_mode(rs2_device* dev);

int rs2_is_enabled(rs2_device* dev, int* enabled);

/* Sets new values for Depth Control Group, returns 0 if success */
int rs2_set_depth_control(rs2_device* dev, STDepthControlGroup* group, int mode);

/* Gets new values for Depth Control Group, returns 0 if success */
int rs2_get_depth_control(rs2_device* dev, STDepthControlGroup* group, int mode);

/* Sets new values for RSM Group, returns 0 if success */
int rs2_set_rsm(rs2_device* dev, STRsm* group, int mode);

/* Gets new values for RSM Group, returns 0 if success */
int rs2_get_rsm(rs2_device* dev, STRsm* group, int mode);

/* Sets new values for STRauSupportVectorControl, returns 0 if success */
int rs2_set_rau_support_vector_control(rs2_device* dev, STRauSupportVectorControl* group, int mode);

/* Gets new values for STRauSupportVectorControl, returns 0 if success */
int rs2_get_rau_support_vector_control(rs2_device* dev, STRauSupportVectorControl* group, int mode);

/* Sets new values for STColorControl, returns 0 if success */
int rs2_set_color_control(rs2_device* dev, STColorControl* group, int mode);

/* Gets new values for STColorControl, returns 0 if success */
int rs2_get_color_control(rs2_device* dev, STColorControl* group, int mode);

/* Sets new values for STRauColorThresholdsControl, returns 0 if success */
int rs2_set_rau_thresholds_control(rs2_device* dev, STRauColorThresholdsControl* group, int mode);

/* Gets new values for STRauColorThresholdsControl, returns 0 if success */
int rs2_get_rau_thresholds_control(rs2_device* dev, STRauColorThresholdsControl* group, int mode);

/* Sets new values for STSloColorThresholdsControl, returns 0 if success */
int rs2_set_slo_color_thresholds_control(rs2_device* dev, STSloColorThresholdsControl* group, int mode);

/* Gets new values for STRauColorThresholdsControl, returns 0 if success */
int rs2_get_slo_color_thresholds_control(rs2_device* dev, STSloColorThresholdsControl* group, int mode);

/* Sets new values for STSloPenaltyControl, returns 0 if success */
int rs2_set_slo_penalty_control(rs2_device* dev, STSloPenaltyControl* group, int mode);

/* Gets new values for STSloPenaltyControl, returns 0 if success */
int rs2_get_slo_penalty_control(rs2_device* dev, STSloPenaltyControl* group, int mode);

/* Sets new values for STHdad, returns 0 if success */
int rs2_set_hdad(rs2_device* dev, STHdad* group, int mode);

/* Gets new values for STHdad, returns 0 if success */
int rs2_get_hdad(rs2_device* dev, STHdad* group, int mode);

/* Sets new values for STColorCorrection, returns 0 if success */
int rs2_set_color_correction(rs2_device* dev, STColorCorrection* group, int mode);

/* Gets new values for STColorCorrection, returns 0 if success */
int rs2_get_color_correction(rs2_device* dev, STColorCorrection* group, int mode);

/* Sets new values for STDepthTableControl, returns 0 if success */
int rs2_set_depth_table(rs2_device* dev, STDepthTableControl* group, int mode);

/* Gets new values for STDepthTableControl, returns 0 if success */
int rs2_get_depth_table(rs2_device* dev, STDepthTableControl* group, int mode);

int rs2_set_ae_control(rs2_device* dev, STAEControl* group, int mode);

int rs2_get_ae_control(rs2_device* dev, STAEControl* group, int mode);

int rs2_set_census(rs2_device* dev, STCensusRadius* group, int mode);

int rs2_get_census(rs2_device* dev, STCensusRadius* group, int mode);

#ifdef __cplusplus
}
#endif
#endif
