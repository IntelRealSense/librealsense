using System;
using System.Linq;
using System.Collections.Generic;
using System.Windows.Forms;

namespace cs_config
{
    public partial class Config : Form
    {
        public Config()
        {
            InitializeComponent();
        }

        public void SetDevice(RealSense.Device d)
        {
            device = d;

            foreach (var stream in Streams) stream.Setup(d);
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (device.IsStreaming)
            {
                try
                {
                    device.Stop();
                    label1.Text = string.Format("Stream stopped");
                    button1.Text = "Start Streaming";
                }
                catch (RealSense.Error err)
                {
                    label1.Text = string.Format("Unable to stop stream: {0}", err.Message);
                }
            }
            else
            {
                try
                {
                    foreach (var stream in Streams) stream.Apply(device);
                    device.Start();
                    label1.Text = string.Format("Stream started");
                    button1.Text = "Stop Streaming";
                }
                catch (RealSense.Error err)
                {
                    label1.Text = string.Format("Unable to start stream: {0}", err.Message);
                }
            }

            foreach (var stream in Streams) stream.Enabled = !device.IsStreaming;
        }

        private StreamConfig[] Streams { get { return new[] { depthConfig, colorConfig, infraredConfig, infrared2Config }; } }

        private RealSense.Device device;
    }
}
