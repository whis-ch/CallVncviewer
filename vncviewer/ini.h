#pragma once
#ifndef INI_H_
#define INI_H_
#include <string>
#include <map>
#include<fstream>
#include <windows.h>
#include <WinUser.h>
#include <winnt.h>
#include "json.hpp"
#include <sstream>
#pragma warning(disable:4996)
#pragma comment(lib, "version.lib")


#define SWC_GLOBAL "GLOBAL"
#define SWC_DIALOG "host_dialog"
#define SWC_HOST "proxyhost"
#define SWC_USEPROXY "m_fUseProxy"
#define SWC_WINDOWSSIZE "windowsSize"
#define SWC_USETITALBAR "useTitalBar"

#define SWC_LISTEN "LISTEN_MODE"
#define SWC_LISTENPORT "listenport"

#define SWC_ENCODER "ENCODERS"
#define SWC_AUTODETECT "autoDetect"
#define SWC_ENCTYPE "EncodingTypes"
#define SWC_USE8B "Use8Bit"
#define SWC_USECOMPRESSLEVEL "useCompressLevel"
#define SWC_COMPRESSLEVEL "compressLevel"
#define SWC_ENABLEJPEG "enableJpegCompression"
#define SWC_JPEGLEVEL "jpegQualityLevel"
#define SWC_PREEMPTIVEUPDATE "preemptiveUpdates"
#define SWC_USECOPYRECTENC "useCopyRectEncoding"
#define SWC_ENABLECACHE "fEnableCache"
#define SWC_ENABLEZTSD "fEnableZstd"


#define SWC_MOUSE_AND_KEYBOARD "MOUSE_AND_KEYBOARD"
#define SWC_VIEWONLY "ViewOnly"
#define SWC_EMUL3BUTTON "Emul3Buttons"
#define SWC_JAPKEYBOARD "JapKeyboard"
#define SWC_GIIENABLE "giiEnable"
#define SWC_REMOTECURSOR "RemoteCursor"
#define SWC_IGNOREREMOTE "IgnoreRemoteCursor"
#define SWC_SWAPMOUSE "SwapMouse"
#define SWC_THROTTLEMOUSE "throttleMouse"
#define SWC_NOHOTKEY "NoHotKeys"

#define SWC_DISPLAY "DISPLAY"
#define SWC_SHOWTOOLBAR "ShowToolbar"
#define SWC_AUTOSCALING "fAutoScaling"
#define SWC_AUTOSCALINGEVEN "fAutoScalingEven"
#define SWC_PER "nPer"
#define SWC_SERVERSCALE "nServerScale"
#define SWC_FULLSCREEN "FullScreen"
#define SWC_SAVEPOS "SavePos"
#define SWC_SAVESIZE "SaveSize"
#define SWC_DIRECTX "Directx"
#define SWC_ALLOWMONITOR "allowMonitorSpanning"
#define SWC_CHANGESERVERRES "changeServerRes"
#define SWC_EXTENDDISPLAY "extendDisplay"
#define SWC_SHOWEXTEND "showExtend"
#define SWC_USERVIRT "use_virt"
#define SWC_USEALLMONITOR "useAllMonitors"

#define SWC_QUICK_ENCODER "QUICK_ENCODER"
#define SWC_QUICKOPTION "m_quickoption"

#define SWC_SECURITY "SECURITY"
#define SWC_SHAERD "Shared"
#define SWC_DISABLECLIPBOARD "DisableClipboard"
#define SWC_USEENCRYPTION "useEncryption"
#define SWC_USEDSMPLUGIN "fUseDSMPlugin"
#define SWC_DSMPLUGINFILENAME "szDSMPluginFilename"
#define SWC_REQUIREENCRYPTION "fRequireEncryption"
#define SWC_ALLOWUNTRUSTEDSERVER "AllowUntrustedServers"
#define SWC_AUTOACCEPTINCOMING "fAutoAcceptIncoming"
#define SWC_AUTOACCEPTNODSM "fAutoAcceptNoDSM"
#define SWC_RESTRICT "restricted"
#define SWC_INFOMSG "InfoMsg"

