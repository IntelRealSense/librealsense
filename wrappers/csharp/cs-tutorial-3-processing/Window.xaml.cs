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

// This demo showcases some of the more advanced API concepts:
// a. Post-processing and stream alignment
// b. Using callbacks
// c. Defining custom processing blocks
// d. Using FramesReleaser to help manage frames lifetime
namespace Intel.RealSense
{
    /// <summary>
    /// Interaction logic for Window.xaml
    /// </summary>
    public partial class ProcessingWindow : Window
    {
        private Pipeline pipeline = new Pipeline();
        private Colorizer colorizer = new Colorizer();
        private Align align = new Align(Stream.Color);
        private DecimationFilter decimate = new DecimationFilter();
        private SpatialFilter spatial = new SpatialFilter();
        private TemporalFilter temp = new TemporalFilter();
        private CustomProcessingBlock block;
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

        public ProcessingWindow()
        {
            try
            {
                var cfg = new Config();
                cfg.EnableStream(Stream.Depth, 640, 480);
                cfg.EnableStream(Stream.Color, Format.Rgb8);
                pipeline.Start(cfg);

                // Create custom processing block
                // For demonstration purposes it will:
                // a. Get a frameset
                // b. Break it down to frames
                // c. Run post-processing on the depth frame
                // d. Combine the result back into a frameset
                // Processing blocks are inherently thread-safe and play well with
                // other API primitives such as frame-queues, 
                // and can be used to encapsulate advanced operations.
                // All invokations are, however, synchronious so the high-level threading model
                // is up to the developer
                block = new CustomProcessingBlock((f, src) =>
                {
                    // We create a FrameReleaser object that would track
                    // all newly allocated .NET frames, and ensure deterministic finalization
                    // at the end of scope. 
                    using (var releaser = new FramesReleaser())
                    {
                        var frames = FrameSet.FromFrame(f, releaser);

                        VideoFrame depth = FramesReleaser.ScopedReturn(releaser, frames.DepthFrame);
                        VideoFrame color = FramesReleaser.ScopedReturn(releaser, frames.ColorFrame);

                        // Apply depth post-processing
                        depth = decimate.ApplyFilter(depth, releaser);
                        depth = spatial.ApplyFilter(depth, releaser);
                        depth = temp.ApplyFilter(depth, releaser);

                        // Combine the frames into a single result
                        var res = src.AllocateCompositeFrame(releaser, depth, color);
                        // Send it to the next processing stage
                        src.FramesReady(res);
                    }
                });

                // Register to results of processing via a callback:
                block.Start(f =>
                {
                    using (var releaser = new FramesReleaser())
                    {
                        // Align, colorize and upload frames for rendering
                        var frames = FrameSet.FromFrame(f, releaser);

                        // Align both frames to the viewport of color camera
                        frames = align.Process(frames, releaser);

                        var depth_frame = FramesReleaser.ScopedReturn(releaser, frames.DepthFrame);
                        var color_frame = FramesReleaser.ScopedReturn(releaser, frames.ColorFrame);

                        UploadImage(imgDepth, colorizer.Colorize(depth_frame, releaser));
                        UploadImage(imgColor, color_frame);
                    }
                });

                var token = tokenSource.Token;

                var t = Task.Factory.StartNew(() =>
                {
                    while (!token.IsCancellationRequested)
                    {
                        using (var frames = pipeline.WaitForFrames())
                        {
                            // Invoke custom processing block
                            block.ProcessFrames(frames);
                        }
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
