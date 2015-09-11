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
            this.SuspendLayout();
            // 
            // depthConfig
            // 
            this.depthConfig.AutoSize = true;
            this.depthConfig.Location = new System.Drawing.Point(12, 12);
            this.depthConfig.Name = "depthConfig";
            this.depthConfig.Size = new System.Drawing.Size(197, 128);
            this.depthConfig.TabIndex = 0;
            // 
            // colorConfig
            // 
            this.colorConfig.AutoSize = true;
            this.colorConfig.Location = new System.Drawing.Point(215, 12);
            this.colorConfig.Name = "colorConfig";
            this.colorConfig.Size = new System.Drawing.Size(197, 128);
            this.colorConfig.TabIndex = 1;
            // 
            // infraredConfig
            // 
            this.infraredConfig.AutoSize = true;
            this.infraredConfig.Location = new System.Drawing.Point(419, 12);
            this.infraredConfig.Name = "infraredConfig";
            this.infraredConfig.Size = new System.Drawing.Size(197, 128);
            this.infraredConfig.TabIndex = 2;
            // 
            // infrared2Config
            // 
            this.infrared2Config.AutoSize = true;
            this.infrared2Config.Location = new System.Drawing.Point(622, 12);
            this.infrared2Config.Name = "infrared2Config";
            this.infrared2Config.Size = new System.Drawing.Size(197, 128);
            this.infrared2Config.TabIndex = 3;
            // 
            // Config
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(825, 588);
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
    }
}

