using System;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace RemoteView
{
    /// <summary>
    /// Exception class processing for global exception capture
    /// </summary>
    public class GlobalException
    {
        /// <summary>
        /// Add Global Capture
        /// </summary>
        public static void AddGlobalObserve()
        {
            Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);
            Application.ThreadException += Application_ThreadException;
            AppDomain.CurrentDomain.UnhandledException += CurrentDomain_UnhandledException;
        }

        /// <summary>
        /// UI thread did not catch exception handler
        /// </summary>
        private static void Application_ThreadException(object sender, ThreadExceptionEventArgs e)
        {
            try
            {
                MessageBox.Show(ExceptionToString(e.Exception, "UI thread"), "System error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            catch (Exception ex)
            {
                string msg = ExceptionToString(ex, "UI thread processing function");
                MessageBox.Show(msg, "System error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Non ui thread did not catch exception handler
        /// </summary>
        public static void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            try
            {
                string msg;
                if (e.ExceptionObject is Exception ex)
                {
                    msg = ExceptionToString(ex, "Non ui thread");
                }
                else
                {
                    msg = $"An error has occurred！information:{e.ExceptionObject}";
                }
                MessageBox.Show(msg, "System error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            catch (Exception ex)
            {
                string msg = ExceptionToString(ex, "Non ui thread processing function");
                MessageBox.Show(msg, "System error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Extract exception information
        /// </summary>
        private static string ExceptionToString(Exception ex, string info)
        {
            StringBuilder str = new StringBuilder($"{DateTime.Now}, {info}an error has occurred！{Environment.NewLine}");
            if (ex.InnerException == null)
            {
                str.Append($"【Object name】：{ex.Source}{Environment.NewLine}");
                str.Append($"【Exception type】：{ex.GetType().Name}{Environment.NewLine}");
                str.Append($"【Description】：{ex.Message}{Environment.NewLine}");
                str.Append($"【Call stack】：{ex.StackTrace}");
            }
            else
            {
                str.Append($"【Object name】：{ex.InnerException.Source}{Environment.NewLine}");
                str.Append($"【Exception type】：{ex.InnerException.GetType().Name}{Environment.NewLine}");
                str.Append($"【Description】：{ex.InnerException.Message}{Environment.NewLine}");
                str.Append($"【Call stack】：{ex.InnerException.StackTrace}");
            }
            return str.ToString();
        }
    }
}
