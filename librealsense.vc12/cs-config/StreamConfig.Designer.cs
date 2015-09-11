namespace cs_config
{
    partial class StreamConfig
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.checkBox1 = new System.Windows.Forms.CheckBox();
            this.label4 = new System.Windows.Forms.Label();
            this.fpsComboBox = new System.Windows.Forms.ComboBox();
            this.fmtComboBox = new System.Windows.Forms.ComboBox();
            this.resComboBox = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // checkBox1
            // 
            this.checkBox1.AutoSize = true;
            this.checkBox1.Location = new System.Drawing.Point(3, 3);
            this.checkBox1.Name = "checkBox1";
            this.checkBox1.Size = new System.Drawing.Size(134, 17);
            this.checkBox1.TabIndex = 15;
            this.checkBox1.Text = "$$$$$ Stream Enabled";
            this.checkBox1.UseVisualStyleBackColor = true;
            this.checkBox1.CheckedChanged += new System.EventHandler(this.OnStateChanged);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(3, 108);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(109, 13);
            this.label4.TabIndex = 14;
            this.label4.Text = "$$$ x $$$ $$$$ $$Hz";
            // 
            // fpsComboBox
            // 
            this.fpsComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.fpsComboBox.FormattingEnabled = true;
            this.fpsComboBox.Location = new System.Drawing.Point(73, 78);
            this.fpsComboBox.Name = "fpsComboBox";
            this.fpsComboBox.Size = new System.Drawing.Size(121, 21);
            this.fpsComboBox.TabIndex = 13;
            this.fpsComboBox.SelectedIndexChanged += new System.EventHandler(this.OnStateChanged);
            // 
            // fmtComboBox
            // 
            this.fmtComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.fmtComboBox.FormattingEnabled = true;
            this.fmtComboBox.Location = new System.Drawing.Point(73, 51);
            this.fmtComboBox.Name = "fmtComboBox";
            this.fmtComboBox.Size = new System.Drawing.Size(121, 21);
            this.fmtComboBox.TabIndex = 12;
            this.fmtComboBox.SelectedIndexChanged += new System.EventHandler(this.OnStateChanged);
            // 
            // resComboBox
            // 
            this.resComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.resComboBox.FormattingEnabled = true;
            this.resComboBox.Location = new System.Drawing.Point(73, 24);
            this.resComboBox.Name = "resComboBox";
            this.resComboBox.Size = new System.Drawing.Size(121, 21);
            this.resComboBox.TabIndex = 11;
            this.resComboBox.SelectedIndexChanged += new System.EventHandler(this.OnStateChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(3, 81);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(54, 13);
            this.label3.TabIndex = 10;
            this.label3.Text = "Framerate";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(3, 54);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(64, 13);
            this.label2.TabIndex = 9;
            this.label2.Text = "Pixel Format";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 27);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(57, 13);
            this.label1.TabIndex = 8;
            this.label1.Text = "Resolution";
            // 
            // StreamConfig
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.checkBox1);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.fpsComboBox);
            this.Controls.Add(this.fmtComboBox);
            this.Controls.Add(this.resComboBox);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Name = "StreamConfig";
            this.Size = new System.Drawing.Size(198, 126);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.CheckBox checkBox1;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.ComboBox fpsComboBox;
        private System.Windows.Forms.ComboBox fmtComboBox;
        private System.Windows.Forms.ComboBox resComboBox;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
    }
}
