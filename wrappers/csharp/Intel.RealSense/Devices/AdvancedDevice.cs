using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Devices
{
    public class AdvancedDevice : Device
    {
        public bool AdvancedModeEnabled
        {
            get
            {
                NativeMethods.rs2_is_enabled(Instance, out var enabled, out var error);
                return enabled == 1 ? true : false;
            }
            set
            {
                NativeMethods.rs2_toggle_advanced_mode(Instance, value ? 1 : 0, out var error);
            }
        }

        public string JsonConfiguration
        {
            get
            {
                IntPtr buffer = NativeMethods.rs2_serialize_json(Instance, out var error);
                int size = NativeMethods.rs2_get_raw_data_size(buffer, out error);
                IntPtr data = NativeMethods.rs2_get_raw_data(buffer, out error);

                return Marshal.PtrToStringAnsi(data, size);
            }
            set
            {
                NativeMethods.rs2_load_json(Instance, value, (uint)value.ToCharArray().Length, out var error);
            }
        }

        internal AdvancedDevice(IntPtr dev) : base(dev)
        {

        }

        public static AdvancedDevice FromDevice(Device dev)
        {
            if (NativeMethods.rs2_is_device_extendable_to(dev.Instance, Extension.AdvancedMode, out var error) == 0)
            {
                throw new ArgumentException("Device does not support AdvancedMode");
            }

            return new AdvancedDevice(dev.Instance);
        }
    }
}
