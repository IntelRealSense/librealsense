using System;
using System.Linq;

namespace Intel.RealSense
{
    class Program
    {
        static void Main(string[] args)
        {
            using (var pipe = new Pipeline())
            {
                pipe.Start();

                var run = true;
                Console.CancelKeyPress += delegate (object sender, ConsoleCancelEventArgs e)
                {
                    e.Cancel = true;
                    run = false;
                };

                while (run)
                {
                    using (var frames = pipe.WaitForFrames())
                    {
                        Console.WriteLine("Got " + frames.Count + " frames!");
                        var depth = frames.FirstOrDefault(x => x.Profile.Stream == Stream.Depth);
                        var color = frames.FirstOrDefault(x => x.Profile.Stream == Stream.Color);

                        if (depth != null)
                        {

                            depth.Release();
                        }

                        if (color != null)
                        {

                            color.Release();
                        }
                    }
                }
            }
        }
    }
}
