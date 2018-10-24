using Intel.RealSense.Types;
using System;

namespace Intel.RealSense.Devices
{
    public class SoftwareDevice : Device
    {
        public SoftwareDevice() : base(CreateSoftwareDevice()) { }

        internal IntPtr AddSoftwareSensor(string name)
            => NativeMethods.rs2_software_device_add_sensor(Instance, name, out var error);

        public SoftwareSensor AddSensor(string name)
            => new SoftwareSensor(AddSoftwareSensor(name));

        public void SetMatcher(Matchers matcher)
            => NativeMethods.rs2_software_device_create_matcher(Instance, matcher, out var error);

        private static IntPtr CreateSoftwareDevice()
            => NativeMethods.rs2_create_software_device(out var error);
    }
}
