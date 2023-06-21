// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class DebugDevice : Device
    {

        internal DebugDevice(IntPtr dev)
            : base(dev)
        { }

        /// <summary>
        /// Create an <see cref="DebugDevice"/> from existing <see cref="Device"/>
        /// </summary>
        /// <param name="dev">a device that supports <see cref="Extension.Debug"/></param>
        /// <returns>a new <see cref="DebugDevice"/></returns>
        /// <exception cref="ArgumentException">Thrown when <paramref name="dev"/> does not support <see cref="Extension.FirmwareLogger"/></exception>
        public static DebugDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.Handle, Extension.Debug, out error) == 0)
            {
                throw new ArgumentException($"Device does not support {nameof(Extension.Debug)}");
            }

            return Device.Create<DebugDevice>(dev.Handle);
        }

        public byte[] SendReceiveRawData(byte[] command_bytes)
        {
            IntPtr nativeBytes = IntPtr.Zero;
            try
            {
                nativeBytes = Marshal.AllocHGlobal(command_bytes.Length);
                Marshal.Copy(command_bytes, 0, nativeBytes, command_bytes.Length);
                object error;
                IntPtr rawDataBuffer = NativeMethods.rs2_send_and_receive_raw_data(Handle, nativeBytes, (uint)command_bytes.Length, out error);

                IntPtr start = NativeMethods.rs2_get_raw_data(rawDataBuffer, out error);
                int size = NativeMethods.rs2_get_raw_data_size(rawDataBuffer, out error);

                byte[] managedBytes = new byte[size];
                Marshal.Copy(start, managedBytes, 0, size);
                NativeMethods.rs2_delete_raw_data(rawDataBuffer);

                return managedBytes;
            }
            finally
            {
                Marshal.FreeHGlobal(nativeBytes);
            }
        }
    }
}
