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

        public byte[] RunTareCalibration(float ground_truth_mm, string json, int timeout_ms)
        {
            object error;
            IntPtr rawDataBuffer = NativeMethods.rs2_run_tare_calibration(Handle, ground_truth_mm, json, json.Length, null, IntPtr.Zero, timeout_ms, out error);
            return GetByteArrayFromRawDataObject(rawDataBuffer);
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
