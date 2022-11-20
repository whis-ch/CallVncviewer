namespace RemoteView
{
    partial class Form1
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要修改
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            this.RemoteCtl = new VncMiddleware.RemoteControl();
            this.SuspendLayout();
            // 
            // RemoteCtl
            // 
            this.RemoteCtl.BackColor = System.Drawing.Color.Black;
            this.RemoteCtl.Dock = System.Windows.Forms.DockStyle.Fill;
            this.RemoteCtl.Font = new System.Drawing.Font("微软雅黑", 16F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Pixel, ((byte)(134)));
            this.RemoteCtl.Location = new System.Drawing.Point(0, 0);
            this.RemoteCtl.Margin = new System.Windows.Forms.Padding(0);
            this.RemoteCtl.Name = "RemoteCtl";
            this.RemoteCtl.Size = new System.Drawing.Size(998, 538);
            this.RemoteCtl.TabIndex = 0;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(998, 538);
            this.Controls.Add(this.RemoteCtl);
            this.Name = "Form1";
            this.Text = "Form1";
            this.ResumeLayout(false);

        }

        #endregion

        private VncMiddleware.RemoteControl RemoteCtl;
    }
}

