//  Copyright (C) 2002 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 TightVNC. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncProperties.cpp

// Implementation of the Properties dialog!

#include "stdhdrs.h"
#include "lmcons.h"
#include "vncservice.h"

#include "winvnc.h"
#include "vncproperties.h"
#include "vncserver.h"
#include "vncpasswd.h"
#include "vncOSVersion.h"
#include "common/win32_helpers.h"
#include "vncConnDialog.h"

#include "Localization.h" // ACT : Add localization on messages
#include "shlwapi.h"

//extern HINSTANCE g_hInst;

bool RunningAsAdministrator ();
const char WINVNC_REGISTRY_KEY [] = "Software\\ORL\\WinVNC3";

// [v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

// Marscha@2004 - authSSP: Function pointer for dyn. linking
typedef void (*vncEditSecurityFn) (HWND hwnd, HINSTANCE hInstance);
vncEditSecurityFn vncEditSecurity = 0;
// ethernet packet 1500 - 40 tcp/ip header - 8 PPPoE info
//unsigned int G_SENDBUFFER=8192;
unsigned int G_SENDBUFFER_EX=1452;

void Secure_Save_Plugin_Config(char *szPlugin);
void Secure_Plugin_elevated(char *szPlugin);
void Secure_Plugin(char *szPlugin);

// Constructor & Destructor
vncProperties::vncProperties()
{
    m_alloweditclients = TRUE;
	m_allowproperties = TRUE;
	m_allowInjection = FALSE;
	m_allowshutdown = TRUE;
	m_dlgvisible = FALSE;
	m_usersettings = TRUE;
	Lock_service_helper=TRUE;
	m_fUseRegistry = FALSE;
    m_ftTimeout = FT_RECV_TIMEOUT;
    m_keepAliveInterval = KEEPALIVE_INTERVAL;
	m_IdleInputTimeout = 0;
	m_pref_Primary=true;
	m_pref_Secondary=false;

	m_pref_DSMPluginConfig[0] = '\0';
	hBmpExpand = (HBITMAP)::LoadImage(hInstResDLL, MAKEINTRESOURCE(IDB_EXPAND), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
	hBmpCollaps = (HBITMAP)::LoadImage(hInstResDLL, MAKEINTRESOURCE(IDB_COLLAPS), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
	m_bExpanded = true;	
	cy = 0;
	cx = 0;
	service_commandline[0] = '\0';
	accept_reject_mesg[0] = '\0';

}

vncProperties::~vncProperties()
{
}

// Initialisation
BOOL
vncProperties::Init(vncServer *server)
{
	// Save the server pointer
	m_server = server;

	// sf@2007 - Registry mode can still be forced for backward compatibility and OS version < Vista
	m_fUseRegistry = ((myIniFile.ReadInt("admin", "UseRegistry", 0) == 1) ? TRUE : FALSE);

	// Load the settings
	if (m_fUseRegistry)
		Load(TRUE);
	else
		LoadFromIniFile();

	// If the password is empty then always show a dialog
	char passwd[MAXPWLEN];
	m_server->GetPassword(passwd);
	{
	    vncPasswd::ToText plain(passwd, m_pref_Secure);
	    if (strlen(plain) == 0)
			 if (!m_allowproperties || !RunningAsAdministrator ()) {
				if(m_server->AuthRequired()) {
					MessageBoxSecure(NULL, sz_ID_NO_PASSWD_NO_OVERRIDE_ERR,
								sz_ID_WINVNC_ERROR,
								MB_OK | MB_ICONSTOP);
					PostQuitMessage(0);
				}
				/*else {
					if (!vncService::RunningAsService())
						MessageBoxSecure(NULL, sz_ID_NO_PASSWD_NO_OVERRIDE_WARN,
								sz_ID_WINVNC_ERROR,
								MB_OK | MB_ICONEXCLAMATION);
				}*/
			} else {
				// If null passwords are not allowed, ensure that one is entered!
				if (m_server->AuthRequired()) {
					char username[UNLEN+1];
					if (!vncService::CurrentUser(username, sizeof(username)))
						return FALSE;
					if (strcmp(username, "") == 0) {
						Lock_service_helper=true;
						MessageBoxSecure(NULL, sz_ID_NO_PASSWD_NO_LOGON_WARN,
									sz_ID_WINVNC_ERROR,
									MB_OK | MB_ICONEXCLAMATION);
						ShowAdmin(TRUE, FALSE);
						Lock_service_helper=false;
					} else {
						ShowAdmin(TRUE, TRUE);
					}
				}
			}
	}
	Lock_service_helper=false;
	return TRUE;
}



// Dialog box handling functions
void
vncProperties::ShowAdmin(BOOL show, BOOL usersettings)
{
//	if (Lock_service_helper) return;
	HANDLE hProcess=NULL;
	HANDLE hPToken=NULL;
	DWORD id = vncService::GetExplorerLogonPid();
	int iImpersonateResult=0;
	{
		char WORKDIR[MAX_PATH];
		if (!GetTempPath(MAX_PATH,WORKDIR))
			{
				//Function failed, just set something
				if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
				{
					char* p = strrchr(WORKDIR, '\\');
					if (p == NULL) return;
					*p = '\0';
				}
					strcpy_s(m_Tempfile,"");
					strcat_s(m_Tempfile,WORKDIR);//set the directory
					strcat_s(m_Tempfile,"\\");
					strcat_s(m_Tempfile,INIFILE_NAME);
		}
		else
		{
			strcpy_s(m_Tempfile,"");
			strcat_s(m_Tempfile,WORKDIR);//set the directory
			strcat_s(m_Tempfile,INIFILE_NAME);
		}
	}
	if (id!=0 && usersettings)
			{
				hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,id);
				if(OpenProcessToken(hProcess,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
										|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
										|TOKEN_READ|TOKEN_WRITE,&hPToken))
				{
					ImpersonateLoggedOnUser(hPToken);
					iImpersonateResult = GetLastError();
					if(iImpersonateResult == ERROR_SUCCESS)
					{
						ExpandEnvironmentStringsForUser(hPToken, "%TEMP%", m_Tempfile, MAX_PATH);
						strcat_s(m_Tempfile,"\\");
						strcat_s(m_Tempfile,INIFILE_NAME);
					}
				}
	}

	if (!m_allowproperties) 
	{
		if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
		if (hProcess) CloseHandle(hProcess);
		if (hPToken) CloseHandle(hPToken);
		return;
	}
	/*if (!RunningAsAdministrator ())
		{
		if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
		CloseHandle(hProcess);
		CloseHandle(hPToken);
		return;
		}*/

	if (m_fUseRegistry)
	{
		if (vncService::RunningAsService()) usersettings=false;
		m_usersettings=usersettings;
	}

	if (show)
	{

		if (!m_fUseRegistry) // Use the ini file
		{
			// We're trying to edit the default local settings - verify that we can
			/*if (!myIniFile.IsWritable())
			{
				if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
				CloseHandle(hProcess);
				CloseHandle(hPToken);
				return;
			}*/
		}
		else // Use the registry
		{
			// Verify that we know who is logged on
			if (usersettings)
			{
				char username[UNLEN+1];
				if (!vncService::CurrentUser(username, sizeof(username)))
					{
						if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
						CloseHandle(hProcess);
						CloseHandle(hPToken);
						return;
					}
				if (strcmp(username, "") == 0) {
					MessageBoxSecure(NULL, sz_ID_NO_CURRENT_USER_ERR, sz_ID_WINVNC_ERROR, MB_OK | MB_ICONEXCLAMATION);
					if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
					CloseHandle(hProcess);
					CloseHandle(hPToken);
					return;
				}
			}
			else
			{
				// We're trying to edit the default local settings - verify that we can
				HKEY hkLocal=NULL;
				HKEY hkDefault=NULL;
				BOOL canEditDefaultPrefs = 1;
				DWORD dw;
				if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
					WINVNC_REGISTRY_KEY,
					0, REG_NONE, REG_OPTION_NON_VOLATILE,
					KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
					canEditDefaultPrefs = 0;
				else if (RegCreateKeyEx(hkLocal,
					"Default",
					0, REG_NONE, REG_OPTION_NON_VOLATILE,
					KEY_WRITE | KEY_READ, NULL, &hkDefault, &dw) != ERROR_SUCCESS)
					canEditDefaultPrefs = 0;
				if (hkLocal) RegCloseKey(hkLocal);
				if (hkDefault) RegCloseKey(hkDefault);

				if (!canEditDefaultPrefs) {
					MessageBoxSecure(NULL, sz_ID_CANNOT_EDIT_DEFAULT_PREFS, sz_ID_WINVNC_ERROR, MB_OK | MB_ICONEXCLAMATION);
					if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
					if (hProcess) CloseHandle(hProcess);
					if (hPToken) CloseHandle(hPToken);
					return;
				}
			}
		}

		// Now, if the dialog is not already displayed, show it!
		if (!m_dlgvisible)
		{
			if (m_fUseRegistry) 
			{
				if (usersettings)
					vnclog.Print(LL_INTINFO, VNCLOG("show per-user Properties\n"));
				else
					vnclog.Print(LL_INTINFO, VNCLOG("show default system Properties\n"));

				// Load in the settings relevant to the user or system
				//Load(usersettings);
				m_usersettings=usersettings;
			}

			for (;;)
			{
				m_returncode_valid = FALSE;

				// Do the dialog box
				// [v1.0.2-jp1 fix]
				//int result = DialogBoxParam(hAppInstance,
				m_bExpanded = true;
				cy = 0;
				cx = 0;
				int result = (int)DialogBoxParam(hInstResDLL,
				    MAKEINTRESOURCE(IDD_PROPERTIES1), 
				    NULL,
				    (DLGPROC) DialogProc,
				    (LONG_PTR) this);

				if (!m_returncode_valid)
				    result = IDCANCEL;

				vnclog.Print(LL_INTINFO, VNCLOG("dialog result = %d\n"), result);

				if (result == -1)
				{
					// Dialog box failed, so quit
					PostQuitMessage(0);
					if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
					CloseHandle(hProcess);
					CloseHandle(hPToken);
					return;
				}

				// We're allowed to exit if the password is not empty
				char passwd[MAXPWLEN];
				m_server->GetPassword(passwd);
				{
				    vncPasswd::ToText plain(passwd, m_server->Secure());
				    if ((strlen(plain) != 0) || !m_server->AuthRequired())
					break;
				}

				vnclog.Print(LL_INTERR, VNCLOG("warning - empty password\n"));

				// If we reached here then OK was used & there is no password!
				int result2 = MessageBoxSecure(NULL, sz_ID_NO_PASSWORD_WARN,
					sz_ID_WINVNC_WARNIN, MB_OK | MB_ICONEXCLAMATION);

				// The password is empty, so if OK was used then redisplay the box,
				// otherwise, if CANCEL was used, close down WinVNC
				if (result == IDCANCEL)
				{
				    vnclog.Print(LL_INTERR, VNCLOG("no password - QUITTING\n"));
				    PostQuitMessage(0);
				    if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
					CloseHandle(hProcess);
					CloseHandle(hPToken);
					fShutdownOrdered = true;
					return;
				}				

				omni_thread::sleep(4);
			}

			// Load in all the settings
			// If you run as service, you reload the saved settings before they are actual saved
			// via runas.....
			if (!vncService::RunningAsService())
			{
			if (m_fUseRegistry) 
				Load(TRUE);
			else
				LoadFromIniFile();
			}

		}
	}
	if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
	if (hProcess) CloseHandle(hProcess);
	if (hPToken) CloseHandle(hPToken);
}

BOOL CALLBACK
vncProperties::DialogProc(HWND hwnd,
						  UINT uMsg,
						  WPARAM wParam,
						  LPARAM lParam )
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
     vncProperties *_this = helper::SafeGetWindowUserData<vncProperties>(hwnd);

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{			
			vnclog.Print(LL_INTINFO, VNCLOG("INITDIALOG properties\n"));
			// Retrieve the Dialog box parameter and use it as a pointer
			// to the calling vncProperties object
            helper::SafeSetWindowUserData(hwnd, lParam);

			_this = (vncProperties *) lParam;
			_this->m_dlgvisible = TRUE;
			if (_this->m_fUseRegistry)
			{
				_this->Load(_this->m_usersettings);

				// Set the dialog box's title to indicate which Properties we're editting
				if (_this->m_usersettings) {
					SetWindowText(hwnd, sz_ID_CURRENT_USER_PROP);
				} else {
					SetWindowText(hwnd, sz_ID_DEFAULT_SYST_PROP);
				}
			}
			else
			{
				_this->LoadFromIniFile();
			}

			// Initialise the properties controls
			HWND hConnectSock = GetDlgItem(hwnd, IDC_CONNECT_SOCK);

			// Tight 1.2.7 method
			BOOL bConnectSock = _this->m_server->SockConnected();
			SendMessage(hConnectSock, BM_SETCHECK, bConnectSock, 0);

			// Set the content of the password field to a predefined string.
		    SetDlgItemText(hwnd, IDC_PASSWORD, "~~~~~~~~");
			EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), bConnectSock);

			// Set the content of the view-only password field to a predefined string. //PGM
		    SetDlgItemText(hwnd, IDC_PASSWORD2, "~~~~~~~~"); //PGM
			EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD2), bConnectSock); //PGM

			// Set the initial keyboard focus
			if (bConnectSock)
			{
				SetFocus(GetDlgItem(hwnd, IDC_PASSWORD));
				SendDlgItemMessage(hwnd, IDC_PASSWORD, EM_SETSEL, 0, (LPARAM)-1);
			}
			else
				SetFocus(hConnectSock);
			// Set display/ports settings
			_this->InitPortSettings(hwnd);

			HWND hConnectHTTP = GetDlgItem(hwnd, IDC_CONNECT_HTTP);
			SendMessage(hConnectHTTP,
				BM_SETCHECK,
				_this->m_server->HTTPConnectEnabled(),
				0);

		   // Modif sf@2002 - v1.1.0
		   HWND hFileTransfer = GetDlgItem(hwnd, IDC_FILETRANSFER);
           SendMessage(hFileTransfer, BM_SETCHECK, _this->m_server->FileTransferEnabled(), 0);

		   HWND hFileTransferUserImp = GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK);
           SendMessage(hFileTransferUserImp, BM_SETCHECK, _this->m_server->FTUserImpersonation(), 0);
		   
		   HWND hBlank = GetDlgItem(hwnd, IDC_BLANK);
           SendMessage(hBlank, BM_SETCHECK, _this->m_server->BlankMonitorEnabled(), 0);
		   if (!VNC_OSVersion::getInstance()->OS_WIN10_TRANS && VNC_OSVersion::getInstance()->OS_WIN10)
			   SetDlgItemText(hwnd, IDC_BLANK, "Enable Blank Monitor on Viewer Request require Min Win10 build 19041 ");
		   if (VNC_OSVersion::getInstance()->OS_WIN8)
			   SetDlgItemText(hwnd, IDC_BLANK, "Enable Blank Monitor on Viewer Not supported on windows 8 ");
		   HWND hBlank2 = GetDlgItem(hwnd, IDC_BLANK2); //PGM
           SendMessage(hBlank2, BM_SETCHECK, _this->m_server->BlankInputsOnly(), 0); //PGM
		   
		   HWND hLoopback = GetDlgItem(hwnd, IDC_ALLOWLOOPBACK);
		   BOOL fLoopback = _this->m_server->LoopbackOk();
		   SendMessage(hLoopback, BM_SETCHECK, fLoopback, 0);
