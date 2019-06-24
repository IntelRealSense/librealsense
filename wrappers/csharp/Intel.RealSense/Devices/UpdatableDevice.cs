// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class UpdatableDevice : Device
    {
        private readonly rs2_update_progress_callback onFlashBackupProgressCallback;

        internal UpdatableDevice(IntPtr dev)
            : base(dev)
        {
            onFlashBackupProgressCallback = new rs2_update_progress_callback(OnFlashBackupProgressInternal);
        }

        /// <summary>
        /// Create an <see cref="UpdatableDevice"/> from existing <see cref="Device"/>
        /// </summary>
        /// <param name="dev">a device that supports <see cref="Extension.Updatable"/></param>
        /// <returns>a new <see cref="AdvancedDevice"/></returns>
        /// <exception cref="ArgumentException">Thrown when <paramref name="dev"/> does not support <see cref="Extension.Updatable"/></exception>
        public static UpdatableDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.Handle, Extension.Updatable, out error) == 0)
            {
                throw new ArgumentException($"Device does not support {nameof(Extension.Updatable)}");
            }

            return Device.Create<UpdatableDevice>(dev.Handle);
        }

        /// <summary>
        /// Enter the device to update state, this will cause the updatable device to disconnect and reconnect as UpdateDevice
        /// </summary>
        public void EnterUpdateState()
        {
            object error;
            NativeMethods.rs2_enter_update_state(Handle, out error);
        }

        public byte[] CreateFlashBackup()
        {
            object error;
            var rawDataPtr = NativeMethods.rs2_create_flash_backup(Handle, onFlashBackupProgressCallback, IntPtr.Zero, out error);
            var size = NativeMethods.rs2_get_raw_data_size(rawDataPtr, out error);
            var flashBackupPtr = NativeMethods.rs2_get_raw_data(rawDataPtr, out error);
            byte[] rv = new byte[size];
            Marshal.Copy(flashBackupPtr, rv, 0, size);
            return rv;
        }

        public delegate void OnFlashBackupProgressDelegate(float progress);
        public event OnFlashBackupProgressDelegate OnFlashBackupProgress;

        private void OnFlashBackupProgressInternal(float progress, IntPtr userData)
        {
            OnFlashBackupProgress?.Invoke(progress);
        }
    }
}
