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
            if (NativeMethods.rs2_is_device_extendable_to(dev.m_instance, Extension.AdvancedMode, out error) == 0)
            {
                throw new ArgumentException("Device does not support AdvancedMode");
            }

            return new AdvancedDevice(dev.m_instance);
        }

        public bool AdvancedModeEnabled
        {
            get
            {
                int enabled = 0;
                object error;
                NativeMethods.rs2_is_enabled(m_instance, out enabled, out error);

                return enabled == 1 ? true : false;
            }
            set
            {
                object error;
                NativeMethods.rs2_toggle_advanced_mode(m_instance, value ? 1 : 0, out error);
            }
        }

        public string JsonConfiguration
        {
            get
            {
                object error;
                IntPtr buffer = NativeMethods.rs2_serialize_json(m_instance, out error);
                int size = NativeMethods.rs2_get_raw_data_size(buffer, out error);
                IntPtr data = NativeMethods.rs2_get_raw_data(buffer, out error);

                return Marshal.PtrToStringAnsi(data, size);
            }
            set
            {
                object error;
                NativeMethods.rs2_load_json(m_instance, value, (uint)value.ToCharArray().Length, out error);
            }
        }
    }
}
