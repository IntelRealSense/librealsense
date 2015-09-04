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

            unsafe
            {
                var intrin = Camera.GetStreamIntrinsics(RealSense.Stream.Color);
                colorPictureBox.Image = new System.Drawing.Bitmap(intrin.ImageSize[0], intrin.ImageSize[1], intrin.ImageSize[0] * 3, System.Drawing.Imaging.PixelFormat.Format24bppRgb, Camera.GetImagePixels(RealSense.Stream.Color));

                /*intrin = Camera.GetStreamIntrinsics(RealSense.Stream.Infrared);
                var bitmap = new System.Drawing.Bitmap(intrin.ImageSize[0], intrin.ImageSize[1], intrin.ImageSize[0], System.Drawing.Imaging.PixelFormat.Format8bppIndexed, Camera.GetImagePixels(RealSense.Stream.Infrared));
                var palette = bitmap.Palette;
                var entries = palette.Entries;
                for (int i = 0; i < 256; i++) entries[i] = Color.FromArgb(i, i, i);
                bitmap.Palette = palette;
                irPictureBox.Image = bitmap;*/
            }
            captureTimer.Start();
        }
    }
}
