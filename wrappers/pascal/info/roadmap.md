# RoadMap

## Table of conversions

| Include                     | Pascal unit                   | Notes
|-----------------------------|-------------------------------|--------------|
| rs.h                        | rs.pas                        |finished    
| rsutil.h                    | rsutil.pas                    |finished            
| rs_advanced_mode.h          | rs_advanced_mode.pas          |finished             
| rs_advanced_mode_command.h  | rs_advanced_mode_command.pas  |finished
| rs_config.h                 | rs_config.pas                 |finished 
| rs_context.h                | rs_context.pas                |finished 
| rs_device.h                 | rs_device.pas                 |finished 
| rs_frame.h                  | rs_frame.pas                  |finished
| rs_internal.h               | rs_internal.pas               |finished
| rs_option.h                 | rs_option.pas                 |finished
| rs_pipeline.h               | rs_pipeline.pas               |finished 
| rs_processing.h             | rs_processing.pas             |finished
| rs_record_playback.h        | rs_record_playback.pas        |finished
| rs_sensor.h                 | rs_sensor.pas                 |finished 
| rs_types.h                  | rs_types.pas                  |finished 



***




## rs.pas

| Function/procedure         | Tested |
|----------------------------|--------|
|RS2_API_VERSION_STR:        |   OK   |
|RS2_API_FULL_VERSION_STR:   |   OK   |
|rs2_get_raw_data_size       |   NO   |
|rs2_delete_raw_data         |   NO   |
|rs2_get_raw_data            |   NO   |
|rs2_depth_frame_get_distance|   Ok   |



## rs_advanced_mode.pas

| Function/procedure                 | Tested |
|------------------------------------|--------|
|rs2_toggle_advanced_mode            |   NO   |
|rs2_is_enabled                      |   NO   |
|rs2_set_depth_control               |   NO   |
|rs2_get_depth_control               |   NO   |
|rs2_set_rsm                         |   NO   |
|rs2_get_rsm                         |   NO   |
|rs2_set_rau_support_vector_control  |   NO   |
|rs2_get_rau_support_vector_control  |   NO   |
|rs2_set_color_control               |   NO   |
|rs2_get_color_control               |   NO   |
|rs2_set_rau_thresholds_control      |   NO   |
|rs2_get_rau_thresholds_control      |   NO   |
|rs2_set_slo_color_thresholds_control|   NO   |
|rs2_get_slo_color_thresholds_control|   NO   |
|rs2_set_slo_penalty_control         |   NO   |
|rs2_get_slo_penalty_control         |   NO   |
|rs2_set_hdad                        |   NO   |
|rs2_get_hdad                        |   NO   |
|rs2_set_color_correction            |   NO   |
|rs2_get_color_correction            |   NO   |
|rs2_set_depth_table                 |   NO   |
|rs2_get_depth_table                 |   NO   |
|rs2_set_ae_control                  |   NO   |
|rs2_get_ae_control                  |   NO   |
|rs2_set_census                      |   NO   |
|rs2_get_census                      |   NO   |
|rs2_set_amp_factor                  |   NO   |
|rs2_get_amp_factor                  |   NO   |



## rs_config.pas

| Function/procedure                             | Tested |
|------------------------------------------------|--------|
|rs2_create_config                               |   OK   |
|rs2_delete_config                               |   OK   |
|rs2_config_enable_stream                        |   OK   |
|rs2_config_enable_all_stream                    |   NO   |
|rs2_config_enable_device                        |   NO   |
|rs2_config_enable_device_from_file              |   NO   |
|rs2_config_enable_device_from_file_repeat_option|   NO   |
|rs2_config_enable_record_to_file                |   NO   |
|rs2_config_disable_stream                       |   NO   |
|rs2_config_disable_indexed_stream               |   NO   |
|rs2_config_disable_all_streams                  |   NO   |
|rs2_config_resolve                              |   NO   |
|rs2_config_can_resolve                          |   NO   |



## rs_context.pas

