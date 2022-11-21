using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;

namespace VncMiddleware
{
    public sealed class RemoteViewer : IRemoteViewer
    {
        #region --Field or property--
        /// <summary>
        /// ini config file path
        /// </summary>
        private string _IniFilePath { get; set; } = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "config.ini");

        ///// <summary>
        ///// process object
        ///// </summary>
        private Process _MyProcess { get; set; }

        /// <summary>
        /// process handle
        /// </summary>
        private IntPtr _ProcessHandle { get; set; }

        /// <summary>
        /// process file path
        /// </summary>
        private string _ProcessVncViewerPath { get; set; } = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "vncviewer.exe");

        /// <inheritdoc/>
        public bool Connected { get; set; } = false;

        /// <inheritdoc/>
        public IntPtr MianWindowHandle { get; set; }
        #endregion

        #region --Public method--
        /// <inheritdoc/>
        public bool StartProcess(string processName)
        {
            bool flag = false;
            string tmpProcessFileName = string.Empty;
            if (!string.IsNullOrWhiteSpace(processName) && !File.Exists(processName))
                return flag;
            if (string.IsNullOrWhiteSpace(processName))
            {
                if (!File.Exists(this._ProcessVncViewerPath))
                {
                    throw new FileNotFoundException($"this file path:{this._ProcessVncViewerPath} was not found");
                }
                tmpProcessFileName = this._ProcessVncViewerPath;
            }
            else
                tmpProcessFileName = processName;

            this.KillProcess(tmpProcessFileName);
            _MyProcess = new Process();
            _MyProcess.StartInfo.FileName = tmpProcessFileName;
            //_MyProcess.StartInfo.Arguments = arguments;
            _MyProcess.StartInfo.WorkingDirectory = "";
            _MyProcess.StartInfo.UseShellExecute = false;
            _MyProcess.StartInfo.CreateNoWindow = true;
            _MyProcess.StartInfo.RedirectStandardInput = true;
            _MyProcess.StartInfo.RedirectStandardOutput = true;
            _MyProcess.StartInfo.RedirectStandardError = true;
            _MyProcess.StartInfo.ErrorDialog = false;
            _MyProcess.ErrorDataReceived += Process_ErrorDataReceived;
            if (_MyProcess.Start())
            {
                _MyProcess.WaitForInputIdle();
                _MyProcess.EnableRaisingEvents = true;
                _MyProcess.Exited += Process_Exited;
                this._ProcessHandle = _MyProcess.Handle;
                this.MianWindowHandle = _MyProcess.MainWindowHandle;
                this.Connected = true;
                flag = true;
            }
            return flag;
        }

        /// <inheritdoc/>
        public void CloseProcess()
        {
            if (this._MyProcess != null && !this._MyProcess.HasExited)
            {
                this._MyProcess.Kill();
            }
            this._MyProcess = null;
        }

        /// <inheritdoc/>
        public void SetConfiguration(string title, string message)
        {
            if (IniUtil.ExistFile(this._IniFilePath))
            {
                string key = IniSectionKey.DicSectionkeys.Where(x => x.Value.Contains(title)).Select(y => y.Key).FirstOrDefault();
                if (!string.IsNullOrWhiteSpace(key))
                {
                    IniUtil.WriteValue(key, title, message, this._IniFilePath);
                }
            }
        }

        /// <inheritdoc/>
        public T GetConfigurationValue<T>(string title)
        {
            if (IniUtil.ExistFile(this._IniFilePath))
            {
                string key = IniSectionKey.DicSectionkeys.Where(x => x.Value.Contains(title)).Select(y => y.Key).FirstOrDefault();
                if (!string.IsNullOrWhiteSpace(key))
                {
                    return IniUtil.ReadValue<T>(key, title, this._IniFilePath);
                }
            }
            return default;
        }

        /// <inheritdoc/>
        public void SetScale(ushort scale)
        {
            if (IsWindow(this._ProcessHandle))
            {
                this.SendMessage("Key_NPer", scale.ToString());
            }
        }

        /// <inheritdoc/>
        public void SetTitleBarVisible(bool visible)
        {
            //..to do
            //this.SetConfiguration(IniSectionKey.Key_TitleBar, visible ? "1" : "0");
        }

        /// <inheritdoc/>
        public void SetViewOnly(bool viewOnly)
        {
            if (IsWindow(this._ProcessHandle))
            {
                this.SendMessage("ViewOnly", viewOnly ? "1" : "0");
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
            sendObject.ContentText = textContent;
            string content = JsonUtil.ObjectToJson(sendObject);
            byte[] arr = System.Text.Encoding.Default.GetBytes(content);
            int len = arr.Length;
            COPYDATASTRUCT cdata;
            cdata.dwData = (IntPtr)_DwDataLength;
            cdata.lpData = content;
            cdata.cData = len + 1;
            SendMessage(this._ProcessHandle, _WM_COPYDATA, 0, ref cdata);
        }

        /// <summary>
        /// kill process
        /// </summary>
        /// <param name="filePath">file path</param>
        private void KillProcess(string filePath)
        {
            Process[] processes = Process.GetProcessesByName(Path.GetFileNameWithoutExtension(filePath));
            if (processes != null && processes.Length > 0)
            {
                foreach (Process item in processes)
                {
                    item.Kill();
                }
            }
            this.CloseProcess();
        }
        #endregion

        #region --Event handler--
        private void Process_Exited(object sender, EventArgs e)
        {
            //this._MyProcess = null;
            this.Connected = false;
            this._ProcessHandle = IntPtr.Zero;
        }

        private void Process_ErrorDataReceived(object sender, DataReceivedEventArgs e)
        {
        }
        #endregion

        #region --Win32--
        private const int _DwDataLength = 1024;
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

        [DllImport("User32.dll")]
        public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
        #endregion 

        public struct SendObject
        {
            public string Title { get; set; }
            public string ContentText { get; set; }
        }
    }
}
