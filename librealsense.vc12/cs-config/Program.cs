using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace cs_config
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
                if (context.GetDeviceCount() == 0)
                {
                    MessageBox.Show("No RealSense devices detected. Program will now exit.", "C# Capture Example", MessageBoxButtons.OK);
                    return;
                }

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                var form = new Config();
                form.SetDevice(context.GetDevice(0));
                Application.Run(form);
            }
        }
    }
}