#ifdef IPV6V4
		   HWND hIPV6 = GetDlgItem(hwnd, IDC_IPV6);
		   BOOL fIPV6 = _this->m_server->IPV6();
		   SendMessage(hIPV6, BM_SETCHECK, fIPV6, 0);
#else
		   HWND hIPV6 = GetDlgItem(hwnd, IDC_IPV6);
		   EnableWindow(hIPV6, false);
#endif
		   HWND hLoopbackonly = GetDlgItem(hwnd, IDC_LOOPBACKONLY);
		   BOOL fLoopbackonly = _this->m_server->LoopbackOnly();
		   SendMessage(hLoopbackonly, BM_SETCHECK, fLoopbackonly, 0);

		   HWND hTrayicon = GetDlgItem(hwnd, IDC_DISABLETRAY);
		   BOOL fTrayicon = _this->m_server->GetDisableTrayIcon();
		   SendMessage(hTrayicon, BM_SETCHECK, fTrayicon, 0);

		   HWND hrdpmode = GetDlgItem(hwnd, IDC_RDPMODE);
		   BOOL frdpmode = _this->m_server->GetRdpmode();
		   SendMessage(hrdpmode, BM_SETCHECK, frdpmode, 0);

		   HWND hNoScreensaver= GetDlgItem(hwnd,IDC_NOSCREENSAVER);
		   BOOL fNoScrensaver = _this->m_server->GetNoScreensaver();
		   SendMessage(hNoScreensaver, BM_SETCHECK, fNoScrensaver, 0);

		   HWND hAllowshutdown = GetDlgItem(hwnd, IDC_ALLOWSHUTDOWN);
		   SendMessage(hAllowshutdown, BM_SETCHECK, !_this->m_allowshutdown , 0);

		   HWND hm_alloweditclients = GetDlgItem(hwnd, IDC_ALLOWEDITCLIENTS);
		   SendMessage(hm_alloweditclients, BM_SETCHECK, !_this->m_alloweditclients , 0);
		   _this->m_server->SetAllowEditClients(_this->m_alloweditclients);
		   

		   if (vnclog.GetMode() >= 2)
			   CheckDlgButton(hwnd, IDC_LOG, BST_CHECKED);
		   else
			   CheckDlgButton(hwnd, IDC_LOG, BST_UNCHECKED);

#ifndef AVILOG
		   ShowWindow (GetDlgItem(hwnd, IDC_CLEAR), SW_HIDE);
		   ShowWindow (GetDlgItem(hwnd, IDC_VIDEO), SW_HIDE);
#endif
		   if (vnclog.GetVideo())
		   {
			   SetDlgItemText(hwnd, IDC_EDIT_PATH, vnclog.GetPath());
			   //EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), true);
			   CheckDlgButton(hwnd, IDC_VIDEO, BST_CHECKED);
		   }
		   else
		   {
			   SetDlgItemText(hwnd, IDC_EDIT_PATH, vnclog.GetPath());
			   //EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), false);
			   CheckDlgButton(hwnd, IDC_VIDEO, BST_UNCHECKED);
		   }
		   
			// Marscha@2004 - authSSP: moved MS-Logon checkbox back to admin props page
			// added New MS-Logon checkbox
			// only enable New MS-Logon checkbox and Configure MS-Logon groups when MS-Logon
			// is checked.
		   HWND hSecure = GetDlgItem(hwnd, IDC_SAVEPASSWORDSECURE);
		   SendMessage(hSecure, BM_SETCHECK, _this->m_server->Secure(), 0);
		   
			HWND hMSLogon = GetDlgItem(hwnd, IDC_MSLOGON_CHECKD);
			SendMessage(hMSLogon, BM_SETCHECK, _this->m_server->MSLogonRequired(), 0);

			HWND hNewMSLogon = GetDlgItem(hwnd, IDC_NEW_MSLOGON);
			SendMessage(hNewMSLogon, BM_SETCHECK, _this->m_server->GetNewMSLogon(), 0);

			HWND hReverseAuth = GetDlgItem(hwnd, IDC_REVERSEAUTH);
			SendMessage(hReverseAuth, BM_SETCHECK, _this->m_server->GetReverseAuthRequired(), 0);

			EnableWindow(GetDlgItem(hwnd, IDC_NEW_MSLOGON), _this->m_server->MSLogonRequired());
			EnableWindow(GetDlgItem(hwnd, IDC_MSLOGON), _this->m_server->MSLogonRequired());
			// Marscha@2004 - authSSP: end of change

		   SetDlgItemInt(hwnd, IDC_SCALE, _this->m_server->GetDefaultScale(), false);

		   
		   // Remote input settings
			HWND hEnableRemoteInputs = GetDlgItem(hwnd, IDC_DISABLE_INPUTS);
			SendMessage(hEnableRemoteInputs,
				BM_SETCHECK,
				!(_this->m_server->RemoteInputsEnabled()),
				0);

			// Local input settings
			HWND hDisableLocalInputs = GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS);
			SendMessage(hDisableLocalInputs,
				BM_SETCHECK,
				_this->m_server->LocalInputsDisabled(),
				0);

			// japanese keybaord
			HWND hJapInputs = GetDlgItem(hwnd, IDC_JAP_INPUTS);
			SendMessage(hJapInputs,
				BM_SETCHECK,
				_this->m_server->JapInputEnabled(),
				0);

			HWND hUnicodeInputs = GetDlgItem(hwnd, IDC_UNICODE_INPUTS);
			SendMessage(hUnicodeInputs,
				BM_SETCHECK,
				_this->m_server->UnicodeInputEnabled(),
				0);

			HWND hwinhelper = GetDlgItem(hwnd, IDC_WIN8_HELPER);
			SendMessage(hwinhelper,
				BM_SETCHECK,
				_this->m_server->Win8HelperEnabled(),
				0);

			// Remove the wallpaper
			HWND hRemoveWallpaper = GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER);
			SendMessage(hRemoveWallpaper,
				BM_SETCHECK,
				_this->m_server->RemoveWallpaperEnabled(),
				0);

			// Lock settings
			HWND hLockSetting;
			switch (_this->m_server->LockSettings()) {
			case 1:
				hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK);
				break;
			case 2:
				hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_LOGOFF);
				break;
			default:
				hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_NOTHING);
			};
			SendMessage(hLockSetting,
				BM_SETCHECK,
				TRUE,
				0);

			HWND hNotificationSelection;
			switch (_this->m_server->getNotificationSelection()) {
			case 1:
				hNotificationSelection = GetDlgItem(hwnd, IDC_RADIONOTIFICATIONIFPROVIDED);
				break;
			default:
				hNotificationSelection = GetDlgItem(hwnd,
					IDC_RADIONOTIFICATIONON);
				break;
			};
			SendMessage(hNotificationSelection,
				BM_SETCHECK,
				TRUE,
				0);

			HWND hmvSetting = 0;
			switch (_this->m_server->ConnectPriority()) {
			case 0:
				hmvSetting = GetDlgItem(hwnd, IDC_MV1);
				break;
			case 1:
				hmvSetting = GetDlgItem(hwnd, IDC_MV2);
				break;
			case 2:
				hmvSetting = GetDlgItem(hwnd, IDC_MV3);
				break;
			case 3:
				hmvSetting = GetDlgItem(hwnd, IDC_MV4);
				break;
			};
			SendMessage(hmvSetting,
				BM_SETCHECK,
				TRUE,
				0);


			HWND hQuerySetting;
			switch (_this->m_server->QueryAccept()) {
			case 0:
				hQuerySetting = GetDlgItem(hwnd, IDC_DREFUSE);
				break;
			case 1:
				hQuerySetting = GetDlgItem(hwnd, IDC_DACCEPT);
				break;
			default:
				hQuerySetting = GetDlgItem(hwnd, IDC_DREFUSE);
			};
			SendMessage(hQuerySetting,
				BM_SETCHECK,
				TRUE,
				0);

			HWND hMaxViewerSetting;
			switch (_this->m_server->getMaxViewerSetting()) {
			case 0:
				hMaxViewerSetting = GetDlgItem(hwnd, IDC_MAXREFUSE);
				break;
			case 1:
				hMaxViewerSetting = GetDlgItem(hwnd, IDC_MAXDISCONNECT);
				break;
			default:
				hMaxViewerSetting = GetDlgItem(hwnd, IDC_MAXREFUSE);
			};
			SendMessage(hMaxViewerSetting,
				BM_SETCHECK,
				TRUE,
				0);

			HWND hCollabo = GetDlgItem(hwnd, IDC_COLLABO);
			SendMessage(hCollabo,
				BM_SETCHECK,
				_this->m_server->getCollabo(),
				0);

			HWND hwndDlg = GetDlgItem(hwnd, IDC_FRAME);
			SendMessage(hwndDlg,
				BM_SETCHECK,
				_this->m_server->getFrame(),
				0);

			hwndDlg = GetDlgItem(hwnd, IDC_NOTIFOCATION);
			SendMessage(hwndDlg,
				BM_SETCHECK,
				_this->m_server->getNotification(),
				0);

			hwndDlg = GetDlgItem(hwnd, IDC_OSD);
			SendMessage(hwndDlg,
				BM_SETCHECK,
				_this->m_server->getOSD(),
				0);

			char maxviewersChar[128];
			UINT maxviewers = _this->m_server->getMaxViewers();
			sprintf_s(maxviewersChar, "%d", (int)maxviewers);
			SetDlgItemText(hwnd, IDC_MAXVIEWERS, (const char*)maxviewersChar);

			// sf@2002 - List available DSM Plugins
			HWND hPlugins = GetDlgItem(hwnd, IDC_PLUGINS_COMBO);
			int nPlugins = _this->m_server->GetDSMPluginPointer()->ListPlugins(hPlugins);
			if (!nPlugins) 
			{
				SendMessage(hPlugins, CB_ADDSTRING, 0, (LPARAM) sz_ID_NO_PLUGIN_DETECT);
				SendMessage(hPlugins, CB_SETCURSEL, 0, 0);
			}
			else
				SendMessage(hPlugins, CB_SELECTSTRING, 0, (LPARAM)_this->m_server->GetDSMPluginName());

			// Modif sf@2002
			SendMessage(GetDlgItem(hwnd, IDC_PLUGIN_CHECK), BM_SETCHECK, _this->m_server->IsDSMPluginEnabled(), 0);
			EnableWindow(GetDlgItem(hwnd, IDC_PLUGIN_BUTTON),  (_this->m_server->AuthClientCount() == 0 ? _this->m_server->IsDSMPluginEnabled(): false));

			// Query window option - Taken from TightVNC advanced properties 
			BOOL queryEnabled = (_this->m_server->QuerySetting() == 4);
			SendMessage(GetDlgItem(hwnd, IDQUERY), BM_SETCHECK, queryEnabled, 0);
			EnableWindow(GetDlgItem(hwnd, IDQUERYTIMEOUT), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_QUERYDISABLETIME), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_DREFUSE), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_DACCEPT), queryEnabled);

			SetDlgItemText(hwnd, IDC_SERVICE_COMMANDLINE, _this->service_commandline);
			SetDlgItemText(hwnd, IDC_EDITQUERYTEXT, _this->accept_reject_mesg);


			char timeout[128];
			UINT t = _this->m_server->QueryTimeout();
			sprintf_s(timeout, "%d", (int)t);
		    SetDlgItemText(hwnd, IDQUERYTIMEOUT, (const char *) timeout);

			char disableTime[128];
			UINT tt = _this->m_server->QueryDisableTime();
			sprintf_s(disableTime, "%d", (int)tt);
		    SetDlgItemText(hwnd, IDC_QUERYDISABLETIME, (const char *) disableTime);

			_this->ExpandBox(hwnd, !_this->m_bExpanded);
			SendMessage(GetDlgItem(hwnd, IDC_BUTTON_EXPAND), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)_this->hBmpExpand);

			SetForegroundWindow(hwnd);

			return FALSE; // Because we've set the focus
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_SHOWOPTIONS:
		case IDC_BUTTON_EXPAND:
			_this->ExpandBox(hwnd, !_this->m_bExpanded);
