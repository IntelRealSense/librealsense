using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace cs_config
{
    public partial class StreamConfig : UserControl
    {
        public RealSense.Stream Stream { get; set; }

        public StreamConfig()
        {
            InitializeComponent();
        }

        public struct Resolution : IEquatable<Resolution>
        {
            public int Width, Height;
            public bool Equals(Resolution res) { return res.Width == Width && res.Height == Height; }
            public override string ToString() { return string.Format("{0} x {1}", Width, Height); }
        }

        public void Setup(RealSense.Device device)
        {
            if(device.GetStreamModeCount(Stream) == 0)
            {
                this.Visible = false;
                return;
            }

            var res = new List<Resolution>();
            var fmt = new List<RealSense.Format>();
            var fps = new List<int>();

            checkBox1.Text = string.Format("{0} Stream Enabled", Stream);
            
            for (int i = 0, n = device.GetStreamModeCount(Stream); i < n; i++)
            {
                int width, height, framerate; RealSense.Format format;
                device.GetStreamMode(Stream, i, out width, out height, out format, out framerate);
                res.Add(new Resolution { Width = width, Height = height });
                fmt.Add(format);
                fps.Add(framerate);
            }

            resComboBox.DataSource = res.Distinct().ToList();
            fmtComboBox.DataSource = fmt.Distinct().ToList();
            fpsComboBox.DataSource = fps.Distinct().OrderByDescending(x => x).ToList();
        }

        public void Apply(RealSense.Device device)
        {
            if (device.GetStreamModeCount(Stream) == 0)
            {
                return;
            }

            if (checkBox1.Checked)
            {
                var res = (Resolution)resComboBox.SelectedItem;
                var fmt = (RealSense.Format)fmtComboBox.SelectedItem;
                var fps = (int)fpsComboBox.SelectedItem;
                device.EnableStream(Stream, res.Width, res.Height, fmt, fps);
            }
            else
            {
                device.DisableStream(Stream);
            }
        }

        private void OnStateChanged(object sender, EventArgs e)
        {
            label1.Enabled = checkBox1.Checked;
            label2.Enabled = checkBox1.Checked;
            label3.Enabled = checkBox1.Checked;
            resComboBox.Enabled = checkBox1.Checked;
            fmtComboBox.Enabled = checkBox1.Checked;
            fpsComboBox.Enabled = checkBox1.Checked;

            if (checkBox1.Checked)
            {
                try
                {
                    var res = (Resolution)resComboBox.SelectedItem;
                    var fmt = (RealSense.Format)fmtComboBox.SelectedItem;
                    var fps = (int)fpsComboBox.SelectedItem;
                    label4.Text = string.Format("{0} {1} {2}Hz", res, fmt, fps);
                }
                catch (System.Exception) { }
            }
            else label4.Text = string.Empty;
        }
    }
}