#define SWC_MISC "MISC"
#define SWC_PREFIX "prefix"
#define SWC_RECONNECTCOUNTER "reconnectcounter"
#define SWC_AUTORECONNECT "autoReconnect"
#define SWC_FTTIMEOUT "FTTimeout"
#define SWC_DISABLESPONSOR "g_disable_sponsor"
#define SWC_NOSTATUS "NoStatus"
#define SWC_IMAGEFORMAT "imageFormat"

namespace ini
{
	class iniReader
	{
	public:
		iniReader()
		{
		}
		~iniReader()
		{
		}
		//filename 文件绝对路径
		bool ReadConfig(const std::string& filename)
		{
			settings_.clear();
			std::ifstream infile(filename.c_str());//构造默认调用open,所以可以不调用open
			//std::ifstream infile;
			//infile.open(filename.c_str());
			//bool ret = infile.is_open()
			if (!infile) {
				return false;
			}
			std::string line, key, value, section;
			std::map<std::string, std::string> k_v;
			std::map<std::string, std::map<std::string, std::string> >::iterator it;
			while (getline(infile, line))
			{
				if (AnalyseLine(line, section, key, value))
				{
					it = settings_.find(section);
					if (it != settings_.end())
					{
						k_v[key] = value;
						it->second = k_v;
					}
					else
					{
						k_v.clear();
						settings_.insert(std::make_pair(section, k_v));
					}
				}
				key.clear();
				value.clear();
			}
			infile.close();
			return true;
		}
		/*     section：模块名
		 *	   item：成员名称
		 *	   default_value：默认值 默认置空
		 *	   返回值（std::string）：返回成员所对应的值
		 */
		std::string ReadString(const char* section, const char* item, const char* default_value)
		{
			std::string tmp_s(section);
			std::string tmp_i(item);
			std::string def(default_value);
			std::map<std::string, std::string> k_v;
			std::map<std::string, std::string>::iterator it_item;
			std::map<std::string, std::map<std::string, std::string> >::iterator it;
			it = settings_.find(tmp_s);
			if (it == settings_.end())
			{
				return def;
			}
			k_v = it->second;
			it_item = k_v.find(tmp_i);
			if (it_item == k_v.end())
			{
				return def;
			}
			return it_item->second;
		}

		/*     section：模块名
		 *	   item：成员名称
		 *	   default_value：默认值 默认置空
		 *	   返回值（int）：返回成员所对应的值
		 */
		int ReadInt(const char* section, const char* item, const int& default_value)
		{
			std::string tmp_s(section);
			std::string tmp_i(item);
			std::map<std::string, std::string> k_v;
			std::map<std::string, std::string>::iterator it_item;
			std::map<std::string, std::map<std::string, std::string> >::iterator it;
			it = settings_.find(tmp_s);
			if (it == settings_.end())
			{
				return default_value;
			}
			k_v = it->second;
			it_item = k_v.find(tmp_i);
			if (it_item == k_v.end())
			{
				return default_value;
			}
			return atoi(it_item->second.c_str());
		}
		/*     section：模块名
		 *	   item：成员名称
		 *	   default_value：默认值 默认置空
		 *	   返回值（float）：返回成员所对应的值
		 */

		float ReadFloat(const char* section, const char* item, const float& default_value)
		{
			std::string tmp_s(section);
			std::string tmp_i(item);
			std::map<std::string, std::string> k_v;
			std::map<std::string, std::string>::iterator it_item;
			std::map<std::string, std::map<std::string, std::string> >::iterator it;
			it = settings_.find(tmp_s);
			if (it == settings_.end())
			{
				return default_value;
			}
			k_v = it->second;
			it_item = k_v.find(tmp_i);
			if (it_item == k_v.end())
			{
				return default_value;
			}
			return atof(it_item->second.c_str());
		}

	private:
		bool IsSpace(char c)
		{
			if (' ' == c || '\t' == c)
				return true;
			return false;
		}

