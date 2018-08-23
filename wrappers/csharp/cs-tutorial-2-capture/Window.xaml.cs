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

        private CancellationTokenSource tokenSource = new CancellationTokenSource();

        private void UploadImage(Image img, VideoFrame frame)
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

                var imgSrc = bs as ImageSource;

                img.Source = imgSrc;
            }));
        }

        public CaptureWindow()
        {
            try
            {
                pipeline = new Pipeline();
                colorizer = new Colorizer();

                var cfg = new Config();
                cfg.EnableStream(Stream.Depth, 640, 480);
                cfg.EnableStream(Stream.Color, Format.Rgb8);

                pipeline.Start(cfg);

                var token = tokenSource.Token;

                var t = Task.Factory.StartNew(() =>
                {
                    while (!token.IsCancellationRequested)
                    {
                        var frames = pipeline.WaitForFrames();

                        var colorized_depth_frame = colorizer.Colorize(frames.DepthFrame);
                        var color_frame = frames.ColorFrame;

                        UploadImage(imgDepth, colorized_depth_frame);
                        UploadImage(imgColor, color_frame);

                        // It is important to pre-emptively dispose of native resources
                        // to avoid creating bottleneck at finalization stage after GC
                        // (Also see FrameReleaser helper object in next tutorial)
                        frames.Dispose();
                        colorized_depth_frame.Dispose();
                        color_frame.Dispose();
                    }
                }, token);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                Application.Current.Shutdown();
            }

            InitializeComponent();
        }

        private void control_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            tokenSource.Cancel();
        }
    }
}
