using System;
using System.Diagnostics;
using System.Drawing.Printing;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using VncMiddleware;

namespace RemoteView
{
    public class RemoteView
    {
        #region --Field or property--
        private System.Timers.Timer _TmrReLink;
        private object _ObjectLock = new object();
        private IRemoteViewer _RemoteViewer { get; set; }
        private bool _ViewOnly { get; set; }
        private IPAddress _VncIpAddress { get; set; }
        private int _VncPort { get; set; }
        /// <summary>
        /// this control parent form
        /// </summary>
        public Control ParentControl { get; set; }
        #endregion

        public RemoteView()
        {
            this._RemoteViewer = new RemoteViewer();

            this._TmrReLink = new System.Timers.Timer();
            this._TmrReLink.Interval = 4000;
            this._TmrReLink.AutoReset = true;
            this._TmrReLink.Enabled = true;
            this._TmrReLink.Elapsed += TmrReLink_Elapsed;

            string vncIpAddress = this._RemoteViewer.GetConfigurationValue<string>(IniSectionKey.Key_HostDialog);
            if (!string.IsNullOrWhiteSpace(vncIpAddress))
            {
                if (IPAddress.TryParse(vncIpAddress, out IPAddress tmpVncIpAddress))
                {
                    this._VncIpAddress = tmpVncIpAddress;
                }
                else
                {
                    throw new InvalidCastException("Ini file host_dialog value format error.");
                }
            }
            else
            {
                throw new Exception("Ini file host_dialog value is null.");
            }
            this._VncPort = this._RemoteViewer.GetConfigurationValue<int>(IniSectionKey.Key_ListenPort);
        }

        private void TmrReLink_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            if (Monitor.TryEnter(this._ObjectLock))
            {
                try
                {
                    using (TcpClient tcp = new TcpClient())
                    {
                        Task task = tcp.ConnectAsync(this._VncIpAddress, this._VncPort);
                        task.Wait(1000);
                        if (task.IsCompleted && !this._RemoteViewer.Connected)
                        {
                            this.StartProcessEmbeddedCtl();
                        }
                        else if (!task.IsCompleted && this._RemoteViewer.Connected)
                        {
                            this._RemoteViewer.CloseProcess();
                        }
                    }
                }
                finally
                {
                    Monitor.Exit(this._ObjectLock);
                }
            }
        }

        /// <summary>
        /// start process and embedded controls
        /// http://t.zoukankan.com/findumars-p-5870478.html
        /// </summary>
        private void StartProcessEmbeddedCtl()
        {
            this.ParentControl.BeginInvoke(new Action(() =>
            {
                this._RemoteViewer.StartProcess();
                if (this._RemoteViewer.MianWindowHandle == IntPtr.Zero)
                    return;
                IntPtr mainHandle = this._RemoteViewer.MianWindowHandle;
                SetParent(mainHandle, this.ParentControl.Handle);

                //Thread.Sleep(100);
                //SetWindowLong(mainHandle, GWL_STYLE, GetWindowLong(mainHandle, GWL_STYLE) & ~WS_CAPTION & ~WS_THICKFRAME);
                //RECT rECT = new RECT();
                //GetWindowRect(mainHandle, ref rECT);
                //MoveWindow(mainHandle, 0, 0, rECT.Right - rECT.Left, rECT.Bottom - rECT.Top, true);
            }));
        }

        public void CloseVncProcess() { this._RemoteViewer.CloseProcess(); }
        #region --Win32--
        private const uint WS_CAPTION = 0x00C00000;
        private const uint WS_THICKFRAME = 0x00040000;
        private const int GWL_STYLE = -16;

        [DllImport("user32.dll", SetLastError = true)]
        public static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);

        [DllImport("user32.dll", EntryPoint = "ShowWindow", CharSet = CharSet.Auto)]
        public static extern int ShowWindow(IntPtr hwnd, int nCmdShow);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern int SetWindowPos(IntPtr hwnd, int hWndInsertAfter, int x, int y, int cx, int cy, int wFlags);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        public static extern int MoveWindow(IntPtr hWnd, int x, int y, int nWidth, int nHeight, bool BRePaint);

        [DllImport("user32.dll")]
        public static extern uint GetWindowLong(IntPtr hWnd, int nIndex);

        [DllImport("user32.dll")]
        public static extern int SetWindowLong(IntPtr hWnd, int nIndex, uint dwNewLong);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool GetWindowRect(IntPtr hWnd, ref RECT lpRect);

        [StructLayout(LayoutKind.Sequential)]
        public struct RECT
        {
            public int Left;
            public int Top;
            public int Right;
            public int Bottom;
        }
        #endregion
    }
}
