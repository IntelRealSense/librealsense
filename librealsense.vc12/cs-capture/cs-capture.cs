using System.Windows.Forms;

namespace cs_capture
{
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

                    context.Cameras[0].EnableStreamPreset(RealSense.Stream.Depth, RealSense.Preset.BestQuality);
                    context.Cameras[0].EnableStream(RealSense.Stream.Color, 640, 480, RealSense.Format.BGR8, 60);
                    context.Cameras[0].StartCapture();

                    Application.EnableVisualStyles();
                    Application.SetCompatibleTextRenderingDefault(false);
                    Application.Run(new Capture { Camera = context.Cameras[0], Text = string.Format("C# Capture Example ({0})", context.Cameras[0].Name) });
                }
            }
            catch(RealSense.Error e)
            {
                MessageBox.Show(string.Format("Caught RealSense.Error while calling {0}({1}):\n  {2}\nProgram will abort.", 
                    e.FailedFunction, e.FailedArgs, e.ErrorMessage), "RealSense Error", MessageBoxButtons.OK);
            }
            catch(System.Exception e)
            {
                MessageBox.Show(string.Format("Caught System.Exception:\n  {0}\nProgram will abort.", e.Message), "RealSense Error", MessageBoxButtons.OK);
            }
        }
    }
}
