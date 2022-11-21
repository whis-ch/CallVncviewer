using System.ComponentModel;
using System.Diagnostics;
using System.Threading;
using System.Windows.Forms;

namespace RemoteView
{
    public partial class Form1 : Form
    {
        private RemoteView _RemoteView { get; set; }
        public Form1()
        {
            InitializeComponent();
            this.Load += Form1_Load;
        }

        private void Form1_Load(object sender, System.EventArgs e)
        {
            this._RemoteView = new RemoteView();
            this._RemoteView.ParentControl = this.panel1;
            this.WindowState = FormWindowState.Maximized;
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            this._RemoteView.CloseVncProcess();
            base.OnClosing(e);
        }
    }
}
