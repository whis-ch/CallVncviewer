using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace VncMiddleware
{
    internal class IniUtil
    {
        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int GetPrivateProfileString(string lpAppName, string lpKeyName, string lpDefault, StringBuilder lpReturnedString, int nSize, string lpFileName);

        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool WritePrivateProfileString(string lpAppName, string lpKeyName, string lpString, string lpFileName);

        /// <summary>
        /// judge whether the file exists
        /// </summary>
        /// <param name="filePath">file path</param>
        /// <returns>exist result</returns>
        public static bool ExistFile(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath))
                return false;
            return File.Exists(filePath) ? true : false;
        }

        /// <summary>
        /// read the value of the key in the section
        /// </summary>
        /// <param name="section">section name</param>
        /// <param name="key">key</param>
        /// <param name="iniFilePath">the path of the ini configuration file plus the ini file name</param>
        /// <param name="noText">the default value returned when the specified entry is not found, which corresponds to the def parameter of the API function. Its value is specified by the user. when no specific value is found in the configuration file, the value of noText is used instead. can be empty</param>
        /// <param name="capacityLength">buffer size, 1024 by default</param>
        /// <returns>read results</returns>
        /// <exception cref="ArgumentNullException">the input parameter is null</exception>
        /// <exception cref="FileNotFoundException">unable to find configuration file</exception>
        public static T ReadValue<T>(string section, string key, string iniFilePath, int capacityLength = 1024, string noText = null)
        {
            if (!string.IsNullOrWhiteSpace(iniFilePath) && File.Exists(iniFilePath))
            {
                if (string.IsNullOrWhiteSpace(section))
                    throw new ArgumentNullException("Section argument value is null.");
                if (string.IsNullOrWhiteSpace(key))
                    throw new ArgumentNullException("Key argument value is null.");
                StringBuilder temp = new StringBuilder(capacityLength);
                GetPrivateProfileString(section, key, noText, temp, capacityLength, iniFilePath);
                if (temp.Length > 0)
                {
                    return ToObject<T>(temp.ToString());
                }
                return default;
            }
            else
                throw new FileNotFoundException("File could not be found.");
        }

        /// <summary>
        /// write the corresponding value of the key in the section
        /// </summary>
        /// <param name="section">section name</param>
        /// <param name="key">key</param>
        /// <param name="value">Value type or string type</param>
        /// <param name="iniFilePath">file path</param>
        /// <returns>write result</returns>
        /// <exception cref="ArgumentNullException">the input parameter is null</exception>
        /// <exception cref="FileNotFoundException">unable to find configuration file</exception>
        public static bool WriteValue<T>(string section, string key, T value, string iniFilePath)
        {
            if (!string.IsNullOrWhiteSpace(iniFilePath) && File.Exists(iniFilePath))
            {
                if (string.IsNullOrWhiteSpace(section))
                    throw new ArgumentNullException("Section argument value is null.");
                if (string.IsNullOrWhiteSpace(key))
                    throw new ArgumentNullException("Key argument value is null.");
                return WritePrivateProfileString(section, key, value != null ? value.ToString() : null, iniFilePath);
            }
            else
                throw new FileNotFoundException("File could not be found.");
        }

        /// <summary>
        /// string to T type
        /// </summary>
        /// <typeparam name="T">generic</typeparam>
        /// <param name="s">string</param>
        /// <returns>generic result</returns>
        /// <exception cref="InvalidCastException">this conversion is not supported. -or- value is null and conversionType is a value type. -or- value does not implement the System.IConvertible interface.</exception>
        /// <exception cref="FormatException">value is not in a format recognized by conversionType.</exception>
        /// <exception cref="OverflowException">vvalue represents a number that is out of the range of conversionType.</exception>
        /// <exception cref="ArgumentNullException">conversionType is null.</exception>
        private static T ToObject<T>(string s) => (T)Convert.ChangeType(s, typeof(T));
    }
}
