using System.IO;

namespace VncMiddleware
{
    internal class IniUtil
    {
        public static bool ExistFile(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath))
                return false;
            return File.Exists(filePath) ? true : false;
        }
    }
}
