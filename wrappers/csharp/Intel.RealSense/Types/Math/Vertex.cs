// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense.Math
{
    /// <summary>
    /// 3D coordinates with origin at topmost left corner of the lense,
    /// with positive Z pointing away from the camera, positive X pointing camera right and positive Y pointing camera down</summary>
    public struct Vertex
    {
        public float x;
        public float y;
        public float z;
    }
}
