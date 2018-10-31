﻿using System;
using System.Runtime.InteropServices;
using System.Security;

namespace Intel.RealSense
{
    //[System.Security.SuppressUnmanagedCodeSecurity]
    internal static class NativeMethods
    {
        const string dllName = "realsense2";

        [DllImport("msvcrt.dll", EntryPoint = "memcpy", CallingConvention = CallingConvention.Cdecl, SetLastError = false)]
        internal static extern IntPtr memcpy(IntPtr dest, IntPtr src, int count);

        #region rs_record_playback
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_record_device(IntPtr device, [MarshalAs(UnmanagedType.LPStr)] string file, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_record_device_pause(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_record_device_resume(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_record_device_filename(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_playback_device([MarshalAs(UnmanagedType.LPStr)] string file, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_playback_device_get_file_path(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern ulong rs2_playback_get_duration(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_playback_seek(IntPtr device, long time, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern ulong rs2_playback_get_position(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_playback_device_resume(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_playback_device_pause(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_playback_device_set_real_time(IntPtr device, int real_time, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_playback_device_is_real_time(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_playback_device_set_status_changed_callback(IntPtr device, IntPtr callback, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern PlaybackStatus rs2_playback_device_get_current_status(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_playback_device_set_playback_speed(IntPtr device, float speed, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_playback_device_stop(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        #endregion
        #region rs_processing
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_colorizer([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_sync_processing_block([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_pointcloud([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_processing_block_fptr([MarshalAs(UnmanagedType.FunctionPtr)] frame_processor_callback on_frame, IntPtr user, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_start_processing_fptr(IntPtr block, [MarshalAs(UnmanagedType.FunctionPtr)] frame_callback on_frame, IntPtr user, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_start_processing_queue(IntPtr block, IntPtr queue, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_process_frame(IntPtr block, IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_processing_block(IntPtr block);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_frame_queue(int capacity, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_frame_queue(IntPtr queue);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_wait_for_frame(IntPtr queue, uint timeout_ms,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_poll_for_frame(IntPtr queue, out IntPtr frame,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_enqueue_frame(IntPtr frame, IntPtr queue);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_align(Stream align_to, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_decimation_filter_block([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_temporal_filter_block([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_spatial_filter_block([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_disparity_transform_block(byte transform_to_disparity, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_hole_filling_filter_block([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        #endregion
        #region rs_option
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_is_option_read_only(IntPtr options, Option option, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern float rs2_get_option(IntPtr options, Option option, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_option(IntPtr options, Option option, float value, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_supports_option(IntPtr options, Option option, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_option_range(IntPtr sensor, Option option, out float min, out float max, out float step, out float def, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_option_description(IntPtr options, Option option, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_option_value_description(IntPtr options, Option option, float value, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        #endregion
        #region rs_frame
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern long rs2_get_frame_metadata(IntPtr frame, FrameMetadataValue frame_metadata, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_supports_frame_metadata(IntPtr frame, FrameMetadataValue frame_metadata, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern TimestampDomain rs2_get_frame_timestamp_domain(IntPtr frameset, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern double rs2_get_frame_timestamp(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern ulong rs2_get_frame_number(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_frame_data(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_frame_width(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_frame_height(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_frame_stride_in_bytes(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_frame_bits_per_pixel(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_frame_add_ref(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_release_frame(IntPtr frame);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_keep_frame(IntPtr frame);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_frame_apply_filter(IntPtr frame, IntPtr block, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_frame_vertices(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_frame_texture_coordinates(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_frame_points_count(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_frame_stream_profile(IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_is_frame_extendable_to(IntPtr frame, Extension extension_type, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_allocate_synthetic_video_frame(IntPtr source, IntPtr new_stream, IntPtr original, int new_bpp, int new_width, int new_height, int new_stride, Extension frame_type, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_allocate_composite_frame(IntPtr source, [In]IntPtr[] frames, int count, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_allocate_composite_frame(IntPtr source, [In]IntPtr frames, int count, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_extract_frame(IntPtr composite, int index, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_embedded_frames_count(IntPtr composite, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_synthetic_frame_ready(IntPtr source, IntPtr frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        #endregion
        #region rs_sensor
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_sensor_list(IntPtr info_list);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_sensors_count(IntPtr info_list, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_sensor(IntPtr sensor);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_sensor(IntPtr list, int index, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_device_from_sensor(IntPtr sensor, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_sensor_info(IntPtr sensor, CameraInfo info, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_supports_sensor_info(IntPtr sensor, CameraInfo info, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_is_sensor_extendable_to(IntPtr sensor, Extension extension, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern float rs2_get_depth_scale(IntPtr sensor, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_region_of_interest(IntPtr sensor, int min_x, int min_y, int max_x, int max_y, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_region_of_interest(IntPtr sensor, out int min_x, out int min_y, out int max_x, out int max_y, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_open(IntPtr device, IntPtr profile, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_open_multiple(IntPtr device, [In]IntPtr[] profiles, int count, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_close(IntPtr sensor, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_start(IntPtr sensor, [MarshalAs(UnmanagedType.FunctionPtr)] frame_callback on_frame, IntPtr user, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_start_queue(IntPtr sensor, IntPtr queue, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_stop(IntPtr sensor, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_notifications_callback(IntPtr sensor, [MarshalAs(UnmanagedType.FunctionPtr)] frame_callback on_notification, IntPtr user, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_notification_description(IntPtr notification, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern double rs2_get_notification_timestamp(IntPtr notification, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern LogSeverity rs2_get_notification_severity(IntPtr notification, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern NotificationCategory rs2_get_notification_category(IntPtr notification, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_stream_profiles(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_stream_profile(IntPtr list, int index, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_stream_profile_data(IntPtr mode, out Stream stream, out Format format, out int index, out int unique_id, out int framerate, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_stream_profile_data(IntPtr mode, Stream stream, int index, Format format, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_clone_stream_profile(IntPtr mode, Stream stream, int index, Format format, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_stream_profile(IntPtr mode);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_stream_profile_is(IntPtr mode, Extension type, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_video_stream_resolution(IntPtr from, out int width, out int height, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_is_stream_profile_default(IntPtr profile, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_stream_profiles_count(IntPtr list, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_stream_profiles_list(IntPtr list);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_extrinsics(IntPtr from, IntPtr to, out Extrinsics extrin, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_video_stream_intrinsics(IntPtr from, out Intrinsics intrinsics, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        #endregion
        #region rs_device
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_device_count(IntPtr info_list, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_device_list(IntPtr info_list);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_device_list_contains(IntPtr info_list, IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_device(IntPtr info_list, int index, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_device(IntPtr device);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_device_info(IntPtr device, CameraInfo info, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_supports_device_info(IntPtr device, CameraInfo info, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_hardware_reset(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_send_and_receive_raw_data(IntPtr device, IntPtr raw_data_to_send, uint size_of_raw_data_to_send, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_motion_intrinsics(IntPtr device, Stream stream, IntPtr intrinsics, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_is_device_extendable_to(IntPtr device, Extension extension, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_query_sensors(IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        #endregion
        #region rs_context
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_context(int api_version, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_context(IntPtr context);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_context_add_device(IntPtr ctx, [MarshalAs(UnmanagedType.LPStr)] string file, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_context_remove_device(IntPtr ctx, [MarshalAs(UnmanagedType.LPStr)] string file, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_query_devices(IntPtr context, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_query_devices_ex(IntPtr context, int mask, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_device_hub(IntPtr context, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_device_hub(IntPtr hub);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_device_hub_wait_for_device(IntPtr ctx, IntPtr hub, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_device_hub_is_device_connected(IntPtr hub, IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_devices_changed_callback(IntPtr ctx, rs2_devices_changed_callback callback, IntPtr user_data, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        #endregion
        #region rs_types
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern ExceptionType rs2_get_librealsense_exception_type([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        #endregion
        #region rs
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_raw_data_size(IntPtr buffer, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_raw_data(IntPtr buffer);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_raw_data(IntPtr buffer, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_api_version([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_log_to_console(LogSeverity min_severity, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_log_to_file(LogSeverity min_severity, [MarshalAs(UnmanagedType.LPStr)] string file_path, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_log(LogSeverity severity, [MarshalAs(UnmanagedType.LPStr)] string message, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern float rs2_depth_frame_get_distance(IntPtr frame_ref, int x, int y, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern double rs2_get_time([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        #endregion
        #region rs_advanced_mode
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_toggle_advanced_mode(IntPtr dev, int enable, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_is_enabled(IntPtr dev, out int enabled, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_depth_control(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_depth_control(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_rsm(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_rsm(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_rau_support_vector_control(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_rau_support_vector_control(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_color_control(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_color_control(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_rau_thresholds_control(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_rau_thresholds_control(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_slo_color_thresholds_control(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_slo_color_thresholds_control(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_slo_penalty_control(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_slo_penalty_control(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_hdad(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_hdad(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_color_correction(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_color_correction(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_depth_table(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_depth_table(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_ae_control(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_ae_control(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_set_census(IntPtr dev, IntPtr group, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_get_census(IntPtr dev, IntPtr group, int mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_load_json(IntPtr dev, [MarshalAs(UnmanagedType.LPStr)] string json_content, uint content_size, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_serialize_json(IntPtr dev, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        #endregion
        #region rs_internal
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_recording_context(int api_version, [MarshalAs(UnmanagedType.LPStr)] string filename, [MarshalAs(UnmanagedType.LPStr)] string section, RecordingMode mode, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_mock_context(int api_version, [MarshalAs(UnmanagedType.LPStr)] string filename, [MarshalAs(UnmanagedType.LPStr)] string section, [MarshalAs(UnmanagedType.LPStr)] string min_api_version, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_software_device([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_software_device_add_sensor(IntPtr dev, [MarshalAs(UnmanagedType.LPStr)] string sensor_name, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_software_sensor_on_video_frame(IntPtr sensor, SoftwareVideoFrame frame, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_software_sensor_set_metadata(IntPtr sensor, long value, FrameMetadataValue type, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_software_device_create_matcher(IntPtr dev, Matchers matcher, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_software_sensor_add_video_stream(IntPtr sensor, VideoStream video_stream, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_software_sensor_add_read_only_option(IntPtr sensor, Option option, float val, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_software_sensor_update_read_only_option(IntPtr sensor, Option option, float val, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);

        #endregion
        #region rs_pipeline
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_pipeline(IntPtr ctx, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_pipeline_stop(IntPtr pipe, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_pipeline_wait_for_frames(IntPtr pipe, uint timeout_ms, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_pipeline_poll_for_frames(IntPtr pipe,
            //[Out, MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(FrameSetMarshaler))] out FrameSet output_frame,
            out IntPtr output_frame,
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_pipeline(IntPtr pipe);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_pipeline_start(IntPtr pipe, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_pipeline_start_with_config(IntPtr pipe, IntPtr config, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_pipeline_get_active_profile(IntPtr pipe, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_pipeline_profile_get_device(IntPtr profile, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_pipeline_profile_get_streams(IntPtr profile, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_pipeline_profile(IntPtr profile);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_config([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_delete_config(IntPtr config);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_config_enable_stream(IntPtr config, Stream stream, int index, int width, int height, Format format, int framerate, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_config_enable_all_stream(IntPtr config, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_config_enable_device(IntPtr config, [MarshalAs(UnmanagedType.LPStr)] string serial, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_config_enable_device_from_file(IntPtr config, [MarshalAs(UnmanagedType.LPStr)] string file, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_config_enable_record_to_file(IntPtr config, [MarshalAs(UnmanagedType.LPStr)] string file, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_config_disable_stream(IntPtr config, int stream, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_config_disable_indexed_stream(IntPtr config, Stream stream, int index, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_config_disable_all_streams(IntPtr config, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_config_resolve(IntPtr config, IntPtr pipe, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_config_can_resolve(IntPtr config, IntPtr pipe, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(Helpers.ErrorMarshaler))] out object error);


        #endregion

        #region Error Handling
        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_failed_function(IntPtr error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_failed_args(IntPtr error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_get_error_message(IntPtr error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_free_error(IntPtr error);
        #endregion

    }
}