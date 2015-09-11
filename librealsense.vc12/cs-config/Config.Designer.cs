namespace cs_config
{
    partial class Config
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.depthConfig = new cs_config.StreamConfig();
            this.colorConfig = new cs_config.StreamConfig();
            this.infraredConfig = new cs_config.StreamConfig();
            this.infrared2Config = new cs_config.StreamConfig();
            this.button1 = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // depthConfig
            // 
            this.depthConfig.AutoSize = true;
            this.depthConfig.Location = new System.Drawing.Point(12, 12);
            this.depthConfig.Name = "depthConfig";
            this.depthConfig.Size = new System.Drawing.Size(197, 128);
            this.depthConfig.TabIndex = 0;
            this.depthConfig.Stream = RealSense.Stream.Depth;
            // 
            // colorConfig
            // 
            this.colorConfig.AutoSize = true;
            this.colorConfig.Location = new System.Drawing.Point(215, 12);
            this.colorConfig.Name = "colorConfig";
            this.colorConfig.Size = new System.Drawing.Size(197, 128);
            this.colorConfig.TabIndex = 1;
            this.colorConfig.Stream = RealSense.Stream.Color;
            // 
            // infraredConfig
            // 
            this.infraredConfig.AutoSize = true;
            this.infraredConfig.Location = new System.Drawing.Point(419, 12);
            this.infraredConfig.Name = "infraredConfig";
            this.infraredConfig.Size = new System.Drawing.Size(197, 128);
            this.infraredConfig.TabIndex = 2;
            this.infraredConfig.Stream = RealSense.Stream.Infrared;
            // 
            // infrared2Config
            // 
            this.infrared2Config.AutoSize = true;
            this.infrared2Config.Location = new System.Drawing.Point(622, 12);
            this.infrared2Config.Name = "infrared2Config";
            this.infrared2Config.Size = new System.Drawing.Size(197, 128);
            this.infrared2Config.TabIndex = 3;
            this.infrared2Config.Stream = RealSense.Stream.Infrared2;
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(12, 147);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(110, 23);
            this.button1.TabIndex = 4;
            this.button1.Text = "Start Streaming";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(128, 152);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(72, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Not streaming";
            // 
            // Config
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(825, 588);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.infrared2Config);
            this.Controls.Add(this.infraredConfig);
            this.Controls.Add(this.colorConfig);
            this.Controls.Add(this.depthConfig);
            this.Name = "Config";
            this.Text = "Form1";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private StreamConfig depthConfig;
        private StreamConfig colorConfig;
        private StreamConfig infraredConfig;
        private StreamConfig infrared2Config;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Label label1;
    }
}

