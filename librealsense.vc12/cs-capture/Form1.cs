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
        private RealSense.Camera camera;

        public Form1()
        {
            InitializeComponent();
        }

        public void SetCamera(RealSense.Camera camera)
        {
            this.camera = camera;
            timer1.Start();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (camera == null) return;

            camera.WaitAllStreams();
            colorPictureBox.Image = new System.Drawing.Bitmap(640, 480, 640 * 3, System.Drawing.Imaging.PixelFormat.Format24bppRgb, camera.GetImagePixels(RealSense.Stream.Color));
        }
    }
}
