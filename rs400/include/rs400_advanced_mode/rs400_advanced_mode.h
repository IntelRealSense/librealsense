#ifndef R400_ADVANCED_MODE_H
#define R400_ADVANCED_MODE_H

#include <stdint.h>

#define R400_ADVANCED_MODE_HPP
#include "AdvancedModeCommand.h"
#undef R400_ADVANCED_MODE_HPP

#include <librealsense/rs2.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sets new values for Depth Control Group, returns 0 if success */
int set_depth_control(rs2_device* dev, STDepthControlGroup* group);

/* Gets new values for Depth Control Group, returns 0 if success */
int get_depth_control(rs2_device* dev, STDepthControlGroup* group);

/* Sets new values for RSM Group, returns 0 if success */
int set_rsm(rs2_device* dev, STRsm* group);

/* Gets new values for RSM Group, returns 0 if success */
int get_rsm(rs2_device* dev, STRsm* group);

/* Sets new values for STRauSupportVectorControl, returns 0 if success */
int set_rau_support_vector_control(rs2_device* dev, STRauSupportVectorControl* group);

/* Gets new values for STRauSupportVectorControl, returns 0 if success */
int get_rau_support_vector_control(rs2_device* dev, STRauSupportVectorControl* group);

/* Sets new values for STColorControl, returns 0 if success */
int set_color_control(rs2_device* dev, STColorControl* group);

/* Gets new values for STColorControl, returns 0 if success */
int get_color_control(rs2_device* dev, STColorControl* group);

/* Sets new values for STRauColorThresholdsControl, returns 0 if success */
int set_rau_thresholds_control(rs2_device* dev, STRauColorThresholdsControl* group);

/* Gets new values for STRauColorThresholdsControl, returns 0 if success */
int get_rau_thresholds_control(rs2_device* dev, STRauColorThresholdsControl* group);

/* Sets new values for STSloColorThresholdsControl, returns 0 if success */
int set_slo_color_thresholds_control(rs2_device* dev, STSloColorThresholdsControl* group);

/* Gets new values for STRauColorThresholdsControl, returns 0 if success */
int get_slo_color_thresholds_control(rs2_device* dev, STSloColorThresholdsControl* group);

/* Sets new values for STSloPenaltyControl, returns 0 if success */
int set_slo_penalty_control(rs2_device* dev, STSloPenaltyControl* group);

/* Gets new values for STSloPenaltyControl, returns 0 if success */
int get_slo_penalty_control(rs2_device* dev, STSloPenaltyControl* group);

/* Sets new values for STHdad, returns 0 if success */
int set_hdad(rs2_device* dev, STHdad* group);

/* Gets new values for STHdad, returns 0 if success */
int get_hdad(rs2_device* dev, STHdad* group);

/* Sets new values for STColorCorrection, returns 0 if success */
int set_color_correction(rs2_device* dev, STColorCorrection* group);

/* Gets new values for STColorCorrection, returns 0 if success */
int get_color_correction(rs2_device* dev, STColorCorrection* group);

/* Sets new values for STDepthTableControl, returns 0 if success */
int set_depth_table(rs2_device* dev, STDepthTableControl* group);

/* Gets new values for STDepthTableControl, returns 0 if success */
int get_get_depth_table(rs2_device* dev, STDepthTableControl* group);

int set_ae_control(rs2_device* dev, STAEControl* group);

int get_ae_control(rs2_device* dev, STAEControl* group);

int set_census(rs2_device* dev, STCensusRadius* group);

int get_census(rs2_device* dev, STCensusRadius* group);

#ifdef __cplusplus
}
#endif
#endif 
