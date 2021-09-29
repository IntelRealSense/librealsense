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
            Config cfg = new Config();
            cfg.EnableStream(Stream.Depth, 640, 480, Format.Z16, 30);
            cfg.EnableStream(Stream.Color, 640, 480, Format.Rgb8, 30);

            var w = new CaptureWindow(args[0], cfg);
            w.ShowDialog();
        }
    }
}
