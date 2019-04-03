// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    public class AdvancedDevice : Device
    {
        internal AdvancedDevice(IntPtr dev)
            : base(dev)
        {
        }

        /// <summary>
        /// Create an <see cref="AdvancedDevice"/> from existing <see cref="Device"/>
        /// </summary>
        /// <param name="dev">a device that supports <see cref="Extension.AdvancedMode"/></param>
        /// <returns>a new <see cref="AdvancedDevice"/></returns>
        /// <exception cref="ArgumentException">Thrown when <paramref name="dev"/> does not support <see cref="Extension.AdvancedMode"/></exception>
        public static AdvancedDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.Handle, Extension.AdvancedMode, out error) == 0)
            {
                throw new ArgumentException($"Device does not support {nameof(Extension.AdvancedMode)}");
            }

            return Device.Create<AdvancedDevice>(dev.Handle);
        }

        /// <summary>
        /// Gets or sets a value indicating whether Advanced-Mode is enabled
        /// </summary>
        /// <value><see langword="true"/> when Advanced-Mode is enabled</value>
        public bool AdvancedModeEnabled
        {
            get
            {
                int enabled = 0;
                try
                {
                    object error;
                    NativeMethods.rs2_is_enabled(Handle, out enabled, out error);
                }
                catch
                {
                    // above might throw, ignored for this getter
                }

                return enabled == 1;
            }

            set
            {
                object error;
                NativeMethods.rs2_toggle_advanced_mode(Handle, value ? 1 : 0, out error);
            }
        }

        /// <summary>
        /// Gets or sets JSON and applies advanced-mode controls
        /// </summary>
        /// <value>Serialize JSON content</value>
        public string JsonConfiguration
        {
            get
            {
                object error;
                IntPtr buffer = NativeMethods.rs2_serialize_json(Handle, out error);
                int size = NativeMethods.rs2_get_raw_data_size(buffer, out error);
                IntPtr data = NativeMethods.rs2_get_raw_data(buffer, out error);
                var str = Marshal.PtrToStringAnsi(data, size);
                NativeMethods.rs2_delete_raw_data(buffer);
                return str;
            }

            set
            {
                object error;
                NativeMethods.rs2_load_json(Handle, value, (uint)value.Length, out error);
            }
        }
    }
}
