// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    public enum Matchers
    {
        DI,      // compare depth and ir based on frame number
        DI_C,    // compare depth and ir based on frame number,
                 // compare the pair of corresponding depth and ir with color based on closest timestamp,
        DLR_C,   // compare depth, left and right ir based on frame number,
                 // compare the set of corresponding depth, left and right with color based on closest timestamp,
                 // commonlly used by RS415, RS435
        DLR,     // compare depth, left and right ir based on frame number,
                 // commonlly used by RS400, RS405, RS410, RS420, RS430
        DIC,     //compare depth, ir and confidence based on frame number used by RS500
        DIC_C,   //compare depth, ir and confidence based on frame number,
                 //compare the set of corresponding depth, ir and confidence with color based on closest timestamp,
                 //commonly used by RS515
        Default, // the default matcher compare all the streams based on closest timestamp
    }
}
