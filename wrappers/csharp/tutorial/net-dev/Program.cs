using System;
using System.Linq;

namespace Intel.RealSense
{
    class Program
    {
        [STAThread]
        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine($"Please, specify network server IP address as a parameter");
                return;
            }
            var w = new CaptureWindow(args[0]);
            w.ShowDialog();
        }
    }
}
