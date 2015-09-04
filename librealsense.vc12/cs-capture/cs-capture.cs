using System;
using System.Drawing;
using System.Windows.Forms;

namespace cs_capture
{
    public class Capture : Form
    {
        private RealSense.Camera camera;
        private PictureBox box;
        private Timer timer;

        public Capture(RealSense.Camera camera)
        {
            this.camera = camera;
            var intrin = camera.GetStreamIntrinsics(RealSense.Stream.Color);
            int width = intrin.ImageSize[0], height = intrin.ImageSize[1];

            Text = string.Format("C# Capture Example ({0})", camera.Name);
            ClientSize = new System.Drawing.Size(width + 24, height + 24);

            box = new PictureBox { Location = new Point(12, 12), Size = new Size(width, height) };
            Controls.Add(box);

            timer = new Timer { Interval = 16 };
            timer.Tick += this.OnCaptureTick;
            timer.Start();
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing) timer.Dispose();
            base.Dispose(disposing);
        }

        private void OnCaptureTick(object sender, EventArgs e)
        {
            if (camera == null) return;

            timer.Stop();

            camera.WaitAllStreams();
            var intrin = camera.GetStreamIntrinsics(RealSense.Stream.Color);
            box.Image = new Bitmap(intrin.ImageSize[0], intrin.ImageSize[1], intrin.ImageSize[0] * 3, System.Drawing.Imaging.PixelFormat.Format24bppRgb, camera.GetImagePixels(RealSense.Stream.Color));

            timer.Start();
        }
    }

    static class Program
    {
        static void Main()
        {
            try
            {
                using (var context = new RealSense.Context())
                {
                    var cameras = context.Cameras;
                    if (cameras.Length < 1) throw new System.Exception("No cameras available. Is it plugged in?");

                    context.Cameras[0].EnableStream(RealSense.Stream.Color, 640, 480, RealSense.Format.BGR8, 60);
                    context.Cameras[0].StartCapture();

                    Application.EnableVisualStyles();
                    Application.SetCompatibleTextRenderingDefault(false);
                    Application.Run(new Capture(context.Cameras[0]));
                }
            }
            catch(RealSense.Error e)
            {
                MessageBox.Show(string.Format("Caught RealSense.Error while calling {0}({1}):\n  {2}\nProgram will abort.", 
                    e.FailedFunction, e.FailedArgs, e.Message), "RealSense Error", MessageBoxButtons.OK);
            }
            catch(System.Exception e)
            {
                MessageBox.Show(string.Format("Caught System.Exception:\n  {0}\nProgram will abort.", e.Message), "RealSense Error", MessageBoxButtons.OK);
            }
        }
    }
}
