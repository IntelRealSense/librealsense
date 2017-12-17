using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Intel.RealSense
{
    /// <summary>
    /// Interaction logic for Window.xaml
    /// </summary>
    public partial class CaptureWindow : Window
    {
        private Pipeline  pipeline;
        private Colorizer colorizer;
        private bool      alive = true;

        private void UploadImage(Image img, Frame frame)
        {
            Dispatcher.Invoke(new Action(() =>
            {
                if (frame.Width == 0) return;

                var bytes = new byte[frame.Stride * frame.Height];
                frame.CopyTo(bytes);

                var bs = BitmapSource.Create(frame.Width, frame.Height,
                                  300, 300,
                                  PixelFormats.Rgb24,
                                  null,
                                  bytes,
                                  frame.Stride);

                frame.Release();
                var imgSrc = bs as ImageSource;

                img.Source = imgSrc;
            }));
        }

        public CaptureWindow()
        {
            pipeline = new Pipeline();
            colorizer = new Colorizer();

            var cfg = new Config();
            cfg.EnableStream(Stream.Depth, 640, 480);
            cfg.EnableStream(Stream.Color, Format.Rgb8);

            pipeline.Start(cfg);

            Thread t = new Thread(() =>
            {
                while (alive)
                {
                    using (var frames = pipeline.WaitForFrames())
                    {
                        var depth = frames.FirstOrDefault(x => x.Profile.Stream == Stream.Depth);
                        var color = frames.FirstOrDefault(x => x.Profile.Stream == Stream.Color);

                        if (depth != null)
                        {
                            var colorized_depth = colorizer.Colorize(depth);

                            UploadImage(imgDepth, colorized_depth);

                            depth.Release();
                        }

                        if (color != null)
                        {
                            UploadImage(imgColor, color);
                            color.Release();
                        }
                    }
                }
            });
            t.IsBackground = true;
            t.Start();

            InitializeComponent();
        }

        private void control_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            alive = false;
        }
    }
}
