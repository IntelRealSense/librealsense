using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    /// <summary>
    /// Read-only strings that can be queried from the device.
    /// </summary>
    /// <remarks>
    /// Not all information attributes are available on all camera types.
    /// This information is mainly available for camera debug and troubleshooting and should not be used in applications. */
    /// </remarks>
    public enum CameraInfo
    {
        Name = 0,
        SerialNumber = 1,
        FirmwareVersion = 2,
        RecommendedFirmwareVersion = 3,
        PhysicalPort = 4,
        DebugOpCode = 5,
        AdvancedMode = 6,
        ProductId = 7,
        CameraLocked = 8,
        UsbTypeDescriptor = 9,
    }
}
