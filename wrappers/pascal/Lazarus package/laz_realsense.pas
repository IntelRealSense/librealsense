{ This file was automatically created by Lazarus. Do not edit!
  This source is only used to compile and install the package.
 }

unit laz_realsense;

{$warn 5023 off : no warning about unused units}
interface

uses
  rs, rs_advanced_mode, rs_advanced_mode_command, rs_config, rs_context, 
  rs_device, rs_frame, rs_internal, rs_option, rs_pipeline, rs_processing, 
  rs_record_playback, rs_sensor, rs_types, LazarusPackageIntf;

implementation

procedure Register;
begin
end;

initialization
  RegisterPackage('laz_realsense', @Register);
end.