//			if (_this->m_bExpanded)
//				_this->InitTab(hwnd);
			return TRUE;
		case IDOK:
		case IDC_APPLY:
			{
				char path[512];
				int lenpath = GetDlgItemText(hwnd, IDC_EDIT_PATH, (LPSTR) &path, 512);
				if (lenpath != 0)
					{
						vnclog.SetPath(path);
				}
				else
				{
					strcpy_s(path,"");
					vnclog.SetPath(path);
				}
				bool Secure_old = _this->m_server->Secure();
				HWND hSecure = GetDlgItem(hwnd, IDC_SAVEPASSWORDSECURE);
				_this->m_server->Secure(SendMessage(hSecure, BM_GETCHECK, 0, 0) == BST_CHECKED);

				// Save the password
				char passwd[MAXPWLEN+1];
				char passwd2[MAXPWLEN+1];
				memset(passwd, '\0', MAXPWLEN + 1); //PGM
				memset(passwd2, '\0', MAXPWLEN + 1); //PGM
				// TightVNC method
				int lenPassword = GetDlgItemText(hwnd, IDC_PASSWORD, (LPSTR) &passwd, MAXPWLEN+1);				
				int lenPassword2 = GetDlgItemText(hwnd, IDC_PASSWORD2, (LPSTR)&passwd2, MAXPWLEN + 1); //PGM

                bool bSecure = _this->m_server->Secure() ? true : false;
				if (Secure_old != bSecure) {
					//We changed the method to save the password
					//load passwords and encrypt the other method
					char password[MAXPWLEN];
					char password2[MAXPWLEN];
					_this->m_server->GetPassword(password);
					vncPasswd::ToText plain(password, Secure_old);
					_this->m_server->GetPassword2(password2);
					vncPasswd::ToText plain2(password2, Secure_old);
					memset(passwd, '\0', MAXPWLEN + 1); //PGM
					memset(passwd2, '\0', MAXPWLEN + 1); //PGM
					strcpy_s(passwd,plain);
					strcpy_s(passwd2, plain2);
					lenPassword = (int)strlen(passwd);
					lenPassword2 = (int)strlen(passwd2);
				}
				
				if (strcmp(passwd, "~~~~~~~~") != 0) {
					if (lenPassword == 0)
					{
						vncPasswd::FromClear crypt(_this->m_server->Secure());
						_this->m_server->SetPassword(crypt);
					}
					else
					{
						vncPasswd::FromText crypt(passwd, _this->m_server->Secure());
						_this->m_server->SetPassword(crypt);
					}
				}

				
				if (strcmp(passwd2, "~~~~~~~~") != 0) { //PGM
					if (lenPassword2 == 0) //PGM
					{ //PGM
						vncPasswd::FromClear crypt2(_this->m_server->Secure()); //PGM
						_this->m_server->SetPassword2(crypt2); //PGM
					} //PGM
					else //PGM
					{ //PGM
						vncPasswd::FromText crypt2(passwd2, _this->m_server->Secure()); //PGM
						_this->m_server->SetPassword2(crypt2); //PGM
					} //PGM
				} //PGM


				//avoid readonly and full passwd being set the same
				if (strcmp(passwd, "~~~~~~~~") != 0 && strcmp(passwd2, "~~~~~~~~") != 0) { 
					if (strcmp(passwd,passwd2)==0)
					{
						MessageBox(NULL,"View only and full password are the same\nView only ignored","Warning",0);
					}					
				} 

				// Save the new settings to the server
				int state = (int)SendDlgItemMessage(hwnd, IDC_PORTNO_AUTO, BM_GETCHECK, 0, 0);
				_this->m_server->SetAutoPortSelect(state == BST_CHECKED);

				// Save port numbers if we're not auto selecting
				if (!_this->m_server->AutoPortSelect()) {
					if ( SendDlgItemMessage(hwnd, IDC_SPECDISPLAY,
											BM_GETCHECK, 0, 0) == BST_CHECKED ) {
						// Display number was specified
						BOOL ok;
						UINT display = GetDlgItemInt(hwnd, IDC_DISPLAYNO, &ok, TRUE);
						if (ok)
							_this->m_server->SetPorts(DISPLAY_TO_PORT(display),
													  DISPLAY_TO_HPORT(display));
					} else {
						// Assuming that port numbers were specified
						BOOL ok1, ok2;
						UINT port_rfb = GetDlgItemInt(hwnd, IDC_PORTRFB, &ok1, TRUE);
						UINT port_http = GetDlgItemInt(hwnd, IDC_PORTHTTP, &ok2, TRUE);
						if (ok1 && ok2)
							_this->m_server->SetPorts(port_rfb, port_http);
					}
				}
				HWND hConnectSock = GetDlgItem(hwnd, IDC_CONNECT_SOCK);
				_this->m_server->SockConnect(
					SendMessage(hConnectSock, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// Update display/port controls on pressing the "Apply" button
				if (LOWORD(wParam) == IDC_APPLY)
					_this->InitPortSettings(hwnd);

				

				HWND hConnectHTTP = GetDlgItem(hwnd, IDC_CONNECT_HTTP);
				_this->m_server->EnableHTTPConnect(
					SendMessage(hConnectHTTP, BM_GETCHECK, 0, 0) == BST_CHECKED
					);
				
				// Remote input stuff
				HWND hEnableRemoteInputs = GetDlgItem(hwnd, IDC_DISABLE_INPUTS);
				_this->m_server->EnableRemoteInputs(
					SendMessage(hEnableRemoteInputs, BM_GETCHECK, 0, 0) != BST_CHECKED
					);

				// Local input stuff
				HWND hDisableLocalInputs = GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS);
				_this->m_server->DisableLocalInputs(
					SendMessage(hDisableLocalInputs, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// japanese keyboard
				HWND hJapInputs = GetDlgItem(hwnd, IDC_JAP_INPUTS);
				_this->m_server->EnableJapInput(
					SendMessage(hJapInputs, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// japanese keyboard
				HWND hUnicodeInputs = GetDlgItem(hwnd, IDC_UNICODE_INPUTS);
				_this->m_server->EnableUnicodeInput(
					SendMessage(hUnicodeInputs, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hwinhelper = GetDlgItem(hwnd, IDC_WIN8_HELPER);
				_this->m_server->Win8HelperEnabled(
					SendMessage(hwinhelper, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// Wallpaper handling
				HWND hRemoveWallpaper = GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER);
				_this->m_server->EnableRemoveWallpaper(
					SendMessage(hRemoveWallpaper, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// Lock settings handling
				if (SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetLockSettings(1);
				} else if (SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOGOFF), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetLockSettings(2);
				} else {
					_this->m_server->SetLockSettings(0);
				}

				if (SendMessage(GetDlgItem(hwnd, IDC_RADIONOTIFICATIONON), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->setNotificationSelection(0);
				}
				else if (SendMessage(GetDlgItem(hwnd, IDC_RADIONOTIFICATIONIFPROVIDED), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->setNotificationSelection(1);
				}

				if (SendMessage(GetDlgItem(hwnd, IDC_DREFUSE), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetQueryAccept(0);
				} else if (SendMessage(GetDlgItem(hwnd, IDC_DACCEPT), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetQueryAccept(1);
				} 

				if (SendMessage(GetDlgItem(hwnd, IDC_MAXREFUSE), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->setMaxViewerSetting(0);
				}
				else if (SendMessage(GetDlgItem(hwnd, IDC_MAXDISCONNECT), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->setMaxViewerSetting(1);
				}

				char maxViewerChar[256];
				strcpy_s(maxViewerChar, "128");				
				if (GetDlgItemText(hwnd, IDC_MAXVIEWERS, (LPSTR)&maxViewerChar, 256) == 0) {
					int value = atoi(maxViewerChar);
					if (value > 128) 
						value = 128;
					_this->m_server->setMaxViewers(value);
				}
				else
					_this->m_server->setMaxViewers(atoi(maxViewerChar));

				HWND hCollabo = GetDlgItem(hwnd, IDC_COLLABO);
				_this->m_server->setCollabo(
					SendMessage(hCollabo, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hwndDlg = GetDlgItem(hwnd, IDC_FRAME);
				_this->m_server->setFrame(
					SendMessage(hwndDlg, BM_GETCHECK, 0, 0) == BST_CHECKED
				);

				hwndDlg = GetDlgItem(hwnd, IDC_NOTIFOCATION);
				_this->m_server->setNotification(
					SendMessage(hwndDlg, BM_GETCHECK, 0, 0) == BST_CHECKED
				);

				hwndDlg = GetDlgItem(hwnd, IDC_OSD);
				_this->m_server->setOSD(
					SendMessage(hwndDlg, BM_GETCHECK, 0, 0) == BST_CHECKED
				);

				if (SendMessage(GetDlgItem(hwnd, IDC_MV1), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetConnectPriority(0);
				} else if (SendMessage(GetDlgItem(hwnd, IDC_MV2), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetConnectPriority(1);
				} 
				 else if (SendMessage(GetDlgItem(hwnd, IDC_MV3), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetConnectPriority(2);
				} else if (SendMessage(GetDlgItem(hwnd, IDC_MV4), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					_this->m_server->SetConnectPriority(3);
				} 
				
				// Modif sf@2002 - v1.1.0
				HWND hFileTransfer = GetDlgItem(hwnd, IDC_FILETRANSFER);
				_this->m_server->EnableFileTransfer(SendMessage(hFileTransfer, BM_GETCHECK, 0, 0) == BST_CHECKED);

				HWND hFileTransferUserImp = GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK);
				_this->m_server->FTUserImpersonation(SendMessage(hFileTransferUserImp, BM_GETCHECK, 0, 0) == BST_CHECKED);

				HWND hBlank = GetDlgItem(hwnd, IDC_BLANK);
				_this->m_server->BlankMonitorEnabled(SendMessage(hBlank, BM_GETCHECK, 0, 0) == BST_CHECKED);
				HWND hBlank2 = GetDlgItem(hwnd, IDC_BLANK2); //PGM
				_this->m_server->BlankInputsOnly(SendMessage(hBlank2, BM_GETCHECK, 0, 0) == BST_CHECKED); //PGM				
				
				_this->m_server->SetLoopbackOk(IsDlgButtonChecked(hwnd, IDC_ALLOWLOOPBACK));
#ifdef IPV6V4
				_this->m_server->SetIPV6(IsDlgButtonChecked(hwnd, IDC_IPV6));
#endif
				_this->m_server->SetLoopbackOnly(IsDlgButtonChecked(hwnd, IDC_LOOPBACKONLY));

				_this->m_server->SetDisableTrayIcon(IsDlgButtonChecked(hwnd, IDC_DISABLETRAY));
				_this->m_server->SetRdpmode(IsDlgButtonChecked(hwnd, IDC_RDPMODE));
				_this->m_server->SetNoScreensaver(IsDlgButtonChecked(hwnd, IDC_NOSCREENSAVER));
				_this->m_allowshutdown=!IsDlgButtonChecked(hwnd, IDC_ALLOWSHUTDOWN);
				_this->m_alloweditclients=!IsDlgButtonChecked(hwnd, IDC_ALLOWEDITCLIENTS);
				_this->m_server->SetAllowEditClients(_this->m_alloweditclients);
				if (IsDlgButtonChecked(hwnd, IDC_LOG))
				{
					vnclog.SetMode(2);
					vnclog.SetLevel(10);
				}
				else
				{
					vnclog.SetMode(0);
				}
				if (IsDlgButtonChecked(hwnd, IDC_VIDEO))
				{
					vnclog.SetVideo(true);
				}
				else
				{
					vnclog.SetVideo(false);
				}
				// Modif sf@2002 - v1.1.0
				// Marscha@2004 - authSSP: moved MS-Logon checkbox back to admin props page
				// added New MS-Logon checkbox				

				HWND hMSLogon = GetDlgItem(hwnd, IDC_MSLOGON_CHECKD);
				_this->m_server->RequireMSLogon(SendMessage(hMSLogon, BM_GETCHECK, 0, 0) == BST_CHECKED);
				
				HWND hNewMSLogon = GetDlgItem(hwnd, IDC_NEW_MSLOGON);
				_this->m_server->SetNewMSLogon(SendMessage(hNewMSLogon, BM_GETCHECK, 0, 0) == BST_CHECKED);
				// Marscha@2004 - authSSP: end of change

				HWND hReverseAuth = GetDlgItem(hwnd, IDC_REVERSEAUTH);
				_this->m_server->SetReverseAuthRequired(SendMessage(hReverseAuth, BM_GETCHECK, 0, 0) == BST_CHECKED);

				int nDScale = GetDlgItemInt(hwnd, IDC_SCALE, NULL, FALSE);
				if (nDScale < 1 || nDScale > 9) nDScale = 1;
				_this->m_server->SetDefaultScale(nDScale);
				
				// sf@2002 - DSM Plugin loading
				// If Use plugin is checked, load the plugin if necessary
				if (SendMessage(GetDlgItem(hwnd, IDC_PLUGIN_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					TCHAR szPlugin[MAX_PATH];
					GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
					_this->m_server->SetDSMPluginName(szPlugin);
					_this->m_server->EnableDSMPlugin(true);
				}
				else // If Use plugin unchecked but the plugin is loaded, unload it
				{
					_this->m_server->EnableDSMPlugin(false);
					if (_this->m_server->GetDSMPluginPointer()->IsLoaded())
					{
						_this->m_server->GetDSMPluginPointer()->UnloadPlugin();
						_this->m_server->GetDSMPluginPointer()->SetEnabled(false);
					}	
				}

				//adzm 2010-05-12 - dsmplugin config
				_this->m_server->SetDSMPluginConfig(_this->m_pref_DSMPluginConfig);

				// Query Window options - Taken from TightVNC advanced properties
				char timeout[256];
				strcpy_s(timeout,"5");
				if (GetDlgItemText(hwnd, IDQUERYTIMEOUT, (LPSTR) &timeout, 256) == 0)
				    _this->m_server->SetQueryTimeout(atoi(timeout));
				else
				    _this->m_server->SetQueryTimeout(atoi(timeout));

				char disabletime[256];
				strcpy_s(disabletime,"5");
				if (GetDlgItemText(hwnd, IDC_QUERYDISABLETIME, (LPSTR) &disabletime, 256) == 0)
				    _this->m_server->SetQueryDisableTime(atoi(disabletime));
				else
				    _this->m_server->SetQueryDisableTime(atoi(disabletime));

				GetDlgItemText(hwnd, IDC_SERVICE_COMMANDLINE, _this->service_commandline, 1024);
				GetDlgItemText(hwnd, IDC_EDITQUERYTEXT, _this->accept_reject_mesg, 512);


				HWND hQuery = GetDlgItem(hwnd, IDQUERY);
				_this->m_server->SetQuerySetting((SendMessage(hQuery, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 4 : 2);

				// And to the registry

				/*if (!RunningAsAdministrator () && vncService::RunningAsService())
				{
					MessageBoxSecure(NULL,"Only admins are allowed to save","Warning", MB_OK | MB_ICONINFORMATION);
				}
				else*/
				{
				// Load the settings
				if (_this->m_fUseRegistry)
					_this->Save();
				else
					_this->SaveToIniFile();
				}

				// Was ok pressed?
				if (LOWORD(wParam) == IDOK)
				{
					// Yes, so close the dialog
					vnclog.Print(LL_INTINFO, VNCLOG("enddialog (OK)\n"));

					_this->m_returncode_valid = TRUE;

					EndDialog(hwnd, IDOK);
					_this->m_dlgvisible = FALSE;
				}

				_this->m_server->SetHookings();

				return TRUE;
			}

		case IDCANCEL:
			vnclog.Print(LL_INTINFO, VNCLOG("enddialog (CANCEL)\n"));

			_this->m_returncode_valid = TRUE;

			EndDialog(hwnd, IDCANCEL);
			_this->m_dlgvisible = FALSE;
			return TRUE;

	    // Added Jef Fix - 5 March 2008 paquette@atnetsend.net
        case IDC_BLANK:
            {
                // only enable alpha blanking if blanking is enabled
                HWND hBlank = ::GetDlgItem(hwnd, IDC_BLANK);               
                HWND hBlank2 = ::GetDlgItem(hwnd, IDC_BLANK2); //PGM
                ::EnableWindow(hBlank2, ::SendMessage(hBlank, BM_GETCHECK, 0, 0) == BST_CHECKED); //PGM
            }
            break;

        case IDC_BLANK2: //PGM
            { //PGM
                // only enable alpha blanking if Disable Only Inputs is disabled //PGM
                HWND hBlank = ::GetDlgItem(hwnd, IDC_BLANK2); //PGM              
            } //PGM
            break; //PGM

		case IDC_VIDEO:
			{
				if (IsDlgButtonChecked(hwnd, IDC_VIDEO))
				   {
					   EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), true);
					   
				   }
				   else
				   {
					   EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), false);
					   
				   }
				break;
			}

		case IDC_CLEAR:
			{
				vnclog.ClearAviConfig();
				break;
			}

		case IDC_CONNECT_SOCK:
			// TightVNC 1.2.7 method
			// The user has clicked on the socket connect tickbox
			{
				BOOL bConnectSock =
					(SendDlgItemMessage(hwnd, IDC_CONNECT_SOCK,
										BM_GETCHECK, 0, 0) == BST_CHECKED);

				EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), bConnectSock);

				HWND hPortNoAuto = GetDlgItem(hwnd, IDC_PORTNO_AUTO);
				EnableWindow(hPortNoAuto, bConnectSock);
				HWND hSpecDisplay = GetDlgItem(hwnd, IDC_SPECDISPLAY);
				EnableWindow(hSpecDisplay, bConnectSock);
				HWND hSpecPort = GetDlgItem(hwnd, IDC_SPECPORT);
				EnableWindow(hSpecPort, bConnectSock);

				EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), bConnectSock &&
					(SendMessage(hSpecDisplay, BM_GETCHECK, 0, 0) == BST_CHECKED));
				EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), bConnectSock &&
					(SendMessage(hSpecPort, BM_GETCHECK, 0, 0) == BST_CHECKED));
				EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), bConnectSock &&
					(SendMessage(hSpecPort, BM_GETCHECK, 0, 0) == BST_CHECKED));
			}
			return TRUE;

		// TightVNC 1.2.7 method
		case IDC_PORTNO_AUTO:
			{
				EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), FALSE);

				SetDlgItemText(hwnd, IDC_DISPLAYNO, "");
				SetDlgItemText(hwnd, IDC_PORTRFB, "");
				SetDlgItemText(hwnd, IDC_PORTHTTP, "");
			}
			return TRUE;

		case IDC_SPECDISPLAY:
			{
				EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), FALSE);

				int display = PORT_TO_DISPLAY(_this->m_server->GetPort());
				if (display < 0 || display > 99)
					display = 0;
				SetDlgItemInt(hwnd, IDC_DISPLAYNO, display, FALSE);
				SetDlgItemInt(hwnd, IDC_PORTRFB, _this->m_server->GetPort(), FALSE);
				SetDlgItemInt(hwnd, IDC_PORTHTTP, _this->m_server->GetHttpPort(), FALSE);


				SetFocus(GetDlgItem(hwnd, IDC_DISPLAYNO));
				SendDlgItemMessage(hwnd, IDC_DISPLAYNO, EM_SETSEL, 0, (LPARAM)-1);
			}
			return TRUE;

		case IDC_SPECPORT:
			{
				EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP), TRUE);

				int d1 = PORT_TO_DISPLAY(_this->m_server->GetPort());
				int d2 = HPORT_TO_DISPLAY(_this->m_server->GetHttpPort());
				if (d1 == d2 && d1 >= 0 && d1 <= 99) {
					SetDlgItemInt(hwnd, IDC_DISPLAYNO, d1, FALSE);
				} else {
					SetDlgItemText(hwnd, IDC_DISPLAYNO, "");
				}
				SetDlgItemInt(hwnd, IDC_PORTRFB, _this->m_server->GetPort(), FALSE);
				SetDlgItemInt(hwnd, IDC_PORTHTTP, _this->m_server->GetHttpPort(), FALSE);


				SetFocus(GetDlgItem(hwnd, IDC_PORTRFB));
				SendDlgItemMessage(hwnd, IDC_PORTRFB, EM_SETSEL, 0, (LPARAM)-1);
			}
			return TRUE;

		// Query window option - Taken from TightVNC advanced properties code
		case IDQUERY:
			{
				HWND hQuery = GetDlgItem(hwnd, IDQUERY);
				BOOL queryon = (SendMessage(hQuery, BM_GETCHECK, 0, 0) == BST_CHECKED);
				EnableWindow(GetDlgItem(hwnd, IDQUERYTIMEOUT), queryon);
				EnableWindow(GetDlgItem(hwnd, IDC_QUERYDISABLETIME), queryon);
				EnableWindow(GetDlgItem(hwnd, IDC_DREFUSE), queryon);
				EnableWindow(GetDlgItem(hwnd, IDC_DACCEPT), queryon);
			}
			return TRUE;

		case IDC_STARTREP:
			{
				vncConnDialog *newconn = new vncConnDialog(_this->m_server);
				if (newconn)
				{
					newconn->DoDialog(true);
					// delete newconn; // NO ! Already done in vncConnDialog.
				}
			}

		// sf@2002 - DSM Plugin
		case IDC_PLUGIN_CHECK:
			{
				EnableWindow(GetDlgItem(hwnd, IDC_PLUGIN_BUTTON), _this->m_server->AuthClientCount() == 0 
						? SendMessage(GetDlgItem(hwnd, IDC_PLUGIN_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED 
						: BST_UNCHECKED);
			}
			return TRUE;
			// Marscha@2004 - authSSP: moved MSLogon checkbox back to admin props page
			// Reason: Different UI for old and new mslogon group config.
		case IDC_MSLOGON_CHECKD:
			{
				BOOL bMSLogonChecked =
				(SendDlgItemMessage(hwnd, IDC_MSLOGON_CHECKD,
										BM_GETCHECK, 0, 0) == BST_CHECKED);

				EnableWindow(GetDlgItem(hwnd, IDC_NEW_MSLOGON), bMSLogonChecked);
				EnableWindow(GetDlgItem(hwnd, IDC_MSLOGON), bMSLogonChecked);

			}
			return TRUE;
		case IDC_MSLOGON:
			{
				// Marscha@2004 - authSSP: if "New MS-Logon" is checked,
				// call vncEditSecurity from SecurityEditor.dll,
				// else call "old" dialog.
				BOOL bNewMSLogonChecked =
				(SendDlgItemMessage(hwnd, IDC_NEW_MSLOGON,
										BM_GETCHECK, 0, 0) == BST_CHECKED);
				if (bNewMSLogonChecked) {
					void winvncSecurityEditorHelper_as_admin();
						HANDLE hProcess,hPToken;
						DWORD id = vncService::GetExplorerLogonPid();
						if (id!=0) 
						{
							hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,id);
							if (!hProcess) goto error;
							if(!OpenProcessToken(hProcess,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
													|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
													|TOKEN_READ|TOKEN_WRITE,&hPToken)) break;

							char dir[MAX_PATH];
							char exe_file_name[MAX_PATH];
							GetModuleFileName(0, exe_file_name, MAX_PATH);
							strcpy_s(dir, exe_file_name);
							strcat_s(dir, " -securityeditorhelper");
				
							{
								STARTUPINFO          StartUPInfo;
								PROCESS_INFORMATION  ProcessInfo;
								ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
								ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));
								StartUPInfo.wShowWindow = SW_SHOW;
								StartUPInfo.lpDesktop = "Winsta0\\Default";
								StartUPInfo.cb = sizeof(STARTUPINFO);
						
								CreateProcessAsUser(hPToken,NULL,dir,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&StartUPInfo,&ProcessInfo);
								DWORD errorcode=GetLastError();
                                if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
                                if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
								if (errorcode == 1314) goto error;
								break;
								error:
										winvncSecurityEditorHelper_as_admin();

							}
						}
				} else { 
					// Marscha@2004 - authSSP: end of change
					_this->m_vncauth.Init(_this->m_server);
					_this->m_vncauth.SetTemp(_this->m_Tempfile);
					_this->m_vncauth.Show(TRUE);
				}
			}
			return TRUE;
		case IDC_CHECKDRIVER:
			{
				CheckVideoDriver(1);
			}
			return TRUE;
		case IDC_PLUGIN_BUTTON:
			{
				HWND hPlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
				if (SendMessage(hPlugin, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					TCHAR szPlugin[MAX_PATH];
					GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
					PathStripPathA(szPlugin);

					if (!_this->m_server->GetDSMPluginPointer()->IsLoaded())
						_this->m_server->GetDSMPluginPointer()->LoadPlugin(szPlugin, false);
					else
					{
						// sf@2003 - We check if the loaded plugin is the same than
						// the currently selected one or not
						_this->m_server->GetDSMPluginPointer()->DescribePlugin();
						if (_stricmp(_this->m_server->GetDSMPluginPointer()->GetPluginFileName(), szPlugin))
						{
							_this->m_server->GetDSMPluginPointer()->UnloadPlugin();
							_this->m_server->GetDSMPluginPointer()->LoadPlugin(szPlugin, false);
						}
					}
				
					if (_this->m_server->GetDSMPluginPointer()->IsLoaded())
					{
						Secure_Save_Plugin_Config(szPlugin);
					}
					else
					{
						MessageBoxSecure(NULL, 
							sz_ID_PLUGIN_NOT_LOAD, 
							sz_ID_PLUGIN_LOADIN, MB_OK | MB_ICONEXCLAMATION );
					}
				}				
				return TRUE;
			}



		}
		break;
	}
	return 0;
}



// TightVNC 1.2.7
// Set display/port settings to the correct state
void
vncProperties::InitPortSettings(HWND hwnd)
{
	BOOL bConnectSock = m_server->SockConnected();
	BOOL bAutoPort = m_server->AutoPortSelect();
	UINT port_rfb = m_server->GetPort();
	UINT port_http = m_server->GetHttpPort();
	int d1 = PORT_TO_DISPLAY(port_rfb);
	int d2 = HPORT_TO_DISPLAY(port_http);
	BOOL bValidDisplay = (d1 == d2 && d1 >= 0 && d1 <= 99);

	CheckDlgButton(hwnd, IDC_PORTNO_AUTO,
		(bAutoPort) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_SPECDISPLAY,
		(!bAutoPort && bValidDisplay) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_SPECPORT,
		(!bAutoPort && !bValidDisplay) ? BST_CHECKED : BST_UNCHECKED);

	EnableWindow(GetDlgItem(hwnd, IDC_PORTNO_AUTO), bConnectSock);
	EnableWindow(GetDlgItem(hwnd, IDC_SPECDISPLAY), bConnectSock);
	EnableWindow(GetDlgItem(hwnd, IDC_SPECPORT), bConnectSock);

	if (bValidDisplay) {
		SetDlgItemInt(hwnd, IDC_DISPLAYNO, d1, FALSE);
	} else {
		SetDlgItemText(hwnd, IDC_DISPLAYNO, "");
	}
	SetDlgItemInt(hwnd, IDC_PORTRFB, port_rfb, FALSE);
	SetDlgItemInt(hwnd, IDC_PORTHTTP, port_http, FALSE);

	EnableWindow(GetDlgItem(hwnd, IDC_DISPLAYNO),
		bConnectSock && !bAutoPort && bValidDisplay);
	EnableWindow(GetDlgItem(hwnd, IDC_PORTRFB),
		bConnectSock && !bAutoPort && !bValidDisplay);
	EnableWindow(GetDlgItem(hwnd, IDC_PORTHTTP),
		bConnectSock && !bAutoPort && !bValidDisplay);
}


// Functions to load & save the settings
LONG
vncProperties::LoadInt(HKEY key, LPCSTR valname, LONG defval)
{
	LONG pref;
	ULONG type = REG_DWORD;
	ULONG prefsize = sizeof(pref);

	if (RegQueryValueEx(key,
		valname,
		NULL,
		&type,
		(LPBYTE) &pref,
		&prefsize) != ERROR_SUCCESS)
		return defval;

	if (type != REG_DWORD)
		return defval;

	if (prefsize != sizeof(pref))
		return defval;

	return pref;
}

void
vncProperties::LoadPassword(HKEY key, char *buffer)
{
	DWORD type = REG_BINARY;
	int slen=MAXPWLEN;
	char inouttext[MAXPWLEN];

	// Retrieve the encrypted password
	if (RegQueryValueEx(key,
		"Password",
		NULL,
		&type,
		(LPBYTE) &inouttext,
		(LPDWORD) &slen) != ERROR_SUCCESS)
		return;

	if (slen > MAXPWLEN)
		return;

	memcpy(buffer, inouttext, MAXPWLEN);
}

void //PGM
vncProperties::LoadPassword2(HKEY key, char *buffer) //PGM
{ //PGM
	DWORD type = REG_BINARY; //PGM
	int slen=MAXPWLEN; //PGM
	char inouttext[MAXPWLEN]; //PGM

	// Retrieve the encrypted password //PGM
	if (RegQueryValueEx(key, //PGM
		"Password2", //PGM
		NULL, //PGM
		&type, //PGM
		(LPBYTE) &inouttext, //PGM
		(LPDWORD) &slen) != ERROR_SUCCESS) //PGM
		return; //PGM

	if (slen > MAXPWLEN) //PGM
		return; //PGM

	memcpy(buffer, inouttext, MAXPWLEN); //PGM
} //PGM

char *
vncProperties::LoadString(HKEY key, LPCSTR keyname)
{
	DWORD type = REG_SZ;
	DWORD buflen = 0;
	BYTE *buffer = 0;

	// Get the length of the AuthHosts string
	if (RegQueryValueEx(key,
		keyname,
		NULL,
		&type,
		NULL,
		&buflen) != ERROR_SUCCESS)
		return 0;

	if (type != REG_SZ)
		return 0;
	buffer = new BYTE[buflen];
	if (buffer == 0)
		return 0;

	// Get the AuthHosts string data
	if (RegQueryValueEx(key,
		keyname,
		NULL,
		&type,
		buffer,
		&buflen) != ERROR_SUCCESS) {
		delete [] buffer;
		return 0;
	}

	// Verify the type
	if (type != REG_SZ) {
		delete [] buffer;
		return 0;
	}

	return (char *)buffer;
}


void
vncProperties::ResetRegistry()
{	
	char username[UNLEN+1];
	HKEY hkLocal, hkLocalUser, hkDefault;
	DWORD dw;

	if (!vncService::CurrentUser((char *)&username, sizeof(username)))
		return;

	// If there is no user logged on them default to SYSTEM
	if (strcmp(username, "") == 0)
		strcpy_s((char *)&username, UNLEN+1, "SYSTEM");

	// Try to get the machine registry key for WinVNC
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		WINVNC_REGISTRY_KEY,
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		{
		hkLocalUser=NULL;
		hkDefault=NULL;
		goto LABELUSERSETTINGS;
		}

	// Now try to get the per-user local key
	if (RegOpenKeyEx(hkLocal,
		username,
		0, KEY_READ,
		&hkLocalUser) != ERROR_SUCCESS)
		hkLocalUser = NULL;

	// Get the default key
	if (RegCreateKeyEx(hkLocal,
		"Default",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ,
		NULL,
		&hkDefault,
		&dw) != ERROR_SUCCESS)
		hkDefault = NULL;

	if (hkLocalUser != NULL) RegCloseKey(hkLocalUser);
	if (hkDefault != NULL) RegCloseKey(hkDefault);
	if (hkLocal != NULL) RegCloseKey(hkLocal);
	RegCloseKey(HKEY_LOCAL_MACHINE);
LABELUSERSETTINGS:
	if ((strcmp(username, "SYSTEM") != 0))
		{
			HKEY hkGlobalUser;
			if (RegCreateKeyEx(HKEY_CURRENT_USER,
				WINVNC_REGISTRY_KEY,
				0, REG_NONE, REG_OPTION_NON_VOLATILE,
				KEY_READ, NULL, &hkGlobalUser, &dw) == ERROR_SUCCESS)
			{
				RegCloseKey(hkGlobalUser);
				RegCloseKey(HKEY_CURRENT_USER);
			}
		}

}

void
vncProperties::Load(BOOL usersettings)
{
	vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Entering Load\n"));

	//if (m_dlgvisible) {
	//	vnclog.Print(LL_INTWARN, VNCLOG("service helper invoked while Properties panel displayed\n"));
	//	return;
	//}
	ResetRegistry();

	if (vncService::RunningAsService()) usersettings=false;

	// sf@2007 - Vista mode
	// The WinVNC service mode is not used under Vista (due to Session0 isolation)
	// Default settings (Service mode) are used when WinVNC app in run under Vista login screen
	// User settings (loggued user mode) are used when WinVNC app in run in a user session
	// Todo: Maybe we should additionally check OS version...
	if (m_server->RunningFromExternalService())
		usersettings=false;

	m_usersettings = usersettings;

	if (m_usersettings)
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - User mode\n"));
	else
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Service mode\n"));
	
	char username[UNLEN+1];
	HKEY hkLocal, hkLocalUser, hkDefault;
	DWORD dw;
	
	// NEW (R3) PREFERENCES ALGORITHM
	// 1.	Look in HKEY_LOCAL_MACHINE/Software/ORL/WinVNC3/%username%
	//		for sysadmin-defined, user-specific settings.
	// 2.	If not found, fall back to %username%=Default
	// 3.	If AllowOverrides is set then load settings from
	//		HKEY_CURRENT_USER/Software/ORL/WinVNC3

	// GET THE CORRECT KEY TO READ FROM

	// Get the user name / service name
	if (!vncService::CurrentUser((char *)&username, sizeof(username)))
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - NO current user\n"));
		return;
	}

	// If there is no user logged on them default to SYSTEM
	if (strcmp(username, "") == 0)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Force USER SYSTEM 1\n"));
		strcpy_s((char *)&username, UNLEN+1, "SYSTEM");
	}


	vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - UserName = %s\n"), username);

	// Try to get the machine registry key for WinVNC
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		WINVNC_REGISTRY_KEY,
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		{
		hkLocalUser=NULL;
		hkDefault=NULL;
		goto LABELUSERSETTINGS;
		}

	// Now try to get the per-user local key
	if (RegOpenKeyEx(hkLocal,
		username,
		0, KEY_READ,
		&hkLocalUser) != ERROR_SUCCESS)
		hkLocalUser = NULL;

	// Get the default key
	if (RegCreateKeyEx(hkLocal,
		"Default",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ,
		NULL,
		&hkDefault,
		&dw) != ERROR_SUCCESS)
		hkDefault = NULL;

	// LOAD THE MACHINE-LEVEL PREFS

	vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Machine level prefs\n"));

	// Logging/debugging prefs
	vnclog.Print(LL_INTINFO, VNCLOG("loading local-only settings\n"));
	//vnclog.SetMode(LoadInt(hkLocal, "DebugMode", 0));
	//vnclog.SetLevel(LoadInt(hkLocal, "DebugLevel", 0));

	// Disable Tray Icon
	m_server->SetDisableTrayIcon(LoadInt(hkLocal, "DisableTrayIcon", false));
	m_server->SetRdpmode(LoadInt(hkLocal, "rdpmode", 0));
	m_server->SetNoScreensaver(LoadInt(hkLocal, "noscreensaver", 0));

	// Authentication required, loopback allowed, loopbackOnly

	m_server->SetLoopbackOnly(LoadInt(hkLocal, "LoopbackOnly", false));

	m_pref_Secure = false;
	m_pref_Secure = LoadInt(hkLocal, "Secure", m_pref_Secure);
	m_server->Secure(m_pref_Secure);

	m_pref_RequireMSLogon=false;
	m_pref_RequireMSLogon = LoadInt(hkLocal, "MSLogonRequired", m_pref_RequireMSLogon);
	m_server->RequireMSLogon(m_pref_RequireMSLogon);

	// Marscha@2004 - authSSP: added NewMSLogon checkbox to admin props page
	m_pref_NewMSLogon = false;
	m_pref_NewMSLogon = LoadInt(hkLocal, "NewMSLogon", m_pref_NewMSLogon);
	m_server->SetNewMSLogon(m_pref_NewMSLogon);

	m_pref_ReverseAuthRequired = true;
	m_pref_ReverseAuthRequired = LoadInt(hkLocal, "ReverseAuthRequired", m_pref_ReverseAuthRequired);
	m_server->SetReverseAuthRequired(m_pref_ReverseAuthRequired);

	// sf@2003 - Moved DSM params here
	m_pref_UseDSMPlugin=false;
	m_pref_UseDSMPlugin = LoadInt(hkLocal, "UseDSMPlugin", m_pref_UseDSMPlugin);
	LoadDSMPluginName(hkLocal, m_pref_szDSMPlugin);	
	
	//adzm 2010-05-12 - dsmplugin config
	{
		char* szBuffer = LoadString(hkLocal, "DSMPluginConfig");
		if (szBuffer) {
			strncpy_s(m_pref_DSMPluginConfig, sizeof(m_pref_DSMPluginConfig) - 1, szBuffer, _TRUNCATE);
			delete[] szBuffer;
		} else {
			m_pref_DSMPluginConfig[0] = '\0';
		}
	}
#ifdef IPV6V4
	m_server->SetIPV6(LoadInt(hkLocal, "UseIpv6", true));
#endif
	if (m_server->LoopbackOnly()) m_server->SetLoopbackOk(true);
	else m_server->SetLoopbackOk(LoadInt(hkLocal, "AllowLoopback", true));
	m_server->SetAuthRequired(LoadInt(hkLocal, "AuthRequired", true));

	m_server->SetConnectPriority(LoadInt(hkLocal, "ConnectPriority", 0));
	if (!m_server->LoopbackOnly())
	{
		char *authhosts = LoadString(hkLocal, "AuthHosts");
		if (authhosts != 0) {
			m_server->SetAuthHosts(authhosts);
			delete [] authhosts;
		} else {
			m_server->SetAuthHosts(0);
		}
	} else {
		m_server->SetAuthHosts(0);
	}

	// If Socket connections are allowed, should the HTTP server be enabled?
LABELUSERSETTINGS:
	// LOAD THE USER PREFERENCES
	//vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Load User Preferences\n"));

	// Set the default user prefs
	vnclog.Print(LL_INTINFO, VNCLOG("clearing user settings\n"));
	m_pref_AutoPortSelect=TRUE;
    m_pref_HTTPConnect = TRUE;
	m_pref_PortNumber = RFB_PORT_OFFSET; 
	m_pref_SockConnect=TRUE;
	{
	    vncPasswd::FromClear crypt(m_pref_Secure);
	    memcpy(m_pref_passwd, crypt, MAXPWLEN);
	}
	m_pref_QuerySetting=2;
	m_pref_QueryTimeout=10;
	m_pref_QueryDisableTime=0;
	m_pref_QueryAccept=0;
	m_pref_IdleTimeout=0;
	m_pref_MaxViewerSetting = 0;
	m_pref_MaxViewers = 128;
	m_pref_Collabo = false;

	m_pref_Frame = FALSE;
	m_pref_Notification = FALSE;
	m_pref_OSD = FALSE;
	m_pref_NotificationSelection = 0;

	m_pref_EnableRemoteInputs=TRUE;
	m_pref_DisableLocalInputs=FALSE;
	m_pref_EnableJapInput=FALSE;
	m_pref_EnableUnicodeInput=FALSE;
	m_pref_EnableWin8Helper=FALSE;
	m_pref_clearconsole=FALSE;
	m_pref_LockSettings=-1;

	m_pref_RemoveWallpaper=FALSE;
	// adzm - 2010-07 - Disable more effects or font smoothing
	m_pref_RemoveEffects=FALSE;
	m_pref_RemoveFontSmoothing=FALSE;
    m_alloweditclients = TRUE;
	m_allowshutdown = TRUE;
	m_allowproperties = TRUE;
	m_allowInjection = FALSE;

	// Modif sf@2002
	// [v1.0.2-jp2 fix] Move to vncpropertiesPoll.cpp
//	m_pref_SingleWindow = FALSE;
	m_pref_UseDSMPlugin = FALSE;
	*m_pref_szDSMPlugin = '\0';
	m_pref_DSMPluginConfig[0] = '\0';

	m_pref_EnableFileTransfer = TRUE;
	m_pref_FTUserImpersonation = TRUE;
	m_pref_EnableBlankMonitor = TRUE;
	m_pref_BlankInputsOnly = FALSE;
	m_pref_QueryIfNoLogon = FALSE;
	m_pref_DefaultScale = 1;

	// Load the local prefs for this user
	if (hkDefault != NULL)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Local Preferences - Default\n"));

		vnclog.Print(LL_INTINFO, VNCLOG("loading DEFAULT local settings\n"));
		LoadUserPrefs(hkDefault);
		m_allowshutdown = LoadInt(hkDefault, "AllowShutdown", m_allowshutdown);
		m_allowproperties = LoadInt(hkDefault, "AllowProperties", m_allowproperties);
		m_allowInjection = LoadInt(hkDefault, "AllowInjection", m_allowInjection);
		m_alloweditclients = LoadInt(hkDefault, "AllowEditClients", m_alloweditclients);
	}

	// Are we being asked to load the user settings, or just the default local system settings?
	if (usersettings)
	{
		// We want the user settings, so load them!
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - User Settings on\n"));

		if (hkLocalUser != NULL)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - LoadUser Preferences\n"));

			vnclog.Print(LL_INTINFO, VNCLOG("loading \"%s\" local settings\n"), username);
			LoadUserPrefs(hkLocalUser);
			m_allowshutdown = LoadInt(hkLocalUser, "AllowShutdown", m_allowshutdown);
			m_allowproperties = LoadInt(hkLocalUser, "AllowProperties", m_allowproperties);
			m_allowInjection = LoadInt(hkLocalUser, "AllowInjection", m_allowInjection);
		  m_alloweditclients = LoadInt(hkLocalUser, "AllowEditClients", m_alloweditclients);
		}

		// Now override the system settings with the user's settings
		// If the username is SYSTEM then don't try to load them, because there aren't any...
		if (m_allowproperties && (strcmp(username, "SYSTEM") != 0))
		{
			vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Override system settings with users settings\n"));
			HKEY hkGlobalUser;
			if (RegCreateKeyEx(HKEY_CURRENT_USER,
				WINVNC_REGISTRY_KEY,
				0, REG_NONE, REG_OPTION_NON_VOLATILE,
				KEY_READ, NULL, &hkGlobalUser, &dw) == ERROR_SUCCESS)
			{
				vnclog.Print(LL_INTINFO, VNCLOG("loading \"%s\" global settings\n"), username);
				LoadUserPrefs(hkGlobalUser);
				RegCloseKey(hkGlobalUser);

				// Close the user registry hive so it can unload if required
				RegCloseKey(HKEY_CURRENT_USER);
			}
		}
	} else {
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - User Settings off\n"));
		if (hkLocalUser != NULL)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("loading \"%s\" local settings\n"), username);
			LoadUserPrefs(hkLocalUser);
			m_allowshutdown = LoadInt(hkLocalUser, "AllowShutdown", m_allowshutdown);
			m_allowproperties = LoadInt(hkLocalUser, "AllowProperties", m_allowproperties);
			m_allowInjection = LoadInt(hkLocalUser, "AllowInjection", m_allowInjection);
		    m_alloweditclients = LoadInt(hkLocalUser, "AllowEditClients", m_alloweditclients);			
		}
		vnclog.Print(LL_INTINFO, VNCLOG("bypassing user-specific settings (both local and global)\n"));
	}

	if (hkLocalUser != NULL) RegCloseKey(hkLocalUser);
	if (hkDefault != NULL) RegCloseKey(hkDefault);
	if (hkLocal != NULL) RegCloseKey(hkLocal);

	// Make the loaded settings active..
	ApplyUserPrefs();
}

