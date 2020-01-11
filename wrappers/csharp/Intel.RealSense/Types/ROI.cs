// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Region of Intrest
    /// </summary>
    public struct ROI
    {
        public int minX;
        public int minY;
        public int maxX;
        public int maxY;
    }
}
