// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    public enum Matchers
    {
        DI,      // compare depth and ir based on frame number
        DI_C,    // compare depth and ir based on frame number,
                 // compare the pair of corresponding depth and ir with color based on closest timestamp,
                 // commonlly used by SR300
        DLR_C,   // compare depth, left and right ir based on frame number,
                 // compare the set of corresponding depth, left and right with color based on closest timestamp,
                 // commonlly used by RS415, RS435
        DLR,     // compare depth, left and right ir based on frame number,
                 // commonlly used by RS400, RS405, RS410, RS420, RS430
        Default, // the default matcher compare all the streams based on closest timestamp
    }
}
