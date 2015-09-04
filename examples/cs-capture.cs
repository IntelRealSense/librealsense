using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using System.Runtime.InteropServices;

public class Capture : Form
{
    private RealSense.Device device;
    private short[] depthMap;
    private uint[] depthHistogram;
    private byte[] depthImage;

    private PictureBox colorPicture, depthPicture;
    private Timer timer;

    public Capture(RealSense.Device device)
    {
        this.device = device;

        var colorIntrin = device.GetStreamIntrinsics(RealSense.Stream.Color);
        var depthIntrin = device.GetStreamIntrinsics(RealSense.Stream.Depth);

        depthMap = new short[depthIntrin.ImageSize[0] * depthIntrin.ImageSize[1]];
        depthHistogram = new uint[0x10000];
        depthImage = new byte[depthMap.Length * 3];

        Text = string.Format("C# Capture Example ({0})", device.Name);
        ClientSize = new System.Drawing.Size(colorIntrin.ImageSize[0] + depthIntrin.ImageSize[0] + 36, colorIntrin.ImageSize[1] + 24);

        colorPicture = new PictureBox { Location = new Point(12, 12), Size = new Size(colorIntrin.ImageSize[0], colorIntrin.ImageSize[1]) };
        Controls.Add(colorPicture);

        depthPicture = new PictureBox { Location = new Point(24 + colorIntrin.ImageSize[0], 12), Size = new Size(depthIntrin.ImageSize[0], depthIntrin.ImageSize[1]) };
        Controls.Add(depthPicture);

        timer = new Timer { Interval = 10 };
        timer.Tick += this.OnCaptureTick;
        timer.Start();
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing) timer.Dispose();
        base.Dispose(disposing);
    }

    private void OnCaptureTick(object sender, EventArgs e)
    {
        if (device == null) return;

        timer.Stop();

        device.WaitForFrames(RealSense.Streams.All);

        // Obtain color image data
        RealSense.Intrinsics intrinsics = device.GetStreamIntrinsics(RealSense.Stream.Color);
        int width = intrinsics.ImageSize[0], height = intrinsics.ImageSize[1];

        // Create a bitmap of the color image and assign it to our PictureBox
        colorPicture.Image = new Bitmap(width, height, width * 3, PixelFormat.Format24bppRgb, device.GetFrameData(RealSense.Stream.Color));

        // Obtain depth image data
        Marshal.Copy(device.GetFrameData(RealSense.Stream.Depth), depthMap, 0, depthMap.Length);
        intrinsics = device.GetStreamIntrinsics(RealSense.Stream.Depth);
        width = intrinsics.ImageSize[0];
        height = intrinsics.ImageSize[1]; 

        // Build a cumulative histogram for of depth values in [1,0xFFFF]      
        for (int i = 0; i < depthHistogram.Length; i++) depthHistogram[i] = 0;
        for (int i = 0; i < depthMap.Length; i++) depthHistogram[(ushort)depthMap[i]]++;
        for (int i = 2; i < depthHistogram.Length; i++) depthHistogram[i] += depthHistogram[i - 1]; 

        // Produce an image in BGR ordered byte array
        for (int i = 0; i < depthMap.Length; ++i)
        {
            ushort d = (ushort)depthMap[i];
            if(d != 0)
            {
                uint f = depthHistogram[d] * 255 / depthHistogram[0xFFFF]; // 0-255 based on histogram location
                depthImage[i*3 + 0] = (byte)f;
                depthImage[i * 3 + 1] = 0;
                depthImage[i * 3 + 2] = (byte)(255 - f);
            }
            else
            {
                depthImage[i * 3 + 0] = 0;
                depthImage[i * 3 + 1] = 5;
                depthImage[i * 3 + 2] = 20;
            }
        }

        // Create a bitmap of the histogram colored depth image and assign it to our PictureBox
        GCHandle handle = GCHandle.Alloc(depthImage, GCHandleType.Pinned);
        depthPicture.Image = new Bitmap(width, height, width * 3, PixelFormat.Format24bppRgb, handle.AddrOfPinnedObject());
        handle.Free();

        timer.Start();
    }

    static void Main()
    {
        try
        {
            using (var context = new RealSense.Context())
            {
                var cameras = context.Cameras;
                if (cameras.Length < 1)
                {
                    MessageBox.Show("No RealSense cameras detected. Program will now exit.", "C# Capture Example", MessageBoxButtons.OK);
                    return;
                }

                context.Cameras[0].EnableStreamPreset(RealSense.Stream.Depth, RealSense.Preset.BestQuality);
                context.Cameras[0].EnableStream(RealSense.Stream.Color, 640, 480, RealSense.Format.BGR8, 60);
                context.Cameras[0].Start();
                
                Application.Run(new Capture(context.Cameras[0]));
            }
        }
        catch (RealSense.Error e)
        {
            MessageBox.Show(string.Format("RealSense error calling {0}({1}):\n  {2}\nProgram will now exit.",
                e.FailedFunction, e.FailedArgs, e.Message), "C# Capture Example", MessageBoxButtons.OK);
        }
    }
}
