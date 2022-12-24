unit rs_advanced_mode;

{$mode ObjFPC}{$H+}

interface

uses rs_advanced_mode_command,
  rs_types;

{
  Enable/Disable Advanced-Mode
}
procedure rs2_toggle_advanced_mode(dev: pRS2_device; enable: integer;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Check if Advanced-Mode is enabled
}
procedure rs2_is_enabled(dev: pRS2_device; Enabled: PInteger; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
  Sets new values for Depth Control Group, returns 0 if success
}
procedure rs2_set_depth_control(dev: pRS2_device; const group: pSTDepthControlGroup;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
 Gets new values for Depth Control Group, returns 0 if success */
}
procedure rs2_get_depth_control(dev: pRS2_device; group: pSTDepthControlGroup;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
 Sets new values for RSM Group, returns 0 if success
}
procedure rs2_set_rsm(dev: pRS2_device; const group: pSTRsm; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
 Gets new values for RSM Group, returns 0 if success
}
procedure rs2_get_rsm(dev: pRS2_device; group: pSTRsm; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
/* Sets new values for STRauSupportVectorControl, returns 0 if success */
void rs2_set_rau_support_vector_control(rs2_device* dev, const STRauSupportVectorControl* group, rs2_error** error);
}
procedure rs2_set_rau_support_vector_control(dev: pRS2_device;
  const group: pSTRauSupportVectorControl; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
 Gets new values for STRauSupportVectorControl, returns 0 if success
}
procedure rs2_get_rau_support_vector_control(dev: pRS2_device;
  group: pSTRauSupportVectorControl; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Sets new values for STColorControl, returns 0 if success */
}
procedure rs2_set_color_control(dev: pRS2_device; const group: TSTColorControl;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
 Gets new values for STColorControl, returns 0 if success
}
procedure rs2_get_color_control(dev: pRS2_device; group: TSTColorControl;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Sets new values for STRauColorThresholdsControl, returns 0 if success
}
procedure rs2_set_rau_thresholds_control(dev: pRS2_device;
  const group: TSTRauColorThresholdsControl; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
 Gets new values for STRauColorThresholdsControl, returns 0 if success
}
procedure rs2_get_rau_thresholds_control(dev: pRS2_device;
  group: TSTRauColorThresholdsControl; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
  Sets new values for STSloColorThresholdsControl, returns 0 if success
}
procedure rs2_set_slo_color_thresholds_control(dev: pRS2_device;
  const group: pSTSloColorThresholdsControl; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
  Gets new values for STRauColorThresholdsControl, returns 0 if success
}
procedure rs2_get_slo_color_thresholds_control(dev: pRS2_device;
  group: pSTSloColorThresholdsControl; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
 Sets new values for STSloPenaltyControl, returns 0 if success
}
procedure rs2_set_slo_penalty_control(dev: pRS2_device;
  const group: pSTSloColorThresholdsControl; error: pRS2_error); cdecl; external;
{
 Gets new values for STSloPenaltyControl, returns 0 if success
}
procedure rs2_get_slo_penalty_control(dev: pRS2_device;
  group: pSTSloColorThresholdsControl; error: pRS2_error); cdecl; external;
{
 Sets new values for STHdad, returns 0 if success
}
procedure rs2_set_hdad(dev: pRS2_device; const group: pSTHdad; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
/* Gets new values for STHdad, returns 0 if success */
}
procedure rs2_get_hdad(dev: pRS2_device; group: pSTHdad; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
 Sets new values for STColorCorrection, returns 0 if success
}
procedure rs2_set_color_correction(dev: pRS2_device; const group: pSTColorCorrection;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
 Gets new values for STColorCorrection, returns 0 if success
}
procedure rs2_get_color_correction(dev: pRS2_device; group: pSTColorCorrection;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
/* Sets new values for STDepthTableControl, returns 0 if success */
void rs2_set_depth_table(rs2_device* dev, const  STDepthTableControl* group, rs2_error** error);
}
procedure rs2_set_depth_table(dev: pRS2_device; const group: pSTDepthTableControl;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
/* Gets new values for STDepthTableControl, returns 0 if success */
void rs2_get_depth_table(rs2_device* dev, STDepthTableControl* group, int mode, rs2_error** error);
}
procedure rs2_get_depth_table(dev: pRS2_device; group: pSTDepthTableControl;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
void rs2_set_ae_control(rs2_device* dev, const  STAEControl* group, rs2_error** error);
}
procedure rs2_set_ae_control(dev: pRS2_device; const group: pSTAEControl;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
procedure rs2_get_ae_control(dev: pRS2_device; group: pSTAEControl; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
procedure rs2_set_census(dev: pRS2_device; const group: pSTCensusRadius;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
procedure rs2_get_census(dev: pRS2_device; group: pSTCensusRadius; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
procedure rs2_set_amp_factor(dev: pRS2_device; const group: pSTAFactor;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
procedure rs2_get_amp_factor(dev: pRS2_device; group: pSTAFactor; error: pRS2_error);
  cdecl; external REALSENSE_DLL;

implementation

end.