void
vncProperties::LoadUserPrefs(HKEY appkey)
{
	// LOAD USER PREFS FROM THE SELECTED KEY

	// Modif sf@2002
	m_pref_EnableFileTransfer = LoadInt(appkey, "FileTransferEnabled", m_pref_EnableFileTransfer);
	m_pref_FTUserImpersonation = LoadInt(appkey, "FTUserImpersonation", m_pref_FTUserImpersonation); // sf@2005
	m_pref_EnableBlankMonitor = LoadInt(appkey, "BlankMonitorEnabled", m_pref_EnableBlankMonitor);
	m_pref_BlankInputsOnly = LoadInt(appkey, "BlankInputsOnly", m_pref_BlankInputsOnly); //PGM
	m_pref_DefaultScale = LoadInt(appkey, "DefaultScale", m_pref_DefaultScale);
	
	m_pref_Primary=LoadInt(appkey, "primary", m_pref_Primary);
	m_pref_Secondary=LoadInt(appkey, "secondary", m_pref_Secondary);

	m_pref_UseDSMPlugin = LoadInt(appkey, "UseDSMPlugin", m_pref_UseDSMPlugin);
	LoadDSMPluginName(appkey, m_pref_szDSMPlugin);

	// Connection prefs
	m_pref_SockConnect=LoadInt(appkey, "SocketConnect", m_pref_SockConnect);
	m_pref_HTTPConnect=LoadInt(appkey, "HTTPConnect", m_pref_HTTPConnect);
	m_pref_AutoPortSelect=LoadInt(appkey, "AutoPortSelect", m_pref_AutoPortSelect);
	m_pref_PortNumber=LoadInt(appkey, "PortNumber", m_pref_PortNumber);
	m_pref_HttpPortNumber=LoadInt(appkey, "HTTPPortNumber",
									DISPLAY_TO_HPORT(PORT_TO_DISPLAY(m_pref_PortNumber)));
	m_pref_IdleTimeout=LoadInt(appkey, "IdleTimeout", m_pref_IdleTimeout);
	
	m_pref_RemoveWallpaper=LoadInt(appkey, "RemoveWallpaper", m_pref_RemoveWallpaper);
	// adzm - 2010-07 - Disable more effects or font smoothing
	m_pref_RemoveEffects=LoadInt(appkey, "RemoveEffects", m_pref_RemoveEffects);
	m_pref_RemoveFontSmoothing=LoadInt(appkey, "RemoveFontSmoothing", m_pref_RemoveFontSmoothing);

	// Connection querying settings
	m_pref_QuerySetting=LoadInt(appkey, "QuerySetting", m_pref_QuerySetting);
	m_server->SetQuerySetting(m_pref_QuerySetting);
	m_pref_QueryTimeout=LoadInt(appkey, "QueryTimeout", m_pref_QueryTimeout);
	m_server->SetQueryTimeout(m_pref_QueryTimeout);
	m_pref_QueryDisableTime=LoadInt(appkey, "QueryDisableTime", m_pref_QueryDisableTime);
	m_server->SetQueryDisableTime(m_pref_QueryDisableTime);
	m_pref_QueryAccept=LoadInt(appkey, "QueryAccept", m_pref_QueryAccept);
	m_server->SetQueryAccept(m_pref_QueryAccept);

	m_pref_MaxViewerSetting = LoadInt(appkey, "MaxViewerSetting", m_pref_MaxViewerSetting);
	m_server->setMaxViewerSetting(m_pref_MaxViewerSetting);
	m_pref_MaxViewers = LoadInt(appkey, "MaxViewers", m_pref_MaxViewers);
	m_server->setMaxViewers(m_pref_MaxViewers);	
	m_pref_Collabo = LoadInt(appkey, "Collabo", m_pref_Collabo);
	m_server->setCollabo(m_pref_Collabo);

	m_pref_Frame = LoadInt(appkey, "Frame", m_pref_Frame);
	m_server->setFrame(m_pref_Frame);
	m_pref_Notification = LoadInt(appkey, "Notification", m_pref_Notification);
	m_server->setNotification(m_pref_Notification);
	m_pref_OSD = LoadInt(appkey, "OSD", m_pref_OSD);
	m_server->setOSD(m_pref_OSD);
	m_pref_NotificationSelection = LoadInt(appkey, "NotificationSelection", m_pref_NotificationSelection);
	m_server->setNotificationSelection(m_pref_NotificationSelection);
	

	// marscha@2006 - Is AcceptDialog required even if no user is logged on
	m_pref_QueryIfNoLogon=LoadInt(appkey, "QueryIfNoLogon", m_pref_QueryIfNoLogon);
	m_server->SetQueryIfNoLogon(m_pref_QueryIfNoLogon);

	// Load the password
	LoadPassword(appkey, m_pref_passwd);
	LoadPassword2(appkey, m_pref_passwd2); //PGM

	// Remote access prefs
	m_pref_EnableRemoteInputs=LoadInt(appkey, "InputsEnabled", m_pref_EnableRemoteInputs);
	m_pref_LockSettings=LoadInt(appkey, "LockSetting", m_pref_LockSettings);
	m_pref_DisableLocalInputs=LoadInt(appkey, "LocalInputsDisabled", m_pref_DisableLocalInputs);
	m_pref_EnableJapInput=LoadInt(appkey, "EnableJapInput", m_pref_EnableJapInput);
	m_pref_EnableUnicodeInput=LoadInt(appkey, "EnableUnicodeInput", m_pref_EnableUnicodeInput);
	m_pref_EnableWin8Helper=LoadInt(appkey, "EnableWin8Helper", m_pref_EnableWin8Helper);
	m_pref_clearconsole=LoadInt(appkey, "clearconsole", m_pref_clearconsole);
}

