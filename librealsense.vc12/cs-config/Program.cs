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
                var devices = context.Devices;
                if (devices.Length < 1)
                {
                    MessageBox.Show("No RealSense devices detected. Program will now exit.", "C# Config Example", MessageBoxButtons.OK);
                    return;
                }

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                var form = new Config();
                form.SetDevice(devices[0]);
                Application.Run(form);
            }
        }
    }
}
