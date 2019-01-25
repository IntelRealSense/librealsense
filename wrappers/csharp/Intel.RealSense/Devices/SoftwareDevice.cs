using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class SoftwareDevice : Device
    {
        private static IntPtr CreateSoftwareDevice()
        {
            object error;
            return NativeMethods.rs2_create_software_device(out error);
        }

        public SoftwareDevice() : base(CreateSoftwareDevice()) { }


        internal IntPtr AddSoftwareSensor(string name)
        {
            object error;
            return NativeMethods.rs2_software_device_add_sensor(m_instance, name, out error);
        }

        public SoftwareSensor AddSensor(string name)
        {
            return new SoftwareSensor(AddSoftwareSensor(name));
        }

        public void SetMatcher(Matchers matcher)
        {
            object error;
            NativeMethods.rs2_software_device_create_matcher(m_instance, matcher, out error);
        }
    }
}
