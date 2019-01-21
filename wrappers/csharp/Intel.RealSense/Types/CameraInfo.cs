using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public enum CameraInfo
    {
        Name = 0,
        SerialNumber = 1,
        RecommendedFirmwareVersion = 2,
        FirmwareVersion = 3,
        PhysicalPort = 4,
        DebugOpCode = 5,
        AdvancedMode = 6,
        ProductId = 7,
        CameraLocked = 8,
        UsbTypeDescriptor = 9,
    }
}