| Function/procedure               | Tested |
|----------------------------------|--------|
|rs2_create_context                |   OK   |
|rs2_delete_context                |   OK   |
|rs2_set_devices_changed_callback  |   NO   |
|rs2_context_add_device            |   NO   |
|rs2_context_add_software_device   |   NO   |
|rs2_context_remove_device         |   NO   |
|query_devices                     |   NO   |
|rs2_context_unload_tracking_module|   NO   |
|rs2_query_devices                 |   OK   |
|rs2_query_devices_ex              |   NO   |
|rs2_create_device_hub             |   NO   |
|rs2_device_hub_wait_for_device    |   NO   |
|rs2_device_hub_is_device_connected|   NO   |



## rs_device.pas

| Function/procedure                     | Tested |
|----------------------------------------|--------|
|rs2_get_device_count                    |   OK   |
|rs2_delete_device_list                  |   OK   |
|rs2_device_list_contains                |   NO   |
|rs2_create_device                       |   OK   |
|rs2_delete_device                       |   OK   |
|rs2_get_device_info                     |   OK   |
|rs2_supports_device_info                |   NO   |
|rs2_hardware_reset                      |   NO   |
|rs2_build_debug_protocol_command        |   NO   |
|rs2_send_and_receive_raw_data           |   NO   |
|rs2_is_device_extendable_to             |   NO   |
|rs2_query_sensors                       |   NO   |
|rs2_loopback_enable                     |   NO   |
|rs2_loopback_disable                    |   NO   |
|rs2_loopback_is_enabled                 |   NO   |
|rs2_connect_tm2_controller              |   NO   |
|rs2_disconnect_tm2_controller           |   NO   |
|rs2_reset_to_factory_calibration        |   NO   |
|rs2_write_calibration                   |   NO   |
|rs2_update_firmware                     |   NO   |
|rs2_create_flash_backup                 |   NO   |
|rs2_check_firmware_compatibility        |   NO   |
|rs2_update_firmware_unsigned            |   NO   |
|rs2_enter_update_state                  |   NO   |
|rs2_run_on_chip_calibration             |   NO   |
|rs2_calibration_type_to_string          |   NO   |
|rs2_calibration_status_to_string        |   NO   |
|rs2_register_calibration_change_callback|   NO   |
|rs2_trigger_device_calibration          |   NO   |
|rs2_run_tare_calibration                |   NO   |
|rs2_get_calibration_table               |   NO   |
|rs2_set_calibration_table               |   NO   |
|rs2_serialize_json                      |   NO   |
|rs2_load_json                           |   NO   |
|rs2_run_focal_length_calibration        |   NO   |
|rs2_run_uv_map_calibration              |   NO   |
|rs2_calculate_target_z                  |   NO   |



## rs_frame.pas

| Function/procedure                | Tested |
|-----------------------------------|--------|
|rs2_timestamp_domain_to_string     |   NO   |
|rs2_calib_target_type_to_string    |   NO   |
|rs2_frame_metadata_to_string       |   NO   |
|rs2_frame_metadata_value_to_string |   NO   |
|rs2_extract_frame                  |   NO   |
|rs2_embedded_frames_count          |   OK   |
|rs2_release_frame                  |   NO   |
|rs2_get_frame_data_size            |   NO   |
|rs2_get_frame_data                 |   NO   |
|rs2_get_frame_number               |   NO   |
|rs2_get_frame_timestamp_domain     |   NO   |
|rs2_get_frame_timestamp            |   NO   |
|rs2_get_frame_metadata             |   NO   |
|rs2_supports_frame_metadata        |   NO   |
|rs2_get_frame_sensor               |   NO   |
|rs2_get_frame_width                |   NO   |
|rs2_get_frame_height               |   NO   |
|rs2_depth_frame_get_units          |   NO   |
|rs2_get_frame_stride_in_bytes      |   NO   |
|rs2_get_frame_bits_per_pixel       |   NO   |
|rs2_frame_add_ref                  |   NO   |
|rs2_keep_frame                     |   NO   |
|rs2_get_frame_vertices             |   NO   |
|rs2_export_to_ply                  |   NO   |
|rs2_get_frame_texture_coordinates  |   NO   |
|rs2_get_frame_points_count         |   NO   |
|rs2_get_frame_stream_profile       |   NO   |
|rs2_is_frame_extendable_to         |   OK   |
|rs2_allocate_synthetic_video_frame |   NO   |
|rs2_allocate_synthetic_motion_frame|   NO   |
|rs2_allocate_points                |   NO   |
|rs2_allocate_composite_frame       |   NO   |
|rs2_synthetic_frame_ready          |   NO   |
|rs2_pose_frame_get_pose_data       |   NO   |
|rs2_extract_target_dimensions      |   NO   |



