// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;

    [StructLayout(LayoutKind.Sequential)]
    public class Pose
    {
        /// <summary> X, Y, Z values of translation, in meters (relative to initial position) </summary>
        public Math.Vector translation;

        /// <summary> X, Y, Z values of velocity, in meter/sec </summary>
        public Math.Vector velocity;

        /// <summary> X, Y, Z values of acceleration, in meter/sec^2 </summary>
        public Math.Vector acceleration;

        /// <summary> Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position) </summary>
        public Math.Quaternion rotation;

        /// <summary> X, Y, Z values of angular velocity, in radians/sec </summary>
        public Math.Vector angular_velocity;

        /// <summary> X, Y, Z values of angular acceleration, in radians/sec^2 </summary>
        public Math.Vector angular_acceleration;

        /// <summary> pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High </summary>
        public uint tracker_confidence;

        /// <summary> pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High </summary>
        public uint mapper_confidence;
    }
}
