using System;
using System.Linq;
using System.Collections.Generic;
using System.Windows.Forms;

namespace cs_config
{
    public partial class Config : Form
    {
        public Config()
        {
            InitializeComponent();
        }

        public void SetDevice(RealSense.Device d)
        {
            depthConfig.Setup(d, RealSense.Stream.Depth);
            colorConfig.Setup(d, RealSense.Stream.Color);
            infraredConfig.Setup(d, RealSense.Stream.Infrared);
            infrared2Config.Setup(d, RealSense.Stream.Infrared2);
        }
    }
}