## rs_internal.pas

| Function/procedure                         | Tested |
|--------------------------------------------|--------|
|rs2_create_recording_context                |   NO   |
|rs2_create_mock_context                     |   NO   |
|rs2_create_mock_context_versioned           |   NO   |
|rs2_create_software_device                  |   NO   |
|rs2_software_device_add_sensor              |   NO   |
|rs2_software_sensor_on_video_frame          |   NO   |
|rs2_software_sensor_on_motion_frame         |   NO   |
|rs2_software_sensor_on_pose_frame           |   NO   |
|rs2_software_sensor_on_notification         |   NO   |
|rs2_software_sensor_set_metadata            |   NO   |
|rs2_software_device_set_destruction_callback|   NO   |
|rs2_software_device_create_matcher          |   NO   |
|rs2_software_device_register_info           |   NO   |
|rs2_software_device_update_info             |   NO   |
|rs2_software_sensor_add_video_stream        |   NO   |
|rs2_software_sensor_add_video_stream_ex     |   NO   |
|rs2_software_sensor_add_motion_stream       |   NO   |
|rs2_software_sensor_add_motion_stream       |   NO   |
|rs2_software_sensor_add_pose_stream         |   NO   |
|rs2_software_sensor_add_pose_stream         |   NO   |
|rs2_software_sensor_add_read_only_option    |   NO   |
|rs2_software_sensor_update_read_only_option |   NO   |
|rs2_software_sensor_add_option              |   NO   |
|rs2_software_sensor_detach                  |   NO   |
|rs2_create_fw_log_message                   |   NO   |
|rs2_get_fw_log                              |   NO   |
|rs2_get_flash_log                           |   NO   |
|rs2_delete_fw_log_message                   |   NO   |
|rs2_fw_log_message_data                     |   NO   |
|rs2_fw_log_message_size                     |   NO   |
|rs2_fw_log_message_timestamp                |   NO   |
|rs2_fw_log_message_severity                 |   NO   |
|rs2_init_fw_log_parser                      |   NO   |
|rs2_create_fw_log_parsed_message            |   NO   |
|rs2_parse_firmware_log                      |   NO   |
|rs2_get_number_of_fw_logs                   |   NO   |
|rs2_get_fw_log_parsed_message               |   NO   |
|rs2_get_fw_log_parsed_file_name             |   NO   |
|rs2_get_fw_log_parsed_thread_name           |   NO   |
|rs2_get_fw_log_parsed_severity              |   NO   |
|rs2_get_fw_log_parsed_line                  |   NO   |
|rs2_get_fw_log_parsed_timestamp             |   NO   |
|rs2_get_fw_log_parsed_sequence_id           |   NO   |
|rs2_create_terminal_parser                  |   NO   |
|rs2_delete_terminal_parser                  |   NO   |
|rs2_terminal_parse_command                  |   NO   |
|pRS2_terminal_parser                        |   NO   |



## rs_option.pas

| Function/procedure              | Tested |
|---------------------------------|--------|
|rs2_option_to_string             |   NO   |
|rs2_sr300_visual_preset_to_string|   NO   |
|rs2_rs400_visual_preset_to_string|   NO   |
|rs2_l500_visual_preset_to_string |   NO   |
|rs2_sensor_mode_to_string        |   NO   |
|rs2_ambient_light_to_string      |   NO   |
|rs2_digital_gain                 |   NO   |
|rs2_host_perf_mode_to_string     |   NO   |
|rs2_is_option_read_only          |   NO   |
|rs2_get_option                   |   NO   |
|rs2_set_option                   |   NO   |
|rs2_options_list                 |   NO   |
|rs2_get_options_list_size        |   NO   |
|rs2_get_option_name              |   NO   |
|rs2_get_option_from_list         |   NO   |
|rs2_delete_options_list          |   NO   |
|rs2_supports_option              |   NO   |
|rs2_get_option_range             |   NO   |
|rs2_get_option_description       |   NO   |
|rs2_get_option_value_description |   NO   |



