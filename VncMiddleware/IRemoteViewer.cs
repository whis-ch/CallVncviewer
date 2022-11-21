using System;

namespace VncMiddleware
{
    /// <summary>
    /// vncviewer operations
    /// </summary>
    public interface IRemoteViewer
    {
        /// <summary>
        /// process main window handle
        /// </summary>
        IntPtr MianWindowHandle { get; }

        /// <summary>
        /// vncviewer connect status
        /// </summary>
        bool Connected { get; set; }

        /// <summary>
        /// use process start vncviewer.exe
        /// </summary>
        /// <param name="processName">process name,this value can be null</param>
        /// <returns>start result</returns>
        bool StartProcess(string processName = null);

        /// <summary>
        /// use process close vncviewer.exe
        /// </summary>
        void CloseProcess();

        /// <summary>
        /// set vncviewer screen scale
        /// </summary>
        /// <param name="scale">max is 200%,min is 0%</param>
        void SetScale(ushort scale);

        /// <summary>
        /// set vncviewer screen is view only,can not use mouse and keyboard
        /// </summary>
        /// <param name="viewOnly">viewonly is true,otherwise set false</param>
        void SetViewOnly(bool viewOnly);

        /// <summary>
        /// set title bar hide or show
        /// </summary>
        /// <param name="visible">visible is true title bar show,otherwise hide.</param>
        void SetTitleBarVisible(bool visible);

        /// <summary>
        /// set ini file configuration
        /// </summary>
        /// <param name="title">header</param>
        /// <param name="message"></param>
        void SetConfiguration(string title, string message);

        /// <summary>
        /// get ini file configuration info
        /// </summary>
        /// <typeparam name="T">convert type</typeparam>
        /// <param name="title">ini file key</param>
        /// <returns>value corresponding to key</returns>
        T GetConfigurationValue<T>(string title);
    }
}
