// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class AutoCalibratedDevice : CalibratedDevice
    {
        internal AutoCalibratedDevice(IntPtr dev)
            : base(dev)
        { }
        public static AutoCalibratedDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.Handle, Extension.AutoCalibratedDevice, out error) == 0)
            {
                throw new ArgumentException($"Device does not support {nameof(Extension.AutoCalibratedDevice)}");
            }

            return Device.Create<AutoCalibratedDevice>(dev.Handle);
        }

        public byte[] RunOnChipCalibration(string json, out float health, int timeout_ms)
        {
            object error;
            IntPtr rawDataBuffer = NativeMethods.rs2_run_on_chip_calibration(Handle, json, json.Length, out health, null, IntPtr.Zero, timeout_ms, out error);
            return GetByteArrayFromRawDataObject(rawDataBuffer);
        }

        public byte[] RunOnChipCalibration(string json, out float health, ProgressCallback cb, int timeout_ms)
        {
            object error;
            rs2_update_progress_callback cb2 = (float progress, IntPtr u) =>
            {
                cb((IntPtr)progress);
            };
            IntPtr rawDataBuffer = NativeMethods.rs2_run_on_chip_calibration(Handle, json, json.Length, out health, cb2, IntPtr.Zero, timeout_ms, out error);
            return GetByteArrayFromRawDataObject(rawDataBuffer);
        }

        public byte[] RunTareCalibration(float ground_truth_mm, string json, int timeout_ms)
        {
            object error;
            IntPtr rawDataBuffer = NativeMethods.rs2_run_tare_calibration(Handle, ground_truth_mm, json, json.Length, null, IntPtr.Zero, timeout_ms, out error);
            return GetByteArrayFromRawDataObject(rawDataBuffer);
        }

        public byte[] RunTareCalibration(float ground_truth_mm, string json, ProgressCallback cb, int timeout_ms)
        {
            object error;
            rs2_update_progress_callback cb2 = (float progress, IntPtr u) =>
            {
                cb((IntPtr)progress);
            };
            IntPtr rawDataBuffer = NativeMethods.rs2_run_tare_calibration(Handle, ground_truth_mm, json, json.Length, cb2, IntPtr.Zero, timeout_ms, out error);
            return GetByteArrayFromRawDataObject(rawDataBuffer);
        }

        public byte[] RunFocalLengthCalibration(FrameQueue left, FrameQueue right, float target_width_mm, float target_height_mm, int adjust_both_sides, out float ratio, out float angle)
        {
            object error;
            IntPtr rawDataBuffer = NativeMethods.rs2_run_focal_length_calibration(Handle, left.Handle, right.Handle,
                target_width_mm, target_height_mm, adjust_both_sides, out ratio, out angle, null, out error);
            return GetByteArrayFromRawDataObject(rawDataBuffer);
        }

        public byte[] RunFocalLengthCalibration(FrameQueue left, FrameQueue right, float target_width_mm, float target_height_mm, int adjust_both_sides, out float ratio, out float angle, ProgressCallback cb)
        {
            object error;
            rs2_update_progress_callback cb2 = (float progress, IntPtr u) =>
            {
                cb((IntPtr)progress);
            };
            IntPtr rawDataBuffer = NativeMethods.rs2_run_focal_length_calibration(Handle, left.Handle, right.Handle,
                target_width_mm, target_height_mm, adjust_both_sides, out ratio, out angle, cb2, out error);
            return GetByteArrayFromRawDataObject(rawDataBuffer);
        }

        public byte[] RunUVMapCalibration(FrameQueue left, FrameQueue color, FrameQueue depth, int px_py_only, out float ratio, out float angle)
        {
            object error;
            IntPtr rawDataBuffer = NativeMethods.rs2_run_uv_map_calibration(Handle, left.Handle, color.Handle, depth.Handle,
                px_py_only, out ratio, out angle, null, out error);
            return GetByteArrayFromRawDataObject(rawDataBuffer);
        }

        public byte[] RunUVMapCalibration(FrameQueue left, FrameQueue color, FrameQueue depth, int px_py_only, out float ratio, out float angle, ProgressCallback cb)
        {
            object error;
            rs2_update_progress_callback cb2 = (float progress, IntPtr u) =>
            {
                cb((IntPtr)progress);
            };
            IntPtr rawDataBuffer = NativeMethods.rs2_run_uv_map_calibration(Handle, left.Handle, color.Handle, depth.Handle,
                px_py_only, out ratio, out angle, cb2, out error);
            return GetByteArrayFromRawDataObject(rawDataBuffer);
        }

        public float CalculateTargetZ(FrameQueue frame_queue1, FrameQueue frame_queue2, FrameQueue frame_queue3, float target_width_mm, float target_height_mm)
        {
            object error;
            return NativeMethods.rs2_calculate_target_z(Handle, frame_queue1.Handle, frame_queue2.Handle, frame_queue3.Handle, target_width_mm, target_height_mm, null, out error);
        }

        public float CalculateTargetZ(FrameQueue frame_queue1, FrameQueue frame_queue2, FrameQueue frame_queue3, float target_width_mm, float target_height_mm, ProgressCallback cb)
        {
            object error;
            rs2_update_progress_callback cb2 = (float progress, IntPtr u) =>
            {
                cb((IntPtr)progress);
            };
            return NativeMethods.rs2_calculate_target_z(Handle, frame_queue1.Handle, frame_queue2.Handle, frame_queue3.Handle, target_width_mm, target_height_mm, cb2, out error);
        }

        public byte[] CalibrationTable
        {
            get
            {
                object error;
                IntPtr rawDataBuffer = NativeMethods.rs2_get_calibration_table(Handle, out error);
                return GetByteArrayFromRawDataObject(rawDataBuffer);
            }
            set
            {
                IntPtr rawCalibrationTable = Marshal.AllocHGlobal(value.Length);
                Marshal.Copy(value, 0, rawCalibrationTable, value.Length);
                object error;
                NativeMethods.rs2_set_calibration_table(Handle, rawCalibrationTable, value.Length, out error);
                Marshal.FreeHGlobal(rawCalibrationTable);
            }
        }

        byte[] GetByteArrayFromRawDataObject(IntPtr raw)
        {
            object error;
            var start = NativeMethods.rs2_get_raw_data(raw, out error);
            var size = NativeMethods.rs2_get_raw_data_size(raw, out error);

            byte[] managedBytes = new byte[size];
            Marshal.Copy(start, managedBytes, 0, size);

            NativeMethods.rs2_delete_raw_data(raw);
            return managedBytes;
        }
    }
}
