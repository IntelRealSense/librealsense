        }

        public CaptureWindow()
{
    InitializeComponent();

    try
    {
        Action<VideoFrame> updateDepth;
        Action<VideoFrame> updateColor;

        // The colorizer processing block will be used to visualize the depth frames.
        colorizer = new Colorizer();

        // Create and config the pipeline to strem color and depth frames.
        pipeline = new Pipeline();

        var cfg = new Config();
        cfg.EnableStream(Stream.Depth, 640, 480);
        cfg.EnableStream(Stream.Color, Format.Rgb8);

        var pp = pipeline.Start(cfg);

        SetupWindow(pp, out updateDepth, out updateColor);

        Task.Factory.StartNew(() =>
        {
            while (!tokenSource.Token.IsCancellationRequested)
            {
                // We wait for the next available FrameSet and using it as a releaser object that would track
                // all newly allocated .NET frames, and ensure deterministic finalization
                // at the end of scope. 
                using (var frames = pipeline.WaitForFrames())
                {
                    var colorFrame = frames.ColorFrame.DisposeWith(frames);
                    var depthFrame = frames.DepthFrame.DisposeWith(frames);

                    // We colorize the depth frame for visualization purposes
                    var colorizedDepth = colorizer.Process<VideoFrame>(depthFrame).DisposeWith(frames);

                    // Render the frames.
                    Dispatcher.Invoke(DispatcherPriority.Render, updateDepth, colorizedDepth);
                    Dispatcher.Invoke(DispatcherPriority.Render, updateColor, colorFrame);

                    Dispatcher.Invoke(new Action(() =>
                    {
                        String depth_dev_sn = depthFrame.Sensor.Info[CameraInfo.SerialNumber];
                        txtTimeStamp.Text = depth_dev_sn + " : " + String.Format("{0,-20:0.00}", depthFrame.Timestamp) + "(" + depthFrame.TimestampDomain.ToString() + ")";
                    }));
                }
            }
        }, tokenSource.Token);
    }
    catch (Exception ex)
    {

        private void control_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            tokenSource.Cancel();
        }

        private void SetupWindow(PipelineProfile pipelineProfile, out Action<VideoFrame> depth, out Action<VideoFrame> color)
        {
            using (var p = pipelineProfile.GetStream(Stream.Depth).As<VideoStreamProfile>())
                imgDepth.Source = new WriteableBitmap(p.Width, p.Height, 96d, 96d, PixelFormats.Rgb24, null);
            depth = UpdateImage(imgDepth);

            using (var p = pipelineProfile.GetStream(Stream.Color).As<VideoStreamProfile>())
                imgColor.Source = new WriteableBitmap(p.Width, p.Height, 96d, 96d, PixelFormats.Rgb24, null);
            color = UpdateImage(imgColor);
        }
    }
}