## rs_pipeline.pas

| Function/procedure                        | Tested |
|-------------------------------------------|--------|
|rs2_create_pipeline                        |   OK   |
|rs2_pipeline_stop                          |   NO   |
|rs2_delete_pipeline                        |   OK   |
|rs2_pipeline_start_with_config             |   OK   |
|rs2_pipeline_wait_for_frames               |   OK   |
|rs2_delete_pipeline_profile                |   NO   |
|rs2_pipeline_poll_for_frames               |   NO   |
|rs2_pipeline_try_wait_for_frames           |   NO   |
|rs2_pipeline_start                         |   NO   |
|rs2_pipeline_start_with_callback           |   NO   |
|rs2_pipeline_start_with_config_and_callback|   NO   |
|rs2_pipeline_get_active_profile            |   NO   |
|rs2_pipeline_profile_get_device            |   NO   |
|rs2_pipeline_profile_get_streams           |   NO   |



## rs_processing.pas

| Function/procedure                        | Tested |
|-------------------------------------------|--------|
|rs2_create_colorizer                       |   NO   |
|rs2_create_sync_processing_block           |   NO   |
|rs2_create_pointcloud                      |   NO   |
|rs2_create_yuy_decoder                     |   NO   |
|rs2_create_y411_decoder                    |   NO   |
|rs2_create_threshold                       |   NO   |
|rs2_create_units_transform                 |   NO   |
|rs2_create_processing_block                |   NO   |
|rs2_create_processing_block_fptr           |   NO   |
|rs2_processing_block_register_simple_option|   NO   |
|rs2_start_processing                       |   NO   |
|rs2_start_processing_fptr                  |   NO   |
|rs2_start_processing_queue                 |   NO   |
|rs2_process_frame                          |   NO   |
|rs2_delete_processing_block                |   NO   |
|rs2_create_frame_queue                     |   NO   |
|rs2_delete_frame_queue                     |   NO   |
|rs2_frame_queue_size                       |   NO   |
|rs2_wait_for_frame                         |   NO   |
|rs2_poll_for_frame                         |   NO   |
|rs2_try_wait_for_frame                     |   NO   |
|rs2_enqueue_frame                          |   NO   |
|rs2_create_align                           |   NO   |
|rs2_create_decimation_filter_block         |   NO   |
|rs2_create_temporal_filter_block           |   NO   |
|rs2_create_spatial_filter_block            |   NO   |
|rs2_create_disparity_transform_block       |   NO   |
|rs2_create_hole_filling_filter_block       |   NO   |
|rs2_create_rates_printer_block             |   NO   |
|rs2_create_zero_order_invalidation_block   |   NO   |
|rs2_create_huffman_depth_decompress_block  |   NO   |
|rs2_create_hdr_merge_processing_block      |   NO   |
|rs2_create_sequence_id_filter              |   NO   |
|rs2_get_processing_block_info              |   NO   |
|rs2_supports_processing_block_info         |   NO   |
|rs2_is_processing_block_extendable_to      |   NO   |



## rs_record_playback.pas

| Function/procedure                            | Tested |
|-----------------------------------------------|--------|
|rs2_playback_status_to_string                  |   NO   |
|rs2_create_record_device                       |   NO   |
|rs2_create_record_device_ex                    |   NO   |
|rs2_record_device_pause                        |   NO   |
|rs2_record_device_resume                       |   NO   |
|rs2_record_device_filename                     |   NO   |
|rs2_create_playback_device                     |   NO   |
|rs2_playback_device_get_file_path              |   NO   |
|rs2_playback_get_duration                      |   NO   |
|rs2_playback_seek                              |   NO   |
|rs2_playback_get_position                      |   NO   |
|rs2_playback_device_resume                     |   NO   |
|rs2_playback_device_pause                      |   NO   |
|rs2_playback_device_set_real_time              |   NO   |
|rs2_playback_device_is_real_time               |   NO   |
|rs2_playback_device_set_status_changed_callback|   NO   |
|rs2_playback_device_get_current_status         |   NO   |
|rs2_playback_device_set_playback_speed         |   NO   |
|rs2_playback_device_stop                       |   NO   |



## rs_sensor.pas

