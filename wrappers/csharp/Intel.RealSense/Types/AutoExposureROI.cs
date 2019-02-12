using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class AutoExposureROI
    {
        internal IntPtr m_instance;

        public ROI GetRegionOfInterest()
        {
            var result = new ROI();
            object error;
            NativeMethods.rs2_get_region_of_interest(m_instance, out result.minX,
                out result.minY, out result.maxX, out result.maxY, out error);
            return result;
        }

        public void SetRegionOfInterest(ROI value)
        {
            object error;
            NativeMethods.rs2_set_region_of_interest(m_instance,
                value.minX, value.minY, value.maxX, value.maxY, out error);
        }
    }
}
