// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;

    public class UpdatableDevice : Device
    {
        internal UpdatableDevice(IntPtr dev)
            : base(dev)
        {
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
    }
}