void
vncProperties::ApplyUserPrefs()
{
	// APPLY THE CACHED PREFERENCES TO THE SERVER

	// Modif sf@2002
	m_server->EnableFileTransfer(m_pref_EnableFileTransfer);
	m_server->FTUserImpersonation(m_pref_FTUserImpersonation); // sf@2005
	m_server->Primary(m_pref_Primary);
	m_server->Secondary(m_pref_Secondary);

	m_server->BlankMonitorEnabled(m_pref_EnableBlankMonitor);
	m_server->BlankInputsOnly(m_pref_BlankInputsOnly); //PGM
	m_server->SetDefaultScale(m_pref_DefaultScale);

	// Update the connection querying settings
	m_server->SetQuerySetting(m_pref_QuerySetting);
	m_server->SetQueryTimeout(m_pref_QueryTimeout);
	m_server->SetQueryDisableTime(m_pref_QueryDisableTime);
	m_server->SetQueryAccept(m_pref_QueryAccept);
	m_server->setMaxViewerSetting(m_pref_MaxViewerSetting);
	m_server->setMaxViewers(m_pref_MaxViewers);
	m_server->setCollabo(m_pref_Collabo);

	m_server->setFrame(m_pref_Frame);
	m_server->setNotification(m_pref_Notification);
	m_server->setOSD(m_pref_OSD);
	m_server->setNotificationSelection(m_pref_NotificationSelection);

	m_server->SetAutoIdleDisconnectTimeout(m_pref_IdleTimeout);
	m_server->EnableRemoveWallpaper(m_pref_RemoveWallpaper);
	// adzm - 2010-07 - Disable more effects or font smoothing
	m_server->EnableRemoveFontSmoothing(m_pref_RemoveFontSmoothing);
	m_server->EnableRemoveEffects(m_pref_RemoveEffects);

	// Is the listening socket closing?

	if (!m_pref_SockConnect)
		m_server->SockConnect(m_pref_SockConnect);

	m_server->EnableHTTPConnect(m_pref_HTTPConnect);

	// Are inputs being disabled?
	if (!m_pref_EnableRemoteInputs)
		m_server->EnableRemoteInputs(m_pref_EnableRemoteInputs);
	if (m_pref_DisableLocalInputs)
		m_server->DisableLocalInputs(m_pref_DisableLocalInputs);
	if (m_pref_EnableJapInput)
		m_server->EnableJapInput(m_pref_EnableJapInput);
	if (m_pref_EnableUnicodeInput)
		m_server->EnableUnicodeInput(m_pref_EnableUnicodeInput);
	if (m_pref_EnableWin8Helper)
		m_server->Win8HelperEnabled(m_pref_EnableWin8Helper);
	m_server->Clearconsole(m_pref_clearconsole);

	// Update the password
	m_server->SetPassword(m_pref_passwd);
	m_server->SetPassword2(m_pref_passwd2); //PGM

	// Now change the listening port settings
	m_server->SetAutoPortSelect(m_pref_AutoPortSelect);
	if (!m_pref_AutoPortSelect)
		// m_server->SetPort(m_pref_PortNumber);
		m_server->SetPorts(m_pref_PortNumber, m_pref_HttpPortNumber); // Tight 1.2.7

	m_server->SockConnect(m_pref_SockConnect);

	// Remote access prefs
	m_server->EnableRemoteInputs(m_pref_EnableRemoteInputs);
	m_server->SetLockSettings(m_pref_LockSettings);
	m_server->DisableLocalInputs(m_pref_DisableLocalInputs);
	m_server->EnableJapInput(m_pref_EnableJapInput);
	m_server->EnableUnicodeInput(m_pref_EnableUnicodeInput);
	m_server->Win8HelperEnabled(m_pref_EnableWin8Helper);
	m_server->Clearconsole(m_pref_clearconsole);
	// DSM Plugin prefs
	m_server->EnableDSMPlugin(m_pref_UseDSMPlugin);
	m_server->SetDSMPluginName(m_pref_szDSMPlugin);
	
	//adzm 2010-05-12 - dsmplugin config
	m_server->SetDSMPluginConfig(m_pref_DSMPluginConfig);

	if (m_server->IsDSMPluginEnabled()) 
		m_server->SetDSMPlugin(false);
}

