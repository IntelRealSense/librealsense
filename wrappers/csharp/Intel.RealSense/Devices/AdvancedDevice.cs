using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class AdvancedDevice : Device
    {
        internal AdvancedDevice(IntPtr dev) : base(dev)
        {
        }

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
        /// Enable/Disable Advanced-Mode
        /// </summary>
        /// <value>
        /// Check if Advanced-Mode is enabled
        /// </value>
        public bool AdvancedModeEnabled
        {
            get
            {
                int enabled = 0;
                try
                {
                    object error;
                    NativeMethods.rs2_is_enabled(Handle, out enabled, out error);
                } catch
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
        /// Load JSON and apply advanced-mode controls
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
