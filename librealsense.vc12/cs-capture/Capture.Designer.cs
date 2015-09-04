namespace cs_capture
{
    partial class Capture
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
            this.components = new System.ComponentModel.Container();
            this.colorPictureBox = new System.Windows.Forms.PictureBox();
            this.captureTimer = new System.Windows.Forms.Timer(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.colorPictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // colorPictureBox
            // 
            this.colorPictureBox.Location = new System.Drawing.Point(12, 12);
            this.colorPictureBox.MinimumSize = new System.Drawing.Size(640, 480);
            this.colorPictureBox.Name = "colorPictureBox";
            this.colorPictureBox.Size = new System.Drawing.Size(640, 480);
            this.colorPictureBox.TabIndex = 0;
            this.colorPictureBox.TabStop = false;
            // 
            // captureTimer
            // 
            this.captureTimer.Enabled = true;
            this.captureTimer.Interval = 1;
            this.captureTimer.Tick += new System.EventHandler(this.OnCaptureTick);
            // 
            // Capture
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(665, 503);
            this.Controls.Add(this.colorPictureBox);
            this.Name = "Capture";
            this.Text = "Form1";
            ((System.ComponentModel.ISupportInitialize)(this.colorPictureBox)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.PictureBox colorPictureBox;
        private System.Windows.Forms.Timer captureTimer;
    }
}