void
vncProperties::SaveInt(HKEY key, LPCSTR valname, LONG val)
{
	RegSetValueEx(key, valname, 0, REG_DWORD, (LPBYTE) &val, sizeof(val));
}

void
vncProperties::SavePassword(HKEY key, char *buffer)
{
	RegSetValueEx(key, "Password", 0, REG_BINARY, (LPBYTE) buffer, MAXPWLEN);
}
void //PGM
vncProperties::SavePassword2(HKEY key, char *buffer) //PGM
{ //PGM
	RegSetValueEx(key, "Password2", 0, REG_BINARY, (LPBYTE) buffer, MAXPWLEN); //PGM
} //PGM
void
vncProperties::SaveString(HKEY key,LPCSTR valname, const char *buffer)
{
	RegSetValueEx(key, valname, 0, REG_BINARY, (LPBYTE) buffer, (DWORD)(strlen(buffer)+1));
}

void
vncProperties::SaveDSMPluginName(HKEY key, char *buffer)
{
	RegSetValueEx(key, "DSMPlugin", 0, REG_BINARY, (LPBYTE) buffer, MAXPATH);
}

void
vncProperties::LoadDSMPluginName(HKEY key, char *buffer)
{
	DWORD type = REG_BINARY;
	int slen=MAXPATH;
	char inouttext[MAXPATH];

	if (RegQueryValueEx(key,
		"DSMPlugin",
		NULL,
		&type,
		(LPBYTE) &inouttext,
		(LPDWORD) &slen) != ERROR_SUCCESS)
		return;

	if (slen > MAXPATH)
		return;

	memcpy(buffer, inouttext, MAXPATH);
}

void
vncProperties::Save()
{
	HKEY appkey;
	DWORD dw;

	if (!m_allowproperties)
		return;

	// NEW (R3) PREFERENCES ALGORITHM
	// The user's prefs are only saved if the user is allowed to override
	// the machine-local settings specified for them.  Otherwise, the
	// properties entry on the tray icon menu will be greyed out.

	// GET THE CORRECT KEY TO READ FROM

	// Have we loaded user settings, or system settings?
	if (m_usersettings) {
		// Verify that we know who is logged on
		char username[UNLEN+1];
		if (!vncService::CurrentUser((char *)&username, sizeof(username)))
			return;
		if (strcmp(username, "") == 0)
			return;

		// Try to get the per-user, global registry key for WinVNC
		if (RegCreateKeyEx(HKEY_CURRENT_USER,
			WINVNC_REGISTRY_KEY,
			0, REG_NONE, REG_OPTION_NON_VOLATILE,
			KEY_WRITE | KEY_READ, NULL, &appkey, &dw) != ERROR_SUCCESS)
			return;
	} else {
		// Try to get the default local registry key for WinVNC
		HKEY hkLocal;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			WINVNC_REGISTRY_KEY,
			0, REG_NONE, REG_OPTION_NON_VOLATILE,
			KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS) {
			MessageBoxSecure(NULL, sz_ID_MB1, sz_ID_WVNC, MB_OK);
			return;
		}

		if (RegCreateKeyEx(hkLocal,
			"Default",
			0, REG_NONE, REG_OPTION_NON_VOLATILE,
			KEY_WRITE | KEY_READ, NULL, &appkey, &dw) != ERROR_SUCCESS) {
			RegCloseKey(hkLocal);
			return;
		}
		RegCloseKey(hkLocal);
	}

	// SAVE PER-USER PREFS IF ALLOWED
	SaveUserPrefs(appkey);
	RegCloseKey(appkey);
	RegCloseKey(HKEY_CURRENT_USER);

	// Machine Preferences
	// Get the machine registry key for WinVNC
	HKEY hkLocal,hkDefault;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		WINVNC_REGISTRY_KEY,
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_WRITE | KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		return;
	if (RegCreateKeyEx(hkLocal,
		"Default",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_WRITE | KEY_READ,
		NULL,
		&hkDefault,
		&dw) != ERROR_SUCCESS)
		hkDefault = NULL;
	// sf@2003
	SaveInt(hkLocal, "DebugMode", vnclog.GetMode());
	SaveInt(hkLocal, "Avilog", vnclog.GetVideo());
	SaveString(hkLocal, "path", vnclog.GetPath());
	SaveInt(hkLocal, "DebugLevel", vnclog.GetLevel());
	SaveInt(hkLocal, "AllowLoopback", m_server->LoopbackOk());
#ifdef IPV6V4
	SaveInt(hkLocal, "UseIpv6", m_server->IPV6());
#endif
	SaveInt(hkLocal, "LoopbackOnly", m_server->LoopbackOnly());
	if (hkDefault) 
		{
			SaveInt(hkDefault, "AllowShutdown", m_allowshutdown);
			SaveInt(hkDefault, "AllowProperties",  m_allowproperties);
			SaveInt(hkDefault, "AllowEditClients", m_alloweditclients);
		}

	SaveInt(hkLocal, "DisableTrayIcon", m_server->GetDisableTrayIcon());
	SaveInt(hkLocal, "rdpmode", m_server->GetRdpmode());
	SaveInt(hkLocal, "noscreensaver", m_server->GetNoScreensaver());
	SaveInt(hkLocal, "Secure", m_server->Secure());
	SaveInt(hkLocal, "MSLogonRequired", m_server->MSLogonRequired());
	// Marscha@2004 - authSSP: save "New MS-Logon" state
	SaveInt(hkLocal, "NewMSLogon", m_server->GetNewMSLogon());
	SaveInt(hkLocal, "ReverseAuthRequired", m_server->GetReverseAuthRequired());
	// sf@2003 - DSM params here
	SaveInt(hkLocal, "UseDSMPlugin", m_server->IsDSMPluginEnabled());
	SaveInt(hkLocal, "ConnectPriority", m_server->ConnectPriority());
	SaveDSMPluginName(hkLocal, m_server->GetDSMPluginName());	
	
	//adzm 2010-05-12 - dsmplugin config
	SaveString(hkLocal, "DSMPluginConfig", m_server->GetDSMPluginConfig());

	if (hkDefault) RegCloseKey(hkDefault);
	if (hkLocal) RegCloseKey(hkLocal);
}

void
vncProperties::SaveUserPrefs(HKEY appkey)
{
	// SAVE THE PER USER PREFS
	vnclog.Print(LL_INTINFO, VNCLOG("saving current settings to registry\n"));

	// Modif sf@2002
	SaveInt(appkey, "FileTransferEnabled", m_server->FileTransferEnabled());
	SaveInt(appkey, "FTUserImpersonation", m_server->FTUserImpersonation()); // sf@2005
	SaveInt(appkey, "BlankMonitorEnabled", m_server->BlankMonitorEnabled());
	SaveInt(appkey, "BlankInputsOnly", m_server->BlankInputsOnly()); //PGM
	SaveInt(appkey, "primary", m_server->Primary());
	SaveInt(appkey, "secondary", m_server->Secondary());

	SaveInt(appkey, "DefaultScale", m_server->GetDefaultScale());

	SaveInt(appkey, "UseDSMPlugin", m_server->IsDSMPluginEnabled());
	SaveDSMPluginName(appkey, m_server->GetDSMPluginName());
	//adzm 2010-05-12 - dsmplugin config
	SaveString(appkey, "DSMPluginConfig", m_server->GetDSMPluginConfig());

	// Connection prefs
	SaveInt(appkey, "SocketConnect", m_server->SockConnected());
	SaveInt(appkey, "HTTPConnect", m_server->HTTPConnectEnabled());
	SaveInt(appkey, "AutoPortSelect", m_server->AutoPortSelect());
	if (!m_server->AutoPortSelect()) {
		SaveInt(appkey, "PortNumber", m_server->GetPort());
		SaveInt(appkey, "HTTPPortNumber", m_server->GetHttpPort());
	}
	SaveInt(appkey, "InputsEnabled", m_server->RemoteInputsEnabled());
	SaveInt(appkey, "LocalInputsDisabled", m_server->LocalInputsDisabled());
	SaveInt(appkey, "IdleTimeout", m_server->AutoIdleDisconnectTimeout());
	SaveInt(appkey, "EnableJapInput", m_server->JapInputEnabled());
	SaveInt(appkey, "EnableUnicodeInput", m_server->UnicodeInputEnabled());
	SaveInt(appkey, "EnableWin8Helper", m_server->Win8HelperEnabled());

	// Connection querying settings
	SaveInt(appkey, "QuerySetting", m_server->QuerySetting());
	SaveInt(appkey, "QueryTimeout", m_server->QueryTimeout());
	SaveInt(appkey, "QueryDisableTime", m_server->QueryDisableTime());
	SaveInt(appkey, "QueryAccept", m_server->QueryAcceptForSave());
	SaveInt(appkey, "MaxViewerSetting", m_server->getMaxViewerSetting());
	SaveInt(appkey, "MaxViewers", m_server->getMaxViewers());
	SaveInt(appkey, "Collabo", m_server->getCollabo());
	SaveInt(appkey, "Frame", m_server->getFrame());
	SaveInt(appkey, "Notification", m_server->getNotification());
	SaveInt(appkey, "OSD", m_server->getOSD());
	SaveInt(appkey, "NotificationSelection", m_server->getNotificationSelection());


	// Lock settings
	SaveInt(appkey, "LockSetting", m_server->LockSettings());

	// Wallpaper removal
	SaveInt(appkey, "RemoveWallpaper", m_server->RemoveWallpaperEnabled());
	// UI Effects
	// adzm - 2010-07 - Disable more effects or font smoothing
	SaveInt(appkey, "RemoveEffects", m_server->RemoveEffectsEnabled());
	SaveInt(appkey, "RemoveFontSmoothing", m_server->RemoveFontSmoothingEnabled());

	// Save the password
	char passwd[MAXPWLEN];
	m_server->GetPassword(passwd);
	SavePassword(appkey, passwd);
	memset(passwd, '\0', MAXPWLEN); //PGM
	m_server->GetPassword2(passwd); //PGM
	SavePassword2(appkey, passwd); //PGM
}


// ********************************************************************
// Ini file part - Wwill replace registry access completely, some day
// WARNING: until then, when adding/modifying a config parameter
//          don't forget to modify both ini file & registry parts !
// ********************************************************************