| Function/procedure                        | Tested |
|-------------------------------------------|--------|
|rs2_camera_info_to_string                  |   NO   |
|rs2_stream_to_string                       |   NO   |
|rs2_format_to_string                       |   NO   |
|rs2_delete_sensor_list                     |   NO   |
|rs2_get_sensors_count                      |   NO   |
|rs2_delete_sensor                          |   NO   |
|rs2_create_sensor                          |   NO   |
|rs2_create_device_from_sensor              |   NO   |
|rs2_get_sensor_info                        |   NO   |
|rs2_supports_sensor_info                   |   NO   |
|rs2_is_sensor_extendable_to                |   NO   |
|rs2_get_depth_scale                        |   NO   |
|rs2_depth_stereo_frame_get_baseline        |   NO   |
|rs2_get_stereo_baseline                    |   NO   |
|rs2_set_region_of_interest                 |   NO   |
|rs2_get_region_of_interest                 |   NO   |
|rs2_open                                   |   NO   |
|rs2_open_multiple                          |   NO   |
|rs2_close                                  |   NO   |
|rs2_start                                  |   NO   |
|rs2_start_queue                            |   NO   |
|rs2_stop                                   |   NO   |
|rs2_set_notifications_callback             |   NO   |
|rs2_get_notification_description           |   NO   |
|rs2_get_notification_timestamp             |   NO   |
|rs2_get_notification_severity              |   NO   |
|rs2_get_notification_category              |   NO   |
|rs2_get_notification_serialized_data       |   NO   |
|rs2_get_stream_profiles                    |   NO   |
|rs2_get_debug_stream_profiles              |   NO   |
|rs2_get_active_streams                     |   NO   |
|rs2_get_stream_profile                     |   NO   |
|rs2_get_stream_profile_data                |   NO   |
|rs2_clone_stream_profile                   |   NO   |
|rs2_clone_video_stream_profile             |   NO   |
|rs2_delete_stream_profile                  |   NO   |
|rs2_stream_profile_is                      |   NO   |
|rs2_get_video_stream_resolution            |   NO   |
|rs2_get_motion_intrinsics                  |   NO   |
|rs2_is_stream_profile_default              |   NO   |
|rs2_get_stream_profiles_count              |   NO   |
|rs2_delete_stream_profile_list             |   NO   |
|rs2_get_extrinsics                         |   NO   |
|rs2_register_extrinsics                    |   NO   |
|rs2_override_extrinsics                    |   NO   |
|rs2_get_video_stream_intrinsics            |   NO   |
|rs2_get_recommended_processing_blocks      |   NO   |
|rs2_get_processing_block                   |   NO   |
|rs2_get_recommended_processing_blocks_count|   NO   |
|rs2_delete_recommended_processing_blocks   |   NO   |
|rs2_import_localization_map                |   NO   |
|rs2_export_localization_map                |   NO   |
|rs2_set_static_node                        |   NO   |
|rs2_get_static_node                        |   NO   |
|rs2_remove_static_node                     |   NO   |
|rs2_load_wheel_odometry_config             |   NO   |
|rs2_send_wheel_odometry                    |   NO   |
|rs2_set_intrinsics                         |   NO   |
|rs2_override_intrinsics                    |   NO   |
|rs2_get_dsm_params                         |   NO   |
|rs2_override_dsm_params                    |   NO   |
|rs2_reset_sensor_calibration               |   NO   |
|rs2_set_motion_device_intrinsics           |   NO   |
|rs2_get_max_usable_depth_range             |   NO   |



## rs_types.pas

| Function/procedure                | Tested |
|-----------------------------------|--------|
|rs2_notification_category_to_string|   NO   |
|rs2_extension_type_to_string       |   NO   |
|rs2_extension_to_string            |   NO   |
|rs2_format_to_string               |   NO   |
|rs2_distortion_to_string           |   NO   |
|rs2_stream_to_string               |   NO   |
|rs2_matchers_to_string             |   NO   |
|rs2_log_severity_to_string         |   NO   |
|rs2_get_error_message              |   NO   |
|rs2_get_failed_function            |   OK   |
|rs2_get_failed_args                |   OK   |
|rs2_free_error                     |   OK   |
|rs2_exception_type_to_string       |   NO   |
|rs2_get_librealsense_exception_type|   NO   |