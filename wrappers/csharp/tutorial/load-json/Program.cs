using System;
using System.Linq;

namespace Intel.RealSense
{
    class Program
    {
        [STAThread]
        static void Main(string[] args)
        {
            var w = new CaptureWindow();
            w.ShowDialog();
        }
    }
}
