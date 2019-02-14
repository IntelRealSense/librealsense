using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense
{
    [StructLayout(LayoutKind.Sequential)]
    public class Pose
    {
        ///<summary> X, Y, Z values of translation, in meters (relative to initial position) </summary>
        public Vector translation;

        ///<summary> X, Y, Z values of velocity, in meter/sec </summary>
        public Vector velocity;

        ///<summary> X, Y, Z values of acceleration, in meter/sec^2 </summary>
        public Vector acceleration;

        ///<summary> Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position) </summary>
        public Quaternion rotation;

        ///<summary> X, Y, Z values of angular velocity, in radians/sec </summary>
        public Vector angular_velocity;

        ///<summary> X, Y, Z values of angular acceleration, in radians/sec^2 </summary>
        public Vector angular_acceleration;

        ///<summary> pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High </summary>
        public uint tracker_confidence;

        ///<summary> pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High </summary>
        public uint mapper_confidence;
    }
}
