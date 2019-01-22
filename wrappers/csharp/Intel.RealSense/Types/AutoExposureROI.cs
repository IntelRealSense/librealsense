using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Types
{
    public class AutoExposureROI
    {
        internal IntPtr Instance { get; private set;}

        public AutoExposureROI(IntPtr instance)
        {
            Instance = instance;
        }

        public ROI GetRegionOfInterest()
        {
            var result = new ROI();
            NativeMethods.rs2_get_region_of_interest(Instance, out result.minX,
                out result.minY, out result.maxX, out result.maxY, out var error);
            return result;
        }

        public void SetRegionOfInterest(ROI value)
        {
            NativeMethods.rs2_set_region_of_interest(Instance,
                value.minX, value.minY, value.maxX, value.maxY, out var error);
        }
    }
}
