using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Windows.Forms;

namespace VncMiddleware
{
    public sealed class IniSectionKey
    {
        #region --SectionKey--
        public const string Section_Global = "GLOBAL";
        public const string Key_HostDialog = "host_dialog";
        public const string Key_Proxyhost = "proxyhost";
        public const string Key_FUseProxy = "m_fUseProxy";
        public const string Key_WIndowsSize = "windowsSize";

        public const string Section_LisenMode = "LISTEN_MODE";
        public const string Key_ListenPort = "listenport";

        public const string Section_Encoders = "ENCODERS";
        public const string Key_AutoDetect = "autoDetect";
        public const string Key_EncodingTypes = "EncodingTypes";
        public const string Key_Use8Bit = "Use8Bit";
        public const string Key_UseCompressLevel = "useCompressLevel";
        public const string Key_CompressLevel = "compressLevel";
        public const string Key_EnableJpegCompression = "enableJpegCompression";
        public const string Key_JpegQualityLevel = "jpegQualityLevel";
        public const string Key_PreemptiveUpdates = "preemptiveUpdates";
        public const string Key_UseCopyRectEncoding = "useCopyRectEncoding";
        public const string Key_FEnableCache = "fEnableCache";
        public const string Key_FEnableZstd = "fEnableZstd";

        public const string Section_MouseAndKeyboard = "MOUSE_AND_KEYBOARD";
        public const string Key_ViewOnly = "ViewOnly";
        public const string Key_Emul3Buttons = "Emul3Buttons";
        public const string Key_JapKeyboard = "JapKeyboard";
        public const string Key_GiiEnable = "giiEnable";
        public const string Key_RemoteCursor = "RemoteCursor";
        public const string Key_IgnoreRemoteCursor = "IgnoreRemoteCursor";
        public const string Key_SwapMouse = "SwapMouse";
        public const string Key_ThrottleMouse = "throttleMouse";
        public const string Key_NoHotKeys = "NoHotKeys";

        public const string Section_Display = "DISPLAY";
        public const string Key_ShowToolbar = "ShowToolbar";
        public const string Key_FAutoScaling = "fAutoScaling";
        public const string Key_FAutoScalingEven = "fAutoScalingEven";
        public const string Key_NPer = "nPer";
        public const string Key_NServerScale = "nServerScale";
        public const string Key_FullScreen = "FullScreen";
        public const string Key_SavePos = "SavePos";
        public const string Key_SaveSize = "SaveSize";
        public const string Key_Directx = "Directx";
        public const string Key_AllowMonitorSpanning = "allowMonitorSpanning";
        public const string Key_ChangeServerRes = "changeServerRes";
        public const string Key_ExtendDisplay = "extendDisplay";
        public const string Key_ShowExtend = "showExtend";
        public const string Key_UseVirt = "use_virt";
        public const string Key_UseAllMonitors = "useAllMonitors";

        public const string Section_QuickEncoder = "QUICK_ENCODER";
        public const string Key_Quickoption = "m_quickoption";

        public const string Section_Security = "SECURITY";
        public const string Key_Shared = "Shared";
        public const string Key_DisableClipboard = "DisableClipboard";
        public const string Key_UseEncryption = "useEncryption";
        public const string Key_FUseDSMPlugin = "fUseDSMPlugin";
        public const string Key_SzDSMPluginFilename = "szDSMPluginFilename";
        public const string Key_FRequireEncryption = "fRequireEncryption";
        public const string Key_AllowUntrustedServers = "AllowUntrustedServers";
        public const string Key_FAutoAcceptIncoming = "fAutoAcceptIncoming";
        public const string Key_FAutoAcceptNoDSM = "fAutoAcceptNoDSM";
        public const string Key_Restricted = "restricted";
        public const string Key_InfoMsg = "InfoMsg";

        public const string Section_Misc = "MISC";
        public const string Key_Prefix = "prefix";
        public const string Key_Reconnectcounter = "reconnectcounter";
        public const string Key_AutoReconnect = "autoReconnect";
        public const string Key_FTTimeout = "FTTimeout";
        public const string Key_Disable_sponsor = "g_disable_sponsor";
        public const string Key_NoStatus = "NoStatus";
        public const string Key_ImageFormat = "imageFormat";
        #endregion 

        public static Dictionary<string, List<string>> DicSectionkeys { get; set; }

        static IniSectionKey()
        {
            DicSectionkeys = new Dictionary<string, List<string>>
            {
                { Section_Global, new List<string>() { Key_HostDialog, Key_Proxyhost, Key_FUseProxy, Key_WIndowsSize } },
                { Section_LisenMode, new List<string>() { Key_ListenPort } },
                {
                    Section_Encoders,
                    new List<string>() {
                                                                        Key_AutoDetect,
                                                                        Key_EncodingTypes,
                                                                        Key_Use8Bit,
                                                                        Key_UseCompressLevel,
                                                                        Key_CompressLevel,
                                                                        Key_EnableJpegCompression,
                                                                        Key_JpegQualityLevel,
                                                                        Key_PreemptiveUpdates,
                                                                        Key_UseCopyRectEncoding,
                                                                        Key_FEnableCache,
                                                                        Key_FEnableZstd }
                },
                {
                    Section_MouseAndKeyboard,
                    new List<string>() {
                                                                        Key_ViewOnly,
                                                                        Key_Emul3Buttons,
                                                                        Key_JapKeyboard,
                                                                        Key_GiiEnable,
                                                                        Key_RemoteCursor,
                                                                        Key_IgnoreRemoteCursor,
                                                                        Key_SwapMouse,
                                                                        Key_ThrottleMouse,
                                                                        Key_NoHotKeys,}
                },
                {
                    Section_Display,
                    new List<string>() {
                                                                        Key_ShowToolbar,
                                                                        Key_FAutoScaling,
                                                                        Key_FAutoScalingEven,
                                                                        Key_NPer,
                                                                        Key_NServerScale,
                                                                        Key_FullScreen,
                                                                        Key_SavePos,
                                                                        Key_SaveSize,
                                                                        Key_Directx,
                                                                        Key_AllowMonitorSpanning,
                                                                        Key_ChangeServerRes,
                                                                        Key_ExtendDisplay,
                                                                        Key_ShowExtend,
                                                                        Key_UseVirt,
                                                                        Key_UseAllMonitors}
                },
                { Section_QuickEncoder, new List<string>() { Key_Quickoption } },
                {
                    Section_Security,
                    new List<string>() {
                                                                        Key_Shared,
                                                                        Key_DisableClipboard,
                                                                        Key_UseEncryption,
                                                                        Key_FUseDSMPlugin,
                                                                        Key_SzDSMPluginFilename,
                                                                        Key_FRequireEncryption,
                                                                        Key_AllowUntrustedServers,
                                                                        Key_FAutoAcceptIncoming,
                                                                        Key_FAutoAcceptNoDSM,
                                                                        Key_Restricted,
                                                                        Key_InfoMsg}
                },
                {
                    Section_Misc,
                    new List<string>() {
                                                                        Key_Prefix,
                                                                        Key_Reconnectcounter,
                                                                        Key_AutoReconnect,
                                                                        Key_FTTimeout,
                                                                        Key_Disable_sponsor,
                                                                        Key_NoStatus,
                                                                        Key_ImageFormat}
                }
            };
        }
    }
}
