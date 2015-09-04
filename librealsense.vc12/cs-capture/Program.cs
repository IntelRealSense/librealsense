using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace cs_capture
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            using (var context = new RealSense.Context())
            {
                if (context.CameraCount < 1) throw new Exception("No cameras available. Is it plugged in?");

                var camera = context.GetCamera(0);               
                camera.EnableStreamPreset(RealSense.Stream.Depth, RealSense.Preset.BestQuality);
                camera.EnableStream(RealSense.Stream.Color, 640, 480, RealSense.Format.BGR8, 60);
                camera.StartCapture();

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new Form1 {Camera = camera});
            }
        }
    }
}
