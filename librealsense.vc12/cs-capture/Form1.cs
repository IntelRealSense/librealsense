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
    public partial class Form1 : Form
    {
        public RealSense.Camera Camera;

        public Form1()
        {
            InitializeComponent();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (Camera == null) return;

            timer1.Stop();
            Camera.WaitAllStreams();
            colorPictureBox.Image = new System.Drawing.Bitmap(640, 480, 640 * 3, System.Drawing.Imaging.PixelFormat.Format24bppRgb, Camera.GetImagePixels(RealSense.Stream.Color));
            timer1.Start();
        }
    }
}