void vncProperties::LoadFromIniFile()
{
	//if (m_dlgvisible)
	//{
	//	vnclog.Print(LL_INTWARN, VNCLOG("service helper invoked while Properties panel displayed\n"));
	//	return;
	//}

	char username[UNLEN+1];

	// Get the user name / service name
	if (!vncService::CurrentUser((char *)&username, sizeof(username)))
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - NO current user\n"));
		return;
	}

	// If there is no user logged on them default to SYSTEM
	if (strcmp(username, "") == 0)
	{
		//vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Force USER SYSTEM 2\n"));
		strcpy_s((char *)&username, UNLEN+1, "SYSTEM");
	}

	// Logging/debugging prefs
	vnclog.SetMode(myIniFile.ReadInt("admin", "DebugMode", 0));
	char temp[512];
	myIniFile.ReadString("admin", "path", temp,512);	
	vnclog.SetPath(temp);
	vnclog.SetLevel(myIniFile.ReadInt("admin", "DebugLevel", 0));
	vnclog.SetVideo(myIniFile.ReadInt("admin", "Avilog", 0) ? true : false);

	// Disable Tray Icon
	m_server->SetDisableTrayIcon(myIniFile.ReadInt("admin", "DisableTrayIcon", false));
	m_server->SetRdpmode(myIniFile.ReadInt("admin", "rdpmode", 0));
	m_server->SetNoScreensaver(myIniFile.ReadInt("admin", "noscreensaver", 0));

	// Authentication required, loopback allowed, loopbackOnly

	m_server->SetLoopbackOnly(myIniFile.ReadInt("admin", "LoopbackOnly", false));

	m_pref_Secure = false;
	m_pref_Secure = myIniFile.ReadInt("admin", "Secure", m_pref_Secure);
	m_server->Secure(m_pref_Secure);

	m_pref_RequireMSLogon=false;
	m_pref_RequireMSLogon = myIniFile.ReadInt("admin", "MSLogonRequired", m_pref_RequireMSLogon);
	m_server->RequireMSLogon(m_pref_RequireMSLogon);

	// Marscha@2004 - authSSP: added NewMSLogon checkbox to admin props page
	m_pref_NewMSLogon = false;
	m_pref_NewMSLogon = myIniFile.ReadInt("admin", "NewMSLogon", m_pref_NewMSLogon);
	m_server->SetNewMSLogon(m_pref_NewMSLogon);

	// sf@2003 - Moved DSM params here
	m_pref_UseDSMPlugin=false;
	m_pref_UseDSMPlugin = myIniFile.ReadInt("admin", "UseDSMPlugin", m_pref_UseDSMPlugin);
	myIniFile.ReadString("admin", "DSMPlugin",m_pref_szDSMPlugin,128);

	m_pref_ReverseAuthRequired = true;
	m_pref_ReverseAuthRequired = myIniFile.ReadInt("admin", "ReverseAuthRequired", m_pref_ReverseAuthRequired);
	m_server->SetReverseAuthRequired(m_pref_ReverseAuthRequired);
	
	//adzm 2010-05-12 - dsmplugin config
	myIniFile.ReadString("admin", "DSMPluginConfig", m_pref_DSMPluginConfig, 512);
#ifdef IPV6V4
	m_server->SetIPV6(myIniFile.ReadInt("admin", "UseIpv6", false));
#endif
	if (m_server->LoopbackOnly()) m_server->SetLoopbackOk(true);
	else m_server->SetLoopbackOk(myIniFile.ReadInt("admin", "AllowLoopback", true));
	m_server->SetAuthRequired(myIniFile.ReadInt("admin", "AuthRequired", true));

	m_server->SetConnectPriority(myIniFile.ReadInt("admin", "ConnectPriority", 0));
	if (!m_server->LoopbackOnly())
	{
		char *authhosts=new char[150];
		myIniFile.ReadString("admin", "AuthHosts",authhosts,150);
		if (authhosts != 0) {
			m_server->SetAuthHosts(authhosts);
			delete [] authhosts;
		} else {
			m_server->SetAuthHosts(0);
		}
	} else {
		m_server->SetAuthHosts(0);
	}

	//vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Load User Preferences\n"));

	// Set the default user prefs
	vnclog.Print(LL_INTINFO, VNCLOG("clearing user settings\n"));
	m_pref_AutoPortSelect=TRUE;
    m_pref_HTTPConnect = TRUE;
	m_pref_PortNumber = RFB_PORT_OFFSET; 
	m_pref_SockConnect=TRUE;
	{
	    vncPasswd::FromClear crypt(m_pref_Secure);
	    memcpy(m_pref_passwd, crypt, MAXPWLEN);
	}
	m_pref_QuerySetting=2;
	m_pref_QueryTimeout=10;
	m_pref_QueryDisableTime=0;
	m_pref_QueryAccept=0;
	m_pref_IdleTimeout=0;
	m_pref_MaxViewerSetting = 0;
	m_pref_MaxViewers = 128;
	m_pref_EnableRemoteInputs=TRUE;
	m_pref_DisableLocalInputs=FALSE;
	m_pref_EnableJapInput=FALSE;
	m_pref_EnableUnicodeInput=FALSE;
	m_pref_EnableWin8Helper=FALSE;
	m_pref_clearconsole=FALSE;
	m_pref_LockSettings=-1;
	m_pref_Collabo=false;

	m_pref_Frame = FALSE;
	m_pref_Notification = FALSE;
	m_pref_OSD = FALSE;
	m_pref_NotificationSelection = 0;

	m_pref_RemoveWallpaper=FALSE;
	// adzm - 2010-07 - Disable more effects or font smoothing
	m_pref_RemoveEffects=FALSE;
	m_pref_RemoveFontSmoothing=FALSE;
    m_alloweditclients = TRUE;
	m_allowshutdown = TRUE;
	m_allowproperties = TRUE;
	m_allowInjection = FALSE;

	// Modif sf@2002
	m_pref_SingleWindow = FALSE;
	m_pref_UseDSMPlugin = FALSE;
	*m_pref_szDSMPlugin = '\0';
	m_pref_DSMPluginConfig[0] = '\0';

	m_pref_EnableFileTransfer = TRUE;
	m_pref_FTUserImpersonation = TRUE;
	m_pref_EnableBlankMonitor = TRUE;
	m_pref_BlankInputsOnly = FALSE;
	m_pref_QueryIfNoLogon = FALSE;
	m_pref_DefaultScale = 1;

	LoadUserPrefsFromIniFile();
	m_allowshutdown = myIniFile.ReadInt("admin", "AllowShutdown", m_allowshutdown);
	m_allowproperties = myIniFile.ReadInt("admin", "AllowProperties", m_allowproperties);
	m_allowInjection = myIniFile.ReadInt("admin", "AllowInjection", m_allowInjection);
	m_alloweditclients = myIniFile.ReadInt("admin", "AllowEditClients", m_alloweditclients);

    m_ftTimeout = myIniFile.ReadInt("admin", "FileTransferTimeout", m_ftTimeout);
    if (m_ftTimeout > 600)
        m_ftTimeout = 600;

    m_keepAliveInterval = myIniFile.ReadInt("admin", "KeepAliveInterval", m_keepAliveInterval);
	m_IdleInputTimeout = myIniFile.ReadInt("admin", "IdleInputTimeout", m_IdleInputTimeout);

    if (m_keepAliveInterval >= (m_ftTimeout - KEEPALIVE_HEADROOM))
        m_keepAliveInterval = m_ftTimeout - KEEPALIVE_HEADROOM;

    m_server->SetFTTimeout(m_ftTimeout);
    m_server->SetKeepAliveInterval(m_keepAliveInterval);
	m_server->SetIdleInputTimeout(m_IdleInputTimeout);
    
	myIniFile.ReadString("admin", "service_commandline", service_commandline, 1024);
	myIniFile.ReadString("admin", "accept_reject_mesg", accept_reject_mesg, 512);

	ApplyUserPrefs();
}


void vncProperties::LoadUserPrefsFromIniFile()
{
	// Modif sf@2002
	m_pref_EnableFileTransfer = myIniFile.ReadInt("admin", "FileTransferEnabled", m_pref_EnableFileTransfer);
	m_pref_FTUserImpersonation = myIniFile.ReadInt("admin", "FTUserImpersonation", m_pref_FTUserImpersonation); // sf@2005
	m_pref_EnableBlankMonitor = myIniFile.ReadInt("admin", "BlankMonitorEnabled", m_pref_EnableBlankMonitor);
	m_pref_BlankInputsOnly = myIniFile.ReadInt("admin", "BlankInputsOnly", m_pref_BlankInputsOnly); //PGM
	m_pref_DefaultScale = myIniFile.ReadInt("admin", "DefaultScale", m_pref_DefaultScale);

	m_pref_UseDSMPlugin = myIniFile.ReadInt("admin", "UseDSMPlugin", m_pref_UseDSMPlugin);
	myIniFile.ReadString("admin", "DSMPlugin",m_pref_szDSMPlugin,128);
	
	//adzm 2010-05-12 - dsmplugin config
	myIniFile.ReadString("admin", "DSMPluginConfig", m_pref_DSMPluginConfig, 512);
	
	m_pref_Primary = myIniFile.ReadInt("admin", "primary", m_pref_Primary);
	m_pref_Secondary = myIniFile.ReadInt("admin", "secondary", m_pref_Secondary);

	// Connection prefs
	m_pref_SockConnect=myIniFile.ReadInt("admin", "SocketConnect", m_pref_SockConnect);
	m_pref_HTTPConnect=myIniFile.ReadInt("admin", "HTTPConnect", m_pref_HTTPConnect);
	m_pref_AutoPortSelect=myIniFile.ReadInt("admin", "AutoPortSelect", m_pref_AutoPortSelect);
	m_pref_PortNumber=myIniFile.ReadInt("admin", "PortNumber", m_pref_PortNumber);
	m_pref_HttpPortNumber=myIniFile.ReadInt("admin", "HTTPPortNumber",
									DISPLAY_TO_HPORT(PORT_TO_DISPLAY(m_pref_PortNumber)));
	m_pref_IdleTimeout=myIniFile.ReadInt("admin", "IdleTimeout", m_pref_IdleTimeout);
	
	m_pref_RemoveWallpaper=myIniFile.ReadInt("admin", "RemoveWallpaper", m_pref_RemoveWallpaper);
	// adzm - 2010-07 - Disable more effects or font smoothing
	m_pref_RemoveEffects=myIniFile.ReadInt("admin", "RemoveEffects", m_pref_RemoveEffects);
	m_pref_RemoveFontSmoothing=myIniFile.ReadInt("admin", "RemoveFontSmoothing", m_pref_RemoveFontSmoothing);

	// Connection querying settings
	m_pref_QuerySetting=myIniFile.ReadInt("admin", "QuerySetting", m_pref_QuerySetting);
	m_server->SetQuerySetting(m_pref_QuerySetting);
	m_pref_QueryTimeout=myIniFile.ReadInt("admin", "QueryTimeout", m_pref_QueryTimeout);
	m_server->SetQueryTimeout(m_pref_QueryTimeout);
	m_pref_QueryDisableTime=myIniFile.ReadInt("admin", "QueryDisableTime", m_pref_QueryDisableTime);
	m_server->SetQueryDisableTime(m_pref_QueryDisableTime);	
	m_pref_QueryAccept=myIniFile.ReadInt("admin", "QueryAccept", m_pref_QueryAccept);
	m_server->SetQueryAccept(m_pref_QueryAccept);

	m_pref_MaxViewerSetting = myIniFile.ReadInt("admin", "MaxViewerSetting", m_pref_MaxViewerSetting);
	m_server->setMaxViewerSetting(m_pref_MaxViewerSetting);
	m_pref_MaxViewers = myIniFile.ReadInt("admin", "MaxViewers", m_pref_MaxViewers);
	m_server->setMaxViewers(m_pref_MaxViewers);

	m_pref_Collabo = myIniFile.ReadInt("admin", "Collabo", m_pref_Collabo);
	m_server->setCollabo(m_pref_Collabo);

	m_pref_Frame = myIniFile.ReadInt("admin", "Frame", m_pref_Frame);
	m_server->setFrame(m_pref_Frame);
	m_pref_Notification = myIniFile.ReadInt("admin", "Notification", m_pref_Notification);
	m_server->setNotification(m_pref_Notification);

	m_pref_OSD = myIniFile.ReadInt("admin", "OSD", m_pref_OSD);
	m_server->setOSD(m_pref_OSD);
	m_pref_NotificationSelection = myIniFile.ReadInt("admin", "NotificationSelection", m_pref_NotificationSelection);
	m_server->setNotificationSelection(m_pref_NotificationSelection);

	// marscha@2006 - Is AcceptDialog required even if no user is logged on
	m_pref_QueryIfNoLogon=myIniFile.ReadInt("admin", "QueryIfNoLogon", m_pref_QueryIfNoLogon);
	m_server->SetQueryIfNoLogon(m_pref_QueryIfNoLogon);

	// Load the password
	myIniFile.ReadPassword(m_pref_passwd,MAXPWLEN);
	myIniFile.ReadPassword2(m_pref_passwd2,MAXPWLEN); //PGM

	// Remote access prefs
	m_pref_EnableRemoteInputs=myIniFile.ReadInt("admin", "InputsEnabled", m_pref_EnableRemoteInputs);
	m_pref_LockSettings=myIniFile.ReadInt("admin", "LockSetting", m_pref_LockSettings);
	m_pref_DisableLocalInputs=myIniFile.ReadInt("admin", "LocalInputsDisabled", m_pref_DisableLocalInputs);
	m_pref_EnableJapInput=myIniFile.ReadInt("admin", "EnableJapInput", m_pref_EnableJapInput);
	m_pref_EnableUnicodeInput=myIniFile.ReadInt("admin", "EnableUnicodeInput", m_pref_EnableUnicodeInput);
	m_pref_EnableWin8Helper=myIniFile.ReadInt("admin", "EnableWin8Helper", m_pref_EnableWin8Helper);
	m_pref_clearconsole=myIniFile.ReadInt("admin", "clearconsole", m_pref_clearconsole);
	G_SENDBUFFER_EX=myIniFile.ReadInt("admin", "sendbuffer", G_SENDBUFFER_EX);
}

