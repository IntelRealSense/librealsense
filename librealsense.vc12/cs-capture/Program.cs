using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace cs_capture
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            using (var context = new RealSense.Context())
            {
                int count = context.CameraCount;
                System.Console.WriteLine("There are " + count + " cameras.");
                for(int i=0; i<count; i++)
                {
                    var camera = context.GetCamera(i);

                    System.Console.WriteLine("Camera " + i + " is an " + camera.Name);
                    camera.EnableStreamPreset(RealSense.Stream.Depth, RealSense.Preset.BestQuality);
                    camera.EnableStreamPreset(RealSense.Stream.Color, RealSense.Preset.BestQuality);
                    camera.StartCapture();
                }
            }

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}
