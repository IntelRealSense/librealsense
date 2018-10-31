﻿using System;
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
using System.Windows.Threading;

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

        static Action<VideoFrame> UpdateImage(Image img)
        {
            var wbmp = img.Source as WriteableBitmap;
            return new Action<VideoFrame>(frame =>
            {
                using (frame)
                {
                    var rect = new Int32Rect(0, 0, frame.Width, frame.Height);
                    wbmp.WritePixels(rect, frame.Data, frame.Stride * frame.Height, frame.Stride);
                }
            });
        }

        public ProcessingWindow()
        {
            InitializeComponent();

            try
            {
                var cfg = new Config();
                cfg.EnableStream(Stream.Depth, 640, 480);
                cfg.EnableStream(Stream.Color, Format.Rgb8);
                var pp = pipeline.Start(cfg);

                using (var p = pp.GetStream(Stream.Depth) as VideoStreamProfile)
                    imgDepth.Source = new WriteableBitmap(p.Width, p.Height, 96d, 96d, PixelFormats.Rgb24, null);
                var updateDepth = UpdateImage(imgDepth);

                using (var p = pp.GetStream(Stream.Color) as VideoStreamProfile)
                    imgColor.Source = new WriteableBitmap(p.Width, p.Height, 96d, 96d, PixelFormats.Rgb24, null);
                var updateColor = UpdateImage(imgColor);

                // Create custom processing block
                // For demonstration purposes it will:
                // a. Get a frameset
                // b. Run post-processing on the depth frame
                // c. Combine the result back into a frameset
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
                        var frames = FrameSet.FromFrame(f).DisposeWith(releaser);

                        var processedFrames = frames.ApplyFilter(decimate).DisposeWith(releaser)
                                                .ApplyFilter(spatial).DisposeWith(releaser)
                                                .ApplyFilter(temp).DisposeWith(releaser)
                                                .ApplyFilter(align).DisposeWith(releaser)
                                                .ApplyFilter(colorizer).DisposeWith(releaser);

                        var colorFrame = processedFrames.ColorFrame.DisposeWith(releaser);
                        var colorizedDepth = processedFrames[Stream.Depth, Format.Rgb8].DisposeWith(releaser);

                        // Combine the frames into a single result
                        var res = src.AllocateCompositeFrame(colorizedDepth, colorFrame).DisposeWith(releaser);
                        // Send it to the next processing stage
                        src.FramesReady(res);
                    }
                });

                // Register to results of processing via a callback:
                block.Start(f =>
                {
                    using (var frames = FrameSet.FromFrame(f))
                    {
                        var colorFrame = frames.ColorFrame.DisposeWith(frames);
                        var colorizedDepth = frames[Stream.Depth, Format.Rgb8].DisposeWith(frames);

                        Dispatcher.Invoke(DispatcherPriority.Render, updateDepth, colorizedDepth);
                        Dispatcher.Invoke(DispatcherPriority.Render, updateColor, colorFrame);
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
