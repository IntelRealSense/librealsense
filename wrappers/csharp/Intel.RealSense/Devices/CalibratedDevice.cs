// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;

    public class CalibratedDevice : Device
    {
        internal CalibratedDevice(IntPtr dev)
            : base(dev)
        { }

        public static CalibratedDevice FromDevice(Device dev)
        {
            return Device.Create<CalibratedDevice>(dev.Handle); ;
        }

        public void WriteCalibration()
        {
            object error;
            NativeMethods.rs2_write_calibration(Handle, out error);
        }

        public void reset_to_factory_calibration()
        {
            object error;
            NativeMethods.rs2_reset_to_factory_calibration(Handle, out error);
        }
}
}
