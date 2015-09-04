using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace cs_capture
{
    public partial class Capture : Form
    {
        public RealSense.Camera Camera;

        public Capture()
        {
            InitializeComponent();
        }

        private void OnCaptureTick(object sender, EventArgs e)
        {
            if (Camera == null) return;

            captureTimer.Stop();
            Camera.WaitAllStreams();
            colorPictureBox.Image = new System.Drawing.Bitmap(640, 480, 640 * 3, System.Drawing.Imaging.PixelFormat.Format24bppRgb, Camera.GetImagePixels(RealSense.Stream.Color));
            captureTimer.Start();
        }
    }
}
