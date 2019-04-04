// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class SoftwareDevice : Device
    {
        private static IntPtr CreateSoftwareDevice()
        {
            object error;
            return NativeMethods.rs2_create_software_device(out error);
        }

        public SoftwareDevice()
            : base(CreateSoftwareDevice())
        {
        }

        public SoftwareSensor AddSensor(string name)
        {
            return new SoftwareSensor(AddSoftwareSensor(name));
        }

        public void SetMatcher(Matchers matcher)
        {
            object error;
            NativeMethods.rs2_software_device_create_matcher(Handle, matcher, out error);
        }

        internal IntPtr AddSoftwareSensor(string name)
        {
            object error;
            return NativeMethods.rs2_software_device_add_sensor(Handle, name, out error);
        }
    }
}
