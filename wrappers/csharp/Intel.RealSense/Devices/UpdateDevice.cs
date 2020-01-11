// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class UpdateDevice : Device
    {
        private readonly rs2_update_progress_callback onUpdateProgressCallback;

        internal UpdateDevice(IntPtr dev)
            : base(dev)
        {
            onUpdateProgressCallback = new rs2_update_progress_callback(OnUpdateProgressInternal);
        }

        /// <summary>
        /// Create an <see cref="UpdateDevice"/> from existing <see cref="Device"/>
        /// </summary>
        /// <param name="dev">a device that supports <see cref="Extension.UpdateDevice"/></param>
        /// <returns>a new <see cref="UpdateDevice"/></returns>
        /// <exception cref="ArgumentException">Thrown when <paramref name="dev"/> does not support <see cref="Extension.UpdateDevice"/></exception>
        public static UpdateDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.Handle, Extension.UpdateDevice, out error) == 0)
            {
                throw new ArgumentException($"Device does not support {nameof(Extension.UpdateDevice)}");
            }

            return Device.Create<UpdateDevice>(dev.Handle);
        }

        /// <summary>
        /// Update an updatable device to the provided firmware, this call is executed on the caller's thread.
        /// For progress notifications, register to OnUpdateProgress event.
        /// </summary>
        public void Update(byte[] fwImage)
        {
            IntPtr nativeFwImage = Marshal.AllocHGlobal(fwImage.Length);
            Marshal.Copy(fwImage, 0, nativeFwImage, fwImage.Length);
            object error;
            NativeMethods.rs2_update_firmware(Handle, nativeFwImage, fwImage.Length, onUpdateProgressCallback, IntPtr.Zero, out error);
            Marshal.FreeHGlobal(nativeFwImage);
        }

        public delegate void OnUpdateProgressDelegate(float progress);
        public event OnUpdateProgressDelegate OnUpdateProgress;

        private void OnUpdateProgressInternal(float progress, IntPtr userData)
        {
            OnUpdateProgress?.Invoke(progress);
        }
    }
}