		bool IsCommentChar(char c)
		{
			switch (c)
			{
			case '#':
				return true;
			default:
				return false;
			}
		}
		void Trim(std::string& str)
		{
			if (str.empty())
			{
				return;
			}
			int i, start_pos, end_pos;
			for (i = 0; i < str.size(); ++i) {
				if (!IsSpace(str[i])) {
					break;
				}
			}
			if (i == str.size())
			{
				str = "";
				return;
			}
			start_pos = i;
			for (i = str.size() - 1; i >= 0; --i) {
				if (!IsSpace(str[i])) {
					break;
				}
			}
			end_pos = i;
			str = str.substr(start_pos, end_pos - start_pos + 1);
		}
		bool AnalyseLine(const std::string& line, std::string& section, std::string& key, std::string& value)
		{
			if (line.empty())
				return false;
			int start_pos = 0, end_pos = line.size() - 1, pos, s_startpos, s_endpos;
			if ((pos = line.find("#")) != -1)
			{
				if (0 == pos)
				{
					return false;
				}
				end_pos = pos - 1;
			}
			if (((s_startpos = line.find("[")) != -1) && ((s_endpos = line.find("]"))) != -1)
			{
				section = line.substr(s_startpos + 1, s_endpos - 1);
				return true;
			}
			std::string new_line = line.substr(start_pos, start_pos + 1 - end_pos);
			if ((pos = new_line.find('=')) == -1)
				return false;
			key = new_line.substr(0, pos);
			value = new_line.substr(pos + 1, end_pos + 1 - (pos + 1));
			Trim(key);
			if (key.empty()) {
				return false;
			}
			Trim(value);
			if ((pos = value.find("\r")) > 0)
			{
				value.replace(pos, 1, "");
			}
			if ((pos = value.find("\n")) > 0)
			{
				value.replace(pos, 1, "");
			}
			return true;
		}
	private:
		std::map<std::string, std::map<std::string, std::string> >settings_;
	};
	class IniWriter
	{
	public:
		IniWriter(const char* szFileName)
		{
			memset(m_szFileName, 0x00, 255);
			memcpy(m_szFileName, szFileName, strlen(szFileName));
		}
		void WriteInteger(const char* szSection, const char* szKey, int iValue)
		{
			char szValue[255];
			sprintf(szValue, "%d", iValue);
			WritePrivateProfileStringA(szSection, szKey, szValue, m_szFileName);
		}
		void WriteFloat(const char* szSection, const char* szKey, float fltValue)
		{
			char szValue[255];
			sprintf(szValue, "%f", fltValue);
			WritePrivateProfileStringA(szSection, szKey, szValue, m_szFileName);
		}
		void WriteString(const char* szSection, const char* szKey, const char* szValue)
		{
			WritePrivateProfileStringA(szSection, szKey, szValue, m_szFileName);
		}
	private:
		char m_szFileName[255];
	};

}