void vncProperties::SaveToIniFile()
{
	if (!m_allowproperties)
		return;

	// SAVE PER-USER PREFS IF ALLOWED
	if (!myIniFile.IsWritable()  || vncService::RunningAsService())
			{
				//First check if temp file is writable
				myIniFile.IniFileSetTemp( m_Tempfile);
				if (!myIniFile.IsWritable())
					{
						vnclog.Print(LL_INTERR, VNCLOG("file %s not writable, error saving new settings\n"), m_Tempfile);
						return;				
					}
				if (!Copy_to_Temp( m_Tempfile))
					{
						vnclog.Print(LL_INTERR, VNCLOG("file %s not writable, error saving new settings\n"), m_Tempfile);
						return;				
					}

				SaveUserPrefsToIniFile();
				myIniFile.WriteInt("admin", "DebugMode", vnclog.GetMode());
				myIniFile.WriteInt("admin", "Avilog", vnclog.GetVideo());
				myIniFile.WriteString("admin", "path", vnclog.GetPath());
				myIniFile.WriteInt("admin", "DebugLevel", vnclog.GetLevel());
				myIniFile.WriteInt("admin", "AllowLoopback", m_server->LoopbackOk());
#ifdef IPV6V4
				myIniFile.WriteInt("admin", "UseIpv6", m_server->IPV6());
#endif
				myIniFile.WriteInt("admin", "LoopbackOnly", m_server->LoopbackOnly());
				myIniFile.WriteInt("admin", "AllowShutdown", m_allowshutdown);
				myIniFile.WriteInt("admin", "AllowProperties",  m_allowproperties);
				myIniFile.WriteInt("admin", "AllowInjection",  m_allowInjection);				
				myIniFile.WriteInt("admin", "AllowEditClients", m_alloweditclients);
				myIniFile.WriteInt("admin", "FileTransferTimeout", m_ftTimeout);
				myIniFile.WriteInt("admin", "KeepAliveInterval", m_keepAliveInterval);
				myIniFile.WriteInt("admin", "IdleInputTimeout", m_IdleInputTimeout);
				myIniFile.WriteInt("admin", "DisableTrayIcon", m_server->GetDisableTrayIcon());
				myIniFile.WriteInt("admin", "rdpmode", m_server->GetRdpmode());
				myIniFile.WriteInt("admin", "noscreensaver", m_server->GetNoScreensaver());
				myIniFile.WriteInt("admin", "Secure", m_server->Secure());
				myIniFile.WriteInt("admin", "MSLogonRequired", m_server->MSLogonRequired());
				// Marscha@2004 - authSSP: save "New MS-Logon" state
				myIniFile.WriteInt("admin", "NewMSLogon", m_server->GetNewMSLogon());
				myIniFile.WriteInt("admin", "ReverseAuthRequired", m_server->GetReverseAuthRequired());
				// sf@2003 - DSM params here
				myIniFile.WriteInt("admin", "ConnectPriority", m_server->ConnectPriority());
				myIniFile.WriteString("admin", "service_commandline", service_commandline);
				myIniFile.WriteString("admin", "accept_reject_mesg", accept_reject_mesg);
				myIniFile.copy_to_secure();
				myIniFile.IniFileSetSecure();
				return;
			}

	SaveUserPrefsToIniFile();
	myIniFile.WriteInt("admin", "DebugMode", vnclog.GetMode());
	myIniFile.WriteInt("admin", "Avilog", vnclog.GetVideo());
	myIniFile.WriteString("admin", "path", vnclog.GetPath());
	myIniFile.WriteInt("admin", "DebugLevel", vnclog.GetLevel());
	myIniFile.WriteInt("admin", "AllowLoopback", m_server->LoopbackOk());
#ifdef IPV6V4
	myIniFile.WriteInt("admin", "UseIpv6", m_server->IPV6());
#endif
	myIniFile.WriteInt("admin", "LoopbackOnly", m_server->LoopbackOnly());
	myIniFile.WriteInt("admin", "AllowShutdown", m_allowshutdown);
	myIniFile.WriteInt("admin", "AllowProperties",  m_allowproperties);
	myIniFile.WriteInt("admin", "AllowInjection",  m_allowInjection);
	myIniFile.WriteInt("admin", "AllowEditClients", m_alloweditclients);
    myIniFile.WriteInt("admin", "FileTransferTimeout", m_ftTimeout);
    myIniFile.WriteInt("admin", "KeepAliveInterval", m_keepAliveInterval);
	myIniFile.WriteInt("admin", "IdleInputTimeout", m_IdleInputTimeout);
	myIniFile.WriteInt("admin", "DisableTrayIcon", m_server->GetDisableTrayIcon());
	myIniFile.WriteInt("admin", "rdpmode", m_server->GetRdpmode());
	myIniFile.WriteInt("admin", "noscreensaver", m_server->GetNoScreensaver());
	myIniFile.WriteInt("admin", "Secure", m_server->Secure());
	myIniFile.WriteInt("admin", "MSLogonRequired", m_server->MSLogonRequired());
	// Marscha@2004 - authSSP: save "New MS-Logon" state
	myIniFile.WriteInt("admin", "NewMSLogon", m_server->GetNewMSLogon());
	myIniFile.WriteInt("admin", "ReverseAuthRequired", m_server->GetReverseAuthRequired());
	// sf@2003 - DSM params here
	myIniFile.WriteInt("admin", "ConnectPriority", m_server->ConnectPriority());

	myIniFile.WriteString("admin", "service_commandline", service_commandline);
	myIniFile.WriteString("admin", "accept_reject_mesg", accept_reject_mesg);
	return;
}


void vncProperties::SaveUserPrefsToIniFile()
{
	// SAVE THE PER USER PREFS
	vnclog.Print(LL_INTINFO, VNCLOG("saving current settings to registry\n"));

	// Modif sf@2002
	myIniFile.WriteInt("admin", "FileTransferEnabled", m_server->FileTransferEnabled());
	myIniFile.WriteInt("admin", "FTUserImpersonation", m_server->FTUserImpersonation()); // sf@2005
	myIniFile.WriteInt("admin", "BlankMonitorEnabled", m_server->BlankMonitorEnabled());
	myIniFile.WriteInt("admin", "BlankInputsOnly", m_server->BlankInputsOnly()); //PGM

	myIniFile.WriteInt("admin", "DefaultScale", m_server->GetDefaultScale());

	myIniFile.WriteInt("admin", "UseDSMPlugin", m_server->IsDSMPluginEnabled());

	myIniFile.WriteString("admin", "DSMPlugin",m_server->GetDSMPluginName());

	//adzm 2010-05-12 - dsmplugin config
	//myIniFile.WriteString("admin", "DSMPluginConfig", m_server->GetDSMPluginConfig());

	myIniFile.WriteInt("admin", "primary", m_server->Primary());
	myIniFile.WriteInt("admin", "secondary", m_server->Secondary());

	// Connection prefs
	myIniFile.WriteInt("admin", "SocketConnect", m_server->SockConnected());
	myIniFile.WriteInt("admin", "HTTPConnect", m_server->HTTPConnectEnabled());
	myIniFile.WriteInt("admin", "AutoPortSelect", m_server->AutoPortSelect());
	if (!m_server->AutoPortSelect()) {
		myIniFile.WriteInt("admin", "PortNumber", m_server->GetPort());
		myIniFile.WriteInt("admin", "HTTPPortNumber", m_server->GetHttpPort());
	}
	myIniFile.WriteInt("admin", "InputsEnabled", m_server->RemoteInputsEnabled());
	myIniFile.WriteInt("admin", "LocalInputsDisabled", m_server->LocalInputsDisabled());
	myIniFile.WriteInt("admin", "IdleTimeout", m_server->AutoIdleDisconnectTimeout());
	myIniFile.WriteInt("admin", "EnableJapInput", m_server->JapInputEnabled());
	myIniFile.WriteInt("admin", "EnableUnicodeInput", m_server->UnicodeInputEnabled());
	myIniFile.WriteInt("admin", "EnableWin8Helper", m_server->Win8HelperEnabled());

	// Connection querying settings
	myIniFile.WriteInt("admin", "QuerySetting", m_server->QuerySetting());
	myIniFile.WriteInt("admin", "QueryTimeout", m_server->QueryTimeout());
	myIniFile.WriteInt("admin", "QueryDisableTime", m_server->QueryDisableTime());
	myIniFile.WriteInt("admin", "QueryAccept", m_server->QueryAcceptForSave());
	myIniFile.WriteInt("admin", "MaxViewerSetting", m_server->getMaxViewerSetting());
	myIniFile.WriteInt("admin", "MaxViewers", m_server->getMaxViewers());
	myIniFile.WriteInt("admin", "Collabo", m_server->getCollabo());
	myIniFile.WriteInt("admin", "Frame", m_server->getFrame());
	myIniFile.WriteInt("admin", "Notification", m_server->getNotification());
	myIniFile.WriteInt("admin", "OSD", m_server->getOSD());
	myIniFile.WriteInt("admin", "NotificationSelection", m_server->getNotificationSelection());
	// Lock settings
	myIniFile.WriteInt("admin", "LockSetting", m_server->LockSettings());

	// Wallpaper removal
	myIniFile.WriteInt("admin", "RemoveWallpaper", m_server->RemoveWallpaperEnabled());
	// UI Effects
	// adzm - 2010-07 - Disable more effects or font smoothing
	myIniFile.WriteInt("admin", "RemoveEffects", m_server->RemoveEffectsEnabled());
	myIniFile.WriteInt("admin", "RemoveFontSmoothing", m_server->RemoveFontSmoothingEnabled());

	// Save the password
	char passwd[MAXPWLEN];
	m_server->GetPassword(passwd);
	myIniFile.WritePassword(passwd);
	memset(passwd, '\0', MAXPWLEN); //PGM
	m_server->GetPassword2(passwd); //PGM
	myIniFile.WritePassword2(passwd); //PGM
}


void vncProperties::ReloadDynamicSettings()
{
	char username[UNLEN+1];

	// Get the user name / service name
	if (!vncService::CurrentUser((char *)&username, sizeof(username)))
	{
		vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - NO current user\n"));
		return;
	}

	// If there is no user logged on them default to SYSTEM
	if (strcmp(username, "") == 0)
	{
		//vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Force USER SYSTEM 2\n"));
		strcpy_s((char *)&username, UNLEN+1, "SYSTEM");
	}

	// Logging/debugging prefs
	vnclog.SetMode(myIniFile.ReadInt("admin", "DebugMode", 0));
	vnclog.SetLevel(myIniFile.ReadInt("admin", "DebugLevel", 0));
}






void Secure_Save_Plugin_Config(char *szPlugin)
{
	HANDLE hProcess = NULL, hPToken = NULL;
	DWORD id = vncService::GetExplorerLogonPid();
	if (id != 0)
	{
		hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, id);
		if (!hProcess) goto error3;
		if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
			| TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_SESSIONID
			| TOKEN_READ | TOKEN_WRITE, &hPToken))
		{
			CloseHandle(hProcess);
			goto error3;
		}

		char dir[MAX_PATH];
		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);
		strcpy_s(dir, exe_file_name);
		strcat_s(dir, " -dsmpluginhelper ");
		strcat_s(dir, szPlugin);

		{
			STARTUPINFO          StartUPInfo;
			PROCESS_INFORMATION  ProcessInfo;
			HANDLE Token = NULL;
			HANDLE process = NULL;
			ZeroMemory(&StartUPInfo, sizeof(STARTUPINFO));
			ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
			StartUPInfo.wShowWindow = SW_SHOW;
			StartUPInfo.lpDesktop = "Winsta0\\Default";
			StartUPInfo.cb = sizeof(STARTUPINFO);

			CreateProcessAsUser(hPToken, NULL, dir, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
			DWORD errorcode = GetLastError();
			if (errorcode == 1314) goto error1;
			if (process) CloseHandle(process);
			if (Token) CloseHandle(Token);
			if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
			if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);	
			return;
		error1:
			Secure_Plugin(szPlugin);
		}
	error3:
		return;
	}
}


void Secure_Plugin_elevated(char *szPlugin)
{
	char dir[MAX_PATH];
	char exe_file_name[MAX_PATH];
	strcpy_s(dir, " -dsmplugininstance ");
	strcat_s(dir, szPlugin);

	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = dir;
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_HIDE;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

void Secure_Plugin(char *szPlugin)
{
	CDSMPlugin* m_pDSMPlugin = NULL;
	m_pDSMPlugin = new CDSMPlugin();
	m_pDSMPlugin->LoadPlugin(szPlugin, false);
	if (m_pDSMPlugin->IsLoaded())
	{
		char szParams[32];
		strcpy_s(szParams, "NoPassword,");
		strcat_s(szParams, "server-app");

		HDESK desktop;
		desktop = OpenInputDesktop(0, FALSE,
			DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
			DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
			DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
			DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
			);

		if (desktop == NULL)
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop Error \n"));
		else
			vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop OK\n"));

		HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
		DWORD dummy;

		char new_name[256];
		if (desktop)
		{
			if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
			{
				vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation \n"));
			}

			vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, desktop, old_desktop);

			if (!SetThreadDesktop(desktop))
			{
				vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK:!SetThreadDesktop \n"));
			}
		}

		HRESULT hr = CoInitialize(NULL);
		HWND hwnd2 = CreateWindowA("STATIC", "dummy", WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
		ShowWindow(hwnd2, SW_HIDE);
		char* szNewConfig = NULL;
		char DSMPluginConfig[512];
		DSMPluginConfig[0] = '\0';
		IniFile myIniFile;
		myIniFile.ReadString("admin", "DSMPluginConfig", DSMPluginConfig, 512);
		m_pDSMPlugin->SetPluginParams(hwnd2, szParams, DSMPluginConfig, &szNewConfig);


		if (szNewConfig != NULL && strlen(szNewConfig) > 0) {
			strcpy_s(DSMPluginConfig, 511, szNewConfig);
		}
		myIniFile.WriteString("admin", "DSMPluginConfig", DSMPluginConfig);


		CoUninitialize();
		SetThreadDesktop(old_desktop);
		if (desktop) CloseDesktop(desktop);
	}
	if (m_pDSMPlugin != NULL) delete(m_pDSMPlugin);
}

void vncProperties::ExpandBox(HWND hDlg, BOOL fExpand)
{
	// if the dialog is already in the requested state, return
	// immediately.
	if (fExpand == m_bExpanded) return;

	RECT rcWnd, rcDefaultBox, rcChild, rcIntersection;
	HWND wndChild = NULL;
	HWND wndDefaultBox = NULL;

	// get the window of the button 
	HWND  pCtrl = GetDlgItem(hDlg, IDC_SHOWOPTIONS);
	if (pCtrl == NULL) return;

	wndDefaultBox = GetDlgItem(hDlg, IDC_DEFAULTBOX);
	if (wndDefaultBox == NULL) return;

	if (!fExpand) SendMessage(GetDlgItem(hDlg, IDC_BUTTON_EXPAND), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmpExpand);
	else SendMessage(GetDlgItem(hDlg, IDC_BUTTON_EXPAND), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmpCollaps);
	// retrieve coordinates for the default child window
	GetWindowRect(wndDefaultBox, &rcDefaultBox);

	// enable/disable all of the child window outside of the default box.
	wndChild = GetTopWindow(hDlg);

	for (; wndChild != NULL; wndChild = GetWindow(wndChild, GW_HWNDNEXT))
	{
		// get rectangle occupied by child window in screen coordinates.
		GetWindowRect(wndChild, &rcChild);

		if (!IntersectRect(&rcIntersection, &rcChild, &rcDefaultBox))
		{
			EnableWindow(wndChild, fExpand);
		}
	}

	if (!fExpand)  // we are contracting
	{
		_ASSERT(m_bExpanded);
		GetWindowRect(hDlg, &rcWnd);

		// this is the first time we are being called to shrink the dialog
		// box.  The dialog box is currently in its expanded size and we must
		// save the expanded width and height so that it can be restored
		// later when the dialog box is expanded.

		if (cx == 0 && cy == 0)
		{
			cx = rcDefaultBox.right - rcWnd.left;
			cy = rcWnd.bottom - rcWnd.top;

			// we also hide the default box here so that it is not visible
			ShowWindow(wndDefaultBox, SW_HIDE);
		}


		// shrink the dialog box so that it encompasses everything from the top,
		// left up to and including the default box.
		SetWindowPos(hDlg, NULL, 0, 0,
			rcDefaultBox.right - rcWnd.left,
			rcDefaultBox.bottom - rcWnd.top,
			SWP_NOZORDER | SWP_NOMOVE);

		SetWindowText(pCtrl, "Advanced options");

		// record that the dialog is contracted.
		m_bExpanded = FALSE;
	}
	else // we are expanding
	{
		_ASSERT(!m_bExpanded);
		SetWindowPos(hDlg, NULL, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOMOVE);

		// make sure that the entire dialog box is visible on the user's
		// screen.
		SendMessage(hDlg, DM_REPOSITION, 0, 0);
		SetWindowText(pCtrl, "Hide");
		m_bExpanded = TRUE;
	}
}