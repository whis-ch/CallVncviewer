using System;
using System.Diagnostics;
using System.IO;
using System.Net.NetworkInformation;
using System.Reflection.Emit;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using static System.Windows.Forms.AxHost;

namespace VncMiddleware
{
    public sealed class RemoteViewer : IRemoteViewer
    {
        #region --Field or property--
        /// <summary>
        /// ini config file path
        /// </summary>
        private string _IniFilePath { get; set; } = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "config.ini");

        private Process _MyProcess { get; set; }

        /// <summary>
        /// Process handle
        /// </summary>
        private IntPtr _ProcessHandle { get; set; }

        /// <summary>
        /// Process file path
        /// </summary>
        private string _ProcessVncViewerPath { get; set; } = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "vncviwer.exe");

        /// <inheritdoc/>
        public bool Connected { get; set; } = false;
        #endregion

        #region --Public method--
        /// <inheritdoc/>
        public bool StartProcess(string processName)
        {
            string tmpProcessFileName = string.Empty;
            if (!string.IsNullOrWhiteSpace(processName) && !File.Exists(processName))
                return false;
            if (string.IsNullOrWhiteSpace(processName))
                tmpProcessFileName = this._ProcessVncViewerPath;
            else
                tmpProcessFileName = processName;
            this._MyProcess = new Process();
            this._MyProcess.StartInfo.FileName = tmpProcessFileName;
            //this._MyProcess.StartInfo.Arguments = arguments;
            this._MyProcess.StartInfo.WorkingDirectory = "";
            this._MyProcess.StartInfo.UseShellExecute = false;
            this._MyProcess.StartInfo.CreateNoWindow = true;
            this._MyProcess.StartInfo.RedirectStandardInput = true;
            this._MyProcess.StartInfo.RedirectStandardOutput = true;
            this._MyProcess.StartInfo.RedirectStandardError = true;
            this._MyProcess.StartInfo.ErrorDialog = false;
            this._MyProcess.ErrorDataReceived += Process_ErrorDataReceived;
            this._MyProcess.EnableRaisingEvents = true;
            this._MyProcess.Exited += Process_Exited;
            if (this._MyProcess.Start())
            {
                this._ProcessHandle = this._MyProcess.Handle;
            }
            return true;
        }

        /// <inheritdoc/>
        public void CloseProcess()
        {
            this._MyProcess?.Kill();
        }

        /// <inheritdoc/>
        public void SetConfiguration(string title, string message)
        {
            if (IniUtil.ExistFile(this._IniFilePath))
            {
                //IniUtil
            }
        }

        /// <inheritdoc/>
        public void SetScale(ushort scale)
        {
            if (IsWindow(this._ProcessHandle))
            {
                this.SendMessage("scale", scale.ToString());
            }
        }

        /// <inheritdoc/>
        public void SetTitleBarVisible(bool visible)
        {
            //IniUtil
        }

        /// <inheritdoc/>
        public void SetViewOnly(bool viewOnly)
        {
            if (IsWindow(this._ProcessHandle))
            {
                this.SendMessage("viewonly", viewOnly ? "1" : "0");
            }
        }
        #endregion

        #region --Private method--
        /// <summary>
        /// send message to process
        /// </summary>
        /// <param name="title">message title</param>
        /// <param name="textContent">message content</param>
        private void SendMessage(string title, string textContent)
        {
            SendObject sendObject = new SendObject();
            sendObject.Title = title;
            sendObject.Message = textContent;
            string content = JsonUtil.ObjectToJson(sendObject);
            byte[] arr = System.Text.Encoding.Default.GetBytes(content);
            int len = arr.Length;
            COPYDATASTRUCT cdata;
            cdata.dwData = (IntPtr)_DwDataLength;
            cdata.lpData = content;
            cdata.cData = len + 1;
            SendMessage(this._ProcessHandle, _WM_COPYDATA, 0, ref cdata);
        }
        #endregion

        #region --Event handler--
        private void Process_Exited(object sender, EventArgs e)
        {
            this._MyProcess = null;
            this.Connected = false;
            this._ProcessHandle = IntPtr.Zero;
        }

        private void Process_ErrorDataReceived(object sender, DataReceivedEventArgs e)
        {
        }
        #endregion

        #region --Win32--
        private const int _DwDataLength = 1000;
        private const int _WM_COPYDATA = 0x004A;
        public struct COPYDATASTRUCT
        {
            public IntPtr dwData;
            public int cData;
            [MarshalAs(UnmanagedType.LPStr)]
            public string lpData;
        }

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        public static extern IntPtr SendMessage(IntPtr hWnd, int Msg, int wParam, ref COPYDATASTRUCT IParam);

        [DllImport("User32.dll")]
        public static extern bool IsWindow(IntPtr hWnd);
        #endregion 

        public struct SendObject
        {
            public string Title { get; set; }
            public string Message { get; set; }
        }
    }
}