namespace communication
{
	struct tagMESSAGE
	{
		char szMsg[1024];
	};
	static const ULONG_PTR CUSTOM_TYPE = 1024;// 定义消息常量
	class communicationToOthers
	{
	public:
		communicationToOthers()
		{
			/*params.insert(std::make_pair(SWC_DIALOG, SWC_GLOBAL));
			params.insert(std::make_pair(SWC_HOST, SWC_GLOBAL));
			params.insert(std::make_pair(SWC_USEPROXY, SWC_GLOBAL));
			params.insert(std::make_pair(SWC_LISTENPORT, SWC_LISTEN));
			params.insert(std::make_pair(SWC_AUTODETECT, SWC_ENCODER));
			params.insert(std::make_pair(SWC_ENCTYPE, SWC_ENCODER));
			params.insert(std::make_pair(SWC_USE8B, SWC_ENCODER));
			params.insert(std::make_pair(SWC_USECOMPRESSLEVEL, SWC_ENCODER));
			params.insert(std::make_pair(SWC_COMPRESSLEVEL, SWC_ENCODER));
			params.insert(std::make_pair(SWC_ENABLEJPEG, SWC_ENCODER));
			params.insert(std::make_pair(SWC_JPEGLEVEL, SWC_ENCODER));
			params.insert(std::make_pair(SWC_PREEMPTIVEUPDATE, SWC_ENCODER));
			params.insert(std::make_pair(SWC_USECOPYRECTENC, SWC_ENCODER));
			params.insert(std::make_pair(SWC_ENABLECACHE, SWC_ENCODER));
			params.insert(std::make_pair(SWC_ENABLEZTSD, SWC_ENCODER));
			params.insert(std::make_pair(SWC_VIEWONLY, SWC_MOUSE_AND_KEYBOARD));
			params.insert(std::make_pair(SWC_EMUL3BUTTON, SWC_MOUSE_AND_KEYBOARD));
			params.insert(std::make_pair(SWC_JAPKEYBOARD, SWC_MOUSE_AND_KEYBOARD));
			params.insert(std::make_pair(SWC_GIIENABLE, SWC_MOUSE_AND_KEYBOARD));
			params.insert(std::make_pair(SWC_REMOTECURSOR, SWC_MOUSE_AND_KEYBOARD));
			params.insert(std::make_pair(SWC_IGNOREREMOTE, SWC_MOUSE_AND_KEYBOARD));
			params.insert(std::make_pair(SWC_SWAPMOUSE, SWC_MOUSE_AND_KEYBOARD));
			params.insert(std::make_pair(SWC_THROTTLEMOUSE, SWC_MOUSE_AND_KEYBOARD));
			params.insert(std::make_pair(SWC_NOHOTKEY, SWC_MOUSE_AND_KEYBOARD));
			params.insert(std::make_pair(SWC_SHOWTOOLBAR, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_AUTOSCALING, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_AUTOSCALINGEVEN, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_PER, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_SERVERSCALE, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_FULLSCREEN, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_SAVEPOS, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_SAVESIZE, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_DIRECTX, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_ALLOWMONITOR, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_CHANGESERVERRES, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_EXTENDDISPLAY, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_SHOWEXTEND, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_USERVIRT, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_USEALLMONITOR, SWC_DISPLAY));
			params.insert(std::make_pair(SWC_QUICKOPTION, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_SHAERD, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_DISABLECLIPBOARD, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_USEDSMPLUGIN, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_DSMPLUGINFILENAME, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_REQUIREENCRYPTION, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_ALLOWUNTRUSTEDSERVER, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_AUTOACCEPTINCOMING, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_AUTOACCEPTNODSM, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_RESTRICT, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_INFOMSG, SWC_QUICK_ENCODER));
			params.insert(std::make_pair(SWC_RECONNECTCOUNTER, SWC_MISC));
			params.insert(std::make_pair(SWC_AUTORECONNECT, SWC_MISC));
			params.insert(std::make_pair(SWC_FTTIMEOUT, SWC_MISC));
			params.insert(std::make_pair(SWC_DISABLESPONSOR, SWC_MISC));
			params.insert(std::make_pair(SWC_NOSTATUS, SWC_MISC));
			params.insert(std::make_pair(SWC_IMAGEFORMAT, SWC_MISC));*/
		}
		void MessageSend(std::string AppName, std::string message)
		{
			HWND hWND1 = (HWND)FindWindow(NULL, AppName.c_str());//找到要传递的进程名
			if (::IsWindow(hWND1))
			{
				tagMESSAGE msg;
				memset(&msg, 0, sizeof(tagMESSAGE));
				strncpy(msg.szMsg, message.c_str(), sizeof(msg.szMsg) - 1);
				COPYDATASTRUCT copydata;
				copydata.dwData = CUSTOM_TYPE;  // 用户定义数据o 
				copydata.lpData = (PVOID)&msg;  //数据大小
				copydata.cbData = sizeof(tagMESSAGE) + 1;  // 指向数据的指针
				::SendMessage(hWND1, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&copydata));
			}
		}
		void MessageReceive(std::string message)
		{

			using json = nlohmann::json;
			try
			{
				json js = json::parse(message.begin(), message.end());
				std::string recvTitle = js.at("Title").get<std::string>();
				std::string recvContentText = js.at("ContentText").get<std::string>();
				HWND hWND1 = ::FindWindow(NULL, "HMI ");
				if (::IsWindow(hWND1))
				{
					if (strcmp(recvTitle.c_str(), "keyValue") == 0)
					{
						std::vector<std::string> values;
						Stringsplit(recvContentText, ".", values);
						if (values.size() == 2)
							long ad = ::SendMessage(hWND1, WM_SYSKEYUP, atoi(values.at(0).c_str()), atoi(values.at(1).c_str()));
					}
					else if (strcmp(recvTitle.c_str(), SWC_PER) == 0)
					{
						std::vector<std::string> values;
						Stringsplit(recvContentText, ".", values);
						if (values.size() == 4)
							SetWindowPos(hWND1, HWND_TOP, atoi(values.at(0).c_str()), atoi(values.at(1).c_str()), atoi(values.at(2).c_str()), atoi(values.at(3).c_str()), SWP_SHOWWINDOW);
						else
						{
							ini::iniReader config;
							ini::IniWriter myINI(".\\config.ini");
							bool ret = config.ReadConfig("config.ini");
							if (ret == false)
							{
								return;
							}
							if (config.ReadString(SWC_GLOBAL, SWC_WINDOWSSIZE, "").size() <= 0)
							{
								RECT rect;
								if (GetWindowRect(hWND1, &rect))
								{
									int width = rect.right - rect.left;
									int height = rect.bottom - rect.top;
									std::stringstream ss;
									ss << rect.left;
									std::string str1 = ss.str();
									ss.clear();
									ss.str("");
									ss << rect.top;
									std::string str2 = ss.str();
									ss.clear();
									ss.str("");
									ss << width;
									std::string str3 = ss.str();
									ss.clear();
									ss.str("");
									ss << height;
									std::string str4 = ss.str();
									std::string str = str1 + "." + str2 + "." + str3 + "." + str4;
									myINI.WriteString(SWC_GLOBAL, SWC_WINDOWSSIZE, str.c_str());
									int totalWidth = rect.right + rect.left;
									int totalHeight = rect.bottom + rect.top;
									int newLeft = (totalWidth - atoi(recvContentText.c_str()) * width / 100) / 2;
									int newTop = (totalHeight - atoi(recvContentText.c_str()) * height / 100) / 2;
									SetWindowPos(hWND1, HWND_TOP, newLeft, newTop, atoi(recvContentText.c_str()) * width / 100, atoi(recvContentText.c_str()) * height / 100, SWP_SHOWWINDOW);
								}
							}
							else
							{
								std::string str = config.ReadString(SWC_GLOBAL, SWC_WINDOWSSIZE, "");
								std::vector<std::string> rect;
								Stringsplit(str, ".", rect);
								if (rect.size() == 4)
								{
									int totalWidth = 2 * atoi(rect[0].c_str()) + atoi(rect[2].c_str());
									int totalHeight = 2 * atoi(rect[1].c_str()) + atoi(rect[3].c_str());
									int newLeft = (totalWidth - atoi(recvContentText.c_str()) * atoi(rect[2].c_str()) / 100) / 2;
									int newTop = (totalHeight - atoi(recvContentText.c_str()) * atoi(rect[3].c_str()) / 100) / 2;
									SetWindowPos(hWND1, HWND_TOP, newLeft, newTop, atoi(recvContentText.c_str()) * atoi(rect[2].c_str()) / 100, atoi(recvContentText.c_str()) * atoi(rect[3].c_str()) / 100, SWP_SHOWWINDOW);
								}
							}
						}
					}
					else if (strcmp(recvTitle.c_str(), SWC_VIEWONLY) == 0)
					{
						EnableWindow(hWND1, atoi(recvContentText.c_str()));
					}
				}
			}
			catch (json::parse_error& ex)
			{
				std::string errMess = ex.what();
			}
		}
	private:
		void Stringsplit(const std::string& str, const std::string& splits, std::vector<std::string>& res)
		{
			if (str == "")        return;
			//在字符串末尾也加入分隔符，方便截取最后一段
			std::string strs = str + splits;
			size_t pos = strs.find(splits);
			int step = splits.size();

			// 若找不到内容则字符串搜索函数返回 npos
			while (pos != strs.npos)
			{
				std::string temp = strs.substr(0, pos);
				res.push_back(temp);
				//去掉已分割的字符串,在剩下的字符串中进行分割
				strs = strs.substr(pos + step, strs.size());
				pos = strs.find(splits);
			}
		}
		//std::map<std::string, std::string> params;
	};


}
#endif