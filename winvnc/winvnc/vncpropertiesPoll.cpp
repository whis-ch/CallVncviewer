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


// vncPropertiesPoll.cpp

// Implementation of the Properties dialog!

#include "stdhdrs.h"
#include "lmcons.h"
#include "vncservice.h"

#include "winvnc.h"
#include "vncpropertiesPoll.h"
#include "vncserver.h"
#include "vncOSVersion.h"
#include <ShlObj.h>
#include "common/win32_helpers.h"


#include "Localization.h" // ACT : Add localization on messages

// [v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

bool RunningAsAdministrator ();
const char WINVNC_REGISTRY_KEY [] = "Software\\ORL\\WinVNC3";

// Constructor & Destructor
vncPropertiesPoll::vncPropertiesPoll()
{
	ddEngine = false;
	m_dlgvisible = FALSE;
	m_usersettings = TRUE;
	if (IsWindows8OrGreater())
		ddEngine = true;
}

vncPropertiesPoll::~vncPropertiesPoll()
{
}

// Initialisation
BOOL
vncPropertiesPoll::Init(vncServer *server)
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

	return TRUE;
}

// Dialog box handling functions
void
vncPropertiesPoll::Show(BOOL show, BOOL usersettings)
{
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
	if (id!=0) 
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

	if (show)
	{

		if (!m_fUseRegistry) // Use the ini file
		{
			// We're trying to edit the default local settings - verify that we can
			/*
			if (!myIniFile.IsWritable())
			{
				if(iImpersonateResult == ERROR_SUCCESS)RevertToSelf();
				CloseHandle(hProcess);
				CloseHandle(hPToken);
				return;
			}
			*/
		}
		else // Use the registry
		{
			// Verify that we know who is logged on
			if (usersettings) {
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
			} else {
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
				// Load in the settings relevant to the user or system
				Load(usersettings);
			}
			else
				LoadFromIniFile();


			for (;;)
			{
				m_returncode_valid = FALSE;

				// Do the dialog box
				// [v1.0.2-jp1 fix]
				//int result = DialogBoxParam(hAppInstance,
				int result = (int)DialogBoxParam(hInstResDLL,
				    MAKEINTRESOURCE(IDD_PROPERTIES), 
				    NULL,
				    (DLGPROC) DialogProcPoll,
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
				break;
				omni_thread::sleep(4);
			}

			// Load in all the settings
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
vncPropertiesPoll::DialogProcPoll(HWND hwnd,
						  UINT uMsg,
						  WPARAM wParam,
						  LPARAM lParam )
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
    vncPropertiesPoll *_this = helper::SafeGetWindowUserData<vncPropertiesPoll>(hwnd);

	switch (uMsg)
	{

	case WM_HSCROLL:
		{
			if ((lParam != 0) &&
				(reinterpret_cast<HWND>(lParam) == GetDlgItem(hwnd, IDC_SLIDERFPS)))
			{
				switch (LOWORD(wParam))
				{
				case SB_ENDSCROLL:
				case SB_LEFT:
				case SB_RIGHT:
				case SB_LINELEFT:
				case SB_LINERIGHT:
				case SB_PAGELEFT:
				case SB_PAGERIGHT:
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					int pos = SendMessage(GetDlgItem(hwnd, IDC_SLIDERFPS), TBM_GETPOS, 0, 0L);
					CHAR temp[250];
					sprintf_s(temp, "%d", pos);
					SetDlgItemText(hwnd, IDC_STATICFPS, temp);
					_this->m_server->MaxFPS(pos);
					break;
				}
			}
			break;
		}
		return (INT_PTR)FALSE;

	case WM_INITDIALOG:
		{
			// Retrieve the Dialog box parameter and use it as a pointer
			// to the calling vncPropertiesPoll object
            helper::SafeSetWindowUserData(hwnd, lParam);
			_this = (vncPropertiesPoll *) lParam;
			_this->m_dlgvisible = TRUE;


			HWND slider = GetDlgItem(hwnd, IDC_SLIDERFPS);
			SendMessage(slider, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(5, 60));
			SendMessage(slider, TBM_SETTICFREQ, 1, 10);		
			SendMessage(slider, TBM_SETPOS, true, _this->m_server->MaxFPS());
			CHAR temp[250];
			sprintf_s(temp, "%d", _this->m_server->MaxFPS());
			SetDlgItemText(hwnd, IDC_STATICFPS, temp);

			if (_this->ddEngine) {
				ShowWindow(GetDlgItem(hwnd, IDC_CHECKDRIVER), false);
				ShowWindow(GetDlgItem(hwnd, IDC_STATICELEVATED), false);
				EnableWindow(GetDlgItem(hwnd, IDC_SLIDERFPS), _this->m_server->Driver());
				EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS60), _this->m_server->Driver());
				EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS5), _this->m_server->Driver());
				SetWindowText(GetDlgItem(hwnd, IDC_DRIVER), "DDEngine (restart on change required)");
			}
			else {
				EnableWindow(GetDlgItem(hwnd, IDC_SLIDERFPS), false);
				EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS60), false);
				EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS5), false);
			}
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
				//LoadFromIniFile();
			}

			// Modif sf@2002

		   SetDlgItemInt(hwnd, IDC_MAXCPU, _this->m_server->MaxCpu(), false);

		   HWND hTurboMode = GetDlgItem(hwnd, IDC_TURBOMODE);
           SendMessage(hTurboMode, BM_SETCHECK, _this->m_server->TurboMode(), 0);

			// Set the polling options
			HWND hDriver = GetDlgItem(hwnd, IDC_DRIVER);
			SendMessage(hDriver,
				BM_SETCHECK,
				_this->m_server->Driver(),0);

			HWND hHook = GetDlgItem(hwnd, IDC_HOOK);
			SendMessage(hHook,
				BM_SETCHECK,
				_this->m_server->Hook(),
				0);

			HWND hPollFullScreen = GetDlgItem(hwnd, IDC_POLL_FULLSCREEN);
			SendMessage(hPollFullScreen,
				BM_SETCHECK,
				_this->m_server->PollFullScreen(),
				0);

			HWND hPollForeground = GetDlgItem(hwnd, IDC_POLL_FOREGROUND);
			SendMessage(hPollForeground,
				BM_SETCHECK,
				_this->m_server->PollForeground(),
				0);

			HWND hPollUnderCursor = GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR);
			SendMessage(hPollUnderCursor,
				BM_SETCHECK,
				_this->m_server->PollUnderCursor(),
				0);

			HWND hPollConsoleOnly = GetDlgItem(hwnd, IDC_CONSOLE_ONLY);
			SendMessage(hPollConsoleOnly,
				BM_SETCHECK,
				_this->m_server->PollConsoleOnly(),
				0);
			EnableWindow(hPollConsoleOnly,
				_this->m_server->PollUnderCursor() || _this->m_server->PollForeground()
				);

			HWND hPollOnEventOnly = GetDlgItem(hwnd, IDC_ONEVENT_ONLY);
			SendMessage(hPollOnEventOnly,
				BM_SETCHECK,
				_this->m_server->PollOnEventOnly(),
				0);
			EnableWindow(hPollOnEventOnly,
				_this->m_server->PollUnderCursor() || _this->m_server->PollForeground()
				);

			// [<--v1.0.2-jp2 fix]

			CheckDlgButton(hwnd, IDC_AUTOCAPT1,
					(_this->m_pref_autocapt == 1) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_AUTOCAPT2,
					(_this->m_pref_autocapt == 2) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_AUTOCAPT3,
					(_this->m_pref_autocapt == 3) ? BST_CHECKED : BST_UNCHECKED);

			SetForegroundWindow(hwnd);

			return FALSE; // Because we've set the focus
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_DRIVER:
			if (SendMessage(GetDlgItem(hwnd, IDC_DRIVER), BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				_this->m_server->Driver(CheckVideoDriver(0));
			}
			else _this->m_server->Driver(false);
			if (_this->ddEngine) {
				EnableWindow(GetDlgItem(hwnd, IDC_SLIDERFPS), _this->m_server->Driver());
				EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS60), _this->m_server->Driver());
				EnableWindow(GetDlgItem(hwnd, IDC_STATICFPS5), _this->m_server->Driver());
			}
			
			break;
		case IDOK:
		case IDC_APPLY:
			{
				

				int maxcpu = GetDlgItemInt(hwnd, IDC_MAXCPU, NULL, FALSE);
				_this->m_server->MaxCpu(maxcpu);

				int pos = SendMessage(GetDlgItem(hwnd, IDC_SLIDERFPS), TBM_GETPOS, 0, 0L);
				_this->m_server->MaxFPS(pos);

				HWND hcapt = GetDlgItem(hwnd, IDC_AUTOCAPT1);
				if (SendMessage(hcapt, BM_GETCHECK, 0, 0) == BST_CHECKED)
						_this->m_pref_autocapt = 1;
				hcapt = GetDlgItem(hwnd, IDC_AUTOCAPT2);
				if (SendMessage(hcapt, BM_GETCHECK, 0, 0) == BST_CHECKED)
						_this->m_pref_autocapt = 2;
				hcapt = GetDlgItem(hwnd, IDC_AUTOCAPT3);
				if (SendMessage(hcapt, BM_GETCHECK, 0, 0) == BST_CHECKED)
						_this->m_pref_autocapt = 3;

				_this->m_server->AutoCapt(_this->m_pref_autocapt);
				// Modif sf@2002
				HWND hTurboMode = GetDlgItem(hwnd, IDC_TURBOMODE);
				_this->m_server->TurboMode(SendMessage(hTurboMode, BM_GETCHECK, 0, 0) == BST_CHECKED);

				// Handle the polling stuff
				HWND hDriver = GetDlgItem(hwnd, IDC_DRIVER);
				bool result=(SendMessage(hDriver, BM_GETCHECK, 0, 0) == BST_CHECKED);
				if (result)
				{
					_this->m_server->Driver(CheckVideoDriver(0));
				}
				else _this->m_server->Driver(false);
				HWND hHook = GetDlgItem(hwnd, IDC_HOOK);
				_this->m_server->Hook(
					SendMessage(hHook, BM_GETCHECK, 0, 0) == BST_CHECKED
					);
				HWND hPollFullScreen = GetDlgItem(hwnd, IDC_POLL_FULLSCREEN);
				_this->m_server->PollFullScreen(
					SendMessage(hPollFullScreen, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hPollForeground = GetDlgItem(hwnd, IDC_POLL_FOREGROUND);
				_this->m_server->PollForeground(
					SendMessage(hPollForeground, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hPollUnderCursor = GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR);
				_this->m_server->PollUnderCursor(
					SendMessage(hPollUnderCursor, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hPollConsoleOnly = GetDlgItem(hwnd, IDC_CONSOLE_ONLY);
				_this->m_server->PollConsoleOnly(
					SendMessage(hPollConsoleOnly, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				HWND hPollOnEventOnly = GetDlgItem(hwnd, IDC_ONEVENT_ONLY);
				_this->m_server->PollOnEventOnly(
					SendMessage(hPollOnEventOnly, BM_GETCHECK, 0, 0) == BST_CHECKED
					);

				// [<--v1.0.2-jp2 fix] Move to vncpropertiesPoll.cpp

				// Save the settings
				if (_this->m_fUseRegistry)
					_this->Save();
				else
					_this->SaveToIniFile();

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

		// [<--v1.0.2-jp2 fix] Move to vncpropertiesPoll.cpp

		 case IDCANCEL:
			vnclog.Print(LL_INTINFO, VNCLOG("enddialog (CANCEL)\n"));

			_this->m_returncode_valid = TRUE;

			EndDialog(hwnd, IDCANCEL);
			_this->m_dlgvisible = FALSE;
			return TRUE;
		

		case IDC_POLL_FOREGROUND:
		case IDC_POLL_UNDER_CURSOR:
			// User has clicked on one of the polling mode buttons
			// affected by the pollconsole and pollonevent options
			{
				// Get the poll-mode buttons
				HWND hPollForeground = GetDlgItem(hwnd, IDC_POLL_FOREGROUND);
				HWND hPollUnderCursor = GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR);

				// Determine whether to enable the modifier options
				BOOL enabled = (SendMessage(hPollForeground, BM_GETCHECK, 0, 0) == BST_CHECKED) ||
					(SendMessage(hPollUnderCursor, BM_GETCHECK, 0, 0) == BST_CHECKED);

				HWND hPollConsoleOnly = GetDlgItem(hwnd, IDC_CONSOLE_ONLY);
				EnableWindow(hPollConsoleOnly, enabled);

				HWND hPollOnEventOnly = GetDlgItem(hwnd, IDC_ONEVENT_ONLY);
				EnableWindow(hPollOnEventOnly, enabled);
			}
			return TRUE;

		
		case IDC_CHECKDRIVER:
			{
				CheckVideoDriver(1);
			}
			return TRUE;
		



		}
		break;
	}
	return 0;
}




// Functions to load & save the settings
LONG
vncPropertiesPoll::LoadInt(HKEY key, LPCSTR valname, LONG defval)
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
vncPropertiesPoll::LoadSingleWindowName(HKEY key, char *buffer)
{
	DWORD type = REG_BINARY;
	int slen=32;
	char inouttext[32];

	if (RegQueryValueEx(key,
		"SingleWindowName",
		NULL,
		&type,
		(LPBYTE) &inouttext,
		(LPDWORD) &slen) != ERROR_SUCCESS)
		return;

	if (slen > MAXPATH)
		return;

	memcpy(buffer, inouttext, 32);
}

char *
vncPropertiesPoll::LoadString(HKEY key, LPCSTR keyname)
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
vncPropertiesPoll::ResetRegistry()
{	
	vnclog.Print(LL_CLIENTS, VNCLOG("Reset Reg\n"));
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
vncPropertiesPoll::Load(BOOL usersettings)
{
//	if (m_dlgvisible) {
//		vnclog.Print(LL_INTWARN, VNCLOG("service helper invoked while Properties panel displayed\n"));
//		return;
//	}
	ResetRegistry();
	
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


LABELUSERSETTINGS:
	// LOAD THE USER PREFERENCES

	// Set the default user prefs
	vnclog.Print(LL_INTINFO, VNCLOG("clearing user settings\n"));
	m_pref_TurboMode = TRUE;//((vncService::VersionMajor() >= 6) ? TRUE : FALSE);
	m_pref_PollUnderCursor=FALSE;
	m_pref_PollForeground= FALSE;
	m_pref_PollFullScreen= TRUE;//((vncService::VersionMajor() >= 6) ? TRUE : FALSE);
	m_pref_PollConsoleOnly=FALSE;
	m_pref_PollOnEventOnly=FALSE;
	m_pref_MaxCpu=100;
	m_pref_MaxFPS = 25;
	m_pref_Driver=CheckVideoDriver(0);
	m_pref_Hook=TRUE;
	m_pref_Virtual=FALSE;
	m_pref_autocapt=1;

	// [v1.0.2-jp2 fix]
	m_pref_SingleWindow = 0;
	*m_pref_szSingleWindowName = '\0';

	// Load the local prefs for this user
	if (hkDefault != NULL)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("loading DEFAULT local settings\n"));
		LoadUserPrefsPoll(hkDefault);
	}

	
	if (m_usersettings) {
		// We want the user settings, so load them!

		if (hkLocalUser != NULL)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("loading \"%s\" local settings\n"), username);
			LoadUserPrefsPoll(hkLocalUser);
		}

		// Now override the system settings with the user's settings
		// If the username is SYSTEM then don't try to load them, because there aren't any...
		if ((strcmp(username, "SYSTEM") != 0))
		{
			HKEY hkGlobalUser;
			if (RegCreateKeyEx(HKEY_CURRENT_USER,
				WINVNC_REGISTRY_KEY,
				0, REG_NONE, REG_OPTION_NON_VOLATILE,
				KEY_READ, NULL, &hkGlobalUser, &dw) == ERROR_SUCCESS)
			{
				vnclog.Print(LL_INTINFO, VNCLOG("loading \"%s\" global settings\n"), username);
				LoadUserPrefsPoll(hkGlobalUser);
				RegCloseKey(hkGlobalUser);

				// Close the user registry hive so it can unload if required
				RegCloseKey(HKEY_CURRENT_USER);
			}
		}
	}

	if (hkLocalUser != NULL) RegCloseKey(hkLocalUser);
	if (hkDefault != NULL) RegCloseKey(hkDefault);
	if (hkLocal != NULL) RegCloseKey(hkLocal);

	// Make the loaded settings active..
	ApplyUserPrefs();

	// Note whether we loaded the user settings or just the default system settings
	m_usersettings = usersettings;
}


void
vncPropertiesPoll::LoadUserPrefsPoll(HKEY appkey)
{
	m_pref_TurboMode = LoadInt(appkey, "TurboMode", m_pref_TurboMode);
	// Polling prefs
	m_pref_PollUnderCursor=LoadInt(appkey, "PollUnderCursor", m_pref_PollUnderCursor);
	m_pref_PollForeground=LoadInt(appkey, "PollForeground", m_pref_PollForeground);
	m_pref_PollFullScreen=LoadInt(appkey, "PollFullScreen", m_pref_PollFullScreen);
	m_pref_PollConsoleOnly=LoadInt(appkey, "OnlyPollConsole", m_pref_PollConsoleOnly);
	m_pref_PollOnEventOnly=LoadInt(appkey, "OnlyPollOnEvent", m_pref_PollOnEventOnly);
	m_pref_MaxCpu=LoadInt(appkey, "MaxCpu2", m_pref_MaxCpu);
	if (m_pref_MaxCpu==0) m_pref_MaxCpu=100;
	m_pref_MaxFPS = LoadInt(appkey, "MaxFPS", m_pref_MaxFPS);
	m_pref_Driver=LoadInt(appkey, "EnableDriver", m_pref_Driver);
	if (m_pref_Driver)m_pref_Driver=CheckVideoDriver(0);
	m_pref_Hook=LoadInt(appkey, "EnableHook", m_pref_Hook);
	m_pref_Virtual=LoadInt(appkey, "EnableVirtual", m_pref_Virtual);
	// [v1.0.2-jp2 fix]
	m_pref_SingleWindow=LoadInt(appkey, "SingleWindow", m_pref_SingleWindow);
	LoadSingleWindowName(appkey, m_pref_szSingleWindowName);
	m_pref_autocapt=LoadInt(appkey, "autocapt", m_pref_autocapt);

}

void
vncPropertiesPoll::ApplyUserPrefs()
{
	// APPLY THE CACHED PREFERENCES TO THE SERVER

	
    m_server->TurboMode(m_pref_TurboMode);
	// Polling prefs
	m_server->PollUnderCursor(m_pref_PollUnderCursor);
	m_server->PollForeground(m_pref_PollForeground);
	m_server->PollFullScreen(m_pref_PollFullScreen);
	m_server->PollConsoleOnly(m_pref_PollConsoleOnly);
	m_server->PollOnEventOnly(m_pref_PollOnEventOnly);
	m_server->MaxCpu(m_pref_MaxCpu);	
	m_server->MaxFPS(m_pref_MaxFPS);
	if (CheckVideoDriver(0) && m_pref_Driver) m_server->Driver(m_pref_Driver);
	else m_server->Driver(false);
	m_server->Hook(m_pref_Hook);
	m_server->Virtual(m_pref_Virtual);
	m_server->AutoCapt(m_pref_autocapt);

}

void
vncPropertiesPoll::SaveInt(HKEY key, LPCSTR valname, LONG val)
{
	RegSetValueEx(key, valname, 0, REG_DWORD, (LPBYTE) &val, sizeof(val));
}

// [v1.0.2-jp2 fix-->]
void
vncPropertiesPoll::SaveString(HKEY key,LPCSTR valname, const char *buffer)
{
	RegSetValueEx(key, valname, 0, REG_BINARY, (LPBYTE) buffer, (DWORD)(strlen(buffer)+1));
}
// [<--v1.0.2-jp2 fix]

void
vncPropertiesPoll::Save()
{
	HKEY appkey;
	DWORD dw;

	// NEW (R3) PREFERENCES ALGORITHM
	// The user's prefs are only saved if the user is allowed to override
	// the machine-local settings specified for them.  Otherwise, the
	// properties entry on the tray icon menu will be greyed out.

	// GET THE CORRECT KEY TO READ FROM

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
	SaveUserPrefsPoll(appkey);
	RegCloseKey(appkey);
	RegCloseKey(HKEY_CURRENT_USER);

}

void
vncPropertiesPoll::SaveUserPrefsPoll(HKEY appkey)
{
	SaveInt(appkey, "TurboMode", m_server->TurboMode());
	// Polling prefs
	SaveInt(appkey, "PollUnderCursor", m_server->PollUnderCursor());
	SaveInt(appkey, "PollForeground", m_server->PollForeground());
	SaveInt(appkey, "PollFullScreen", m_server->PollFullScreen());

	SaveInt(appkey, "OnlyPollConsole", m_server->PollConsoleOnly());
	SaveInt(appkey, "OnlyPollOnEvent", m_server->PollOnEventOnly());
	SaveInt(appkey, "MaxCpu2", m_server->MaxCpu());
	SaveInt(appkey, "MaxFPS", m_server->MaxFPS());
	SaveInt(appkey, "EnableDriver", m_server->Driver());
	SaveInt(appkey, "EnableHook", m_server->Hook());
	SaveInt(appkey, "EnableVirtual", m_server->Virtual());
	SaveInt(appkey, "autocapt", m_server->AutoCapt());
}


// ********************************************************************
// Ini file part - Wwill replace registry access completely, some day
// WARNING: until then, when adding/modifying a config parameter
//          don't forget to modify both ini file & registry parts !
// ********************************************************************

void vncPropertiesPoll::LoadFromIniFile()
{
//	if (m_dlgvisible)
//	{
//		vnclog.Print(LL_INTWARN, VNCLOG("service helper invoked while Properties panel displayed\n"));
//		return;
//	}
	
	m_pref_TurboMode = TRUE;//((vncService::VersionMajor() >= 6) ? TRUE : FALSE);
	m_pref_PollUnderCursor=FALSE;
	m_pref_PollForeground= FALSE;
	m_pref_PollFullScreen= TRUE;//((vncService::VersionMajor() >= 6) ? TRUE : FALSE);
	m_pref_PollConsoleOnly=FALSE;
	m_pref_PollOnEventOnly=FALSE;
	m_pref_MaxCpu=100;
	m_pref_MaxFPS = 25;
	m_pref_Driver=CheckVideoDriver(0);
	m_pref_Hook=TRUE;
	m_pref_Virtual=FALSE;
	m_pref_autocapt=1;
	m_pref_SingleWindow = 0;
	*m_pref_szSingleWindowName = '\0';

	LoadUserPrefsPollFromIniFile();
	ApplyUserPrefs();
}


void vncPropertiesPoll::LoadUserPrefsPollFromIniFile()
{
	m_pref_TurboMode = myIniFile.ReadInt("poll", "TurboMode", m_pref_TurboMode);
	// Polling prefs
	m_pref_PollUnderCursor=myIniFile.ReadInt("poll", "PollUnderCursor", m_pref_PollUnderCursor);
	m_pref_PollForeground=myIniFile.ReadInt("poll", "PollForeground", m_pref_PollForeground);
	m_pref_PollFullScreen=myIniFile.ReadInt("poll", "PollFullScreen", m_pref_PollFullScreen);
	m_pref_PollConsoleOnly=myIniFile.ReadInt("poll", "OnlyPollConsole", m_pref_PollConsoleOnly);
	m_pref_PollOnEventOnly=myIniFile.ReadInt("poll", "OnlyPollOnEvent", m_pref_PollOnEventOnly);
	m_pref_MaxCpu=myIniFile.ReadInt("poll", "MaxCpu2", m_pref_MaxCpu);
	if (m_pref_MaxCpu==0) m_pref_MaxCpu=100;
	m_pref_MaxFPS = myIniFile.ReadInt("poll", "MaxFPS", m_pref_MaxFPS);
	m_pref_Driver=myIniFile.ReadInt("poll", "EnableDriver", m_pref_Driver);
	if (m_pref_Driver)m_pref_Driver=CheckVideoDriver(0);
	m_pref_Hook=myIniFile.ReadInt("poll", "EnableHook", m_pref_Hook);
	m_pref_Virtual=myIniFile.ReadInt("poll", "EnableVirtual", m_pref_Virtual);
	
	m_pref_SingleWindow=myIniFile.ReadInt("poll","SingleWindow",m_pref_SingleWindow);
	myIniFile.ReadString("poll", "SingleWindowName", m_pref_szSingleWindowName,32);
	m_pref_autocapt = myIniFile.ReadInt("poll", "autocapt", m_pref_autocapt);

}


void vncPropertiesPoll::SaveToIniFile()
{
	bool use_uac=false;
	if (!myIniFile.IsWritable())
			{
				myIniFile.IniFileSetTemp( m_Tempfile);
				if (!myIniFile.IsWritable())
					{
						vnclog.Print(LL_INTERR, VNCLOG("file %s not writable, error saving new settings\n"), m_Tempfile);
						return;				
					}
				if (!Copy_to_Temp(m_Tempfile))
					{
						vnclog.Print(LL_INTERR, VNCLOG("file %s not writable, error saving new settings\n"), m_Tempfile);
						return;				
					}
				SaveUserPrefsPollToIniFile();
				myIniFile.copy_to_secure();
				myIniFile.IniFileSetSecure();
			}
	SaveUserPrefsPollToIniFile();
}


void vncPropertiesPoll::SaveUserPrefsPollToIniFile()
{
	myIniFile.WriteInt("poll", "TurboMode", m_server->TurboMode());
	// Polling prefs
	myIniFile.WriteInt("poll", "PollUnderCursor", m_server->PollUnderCursor());
	myIniFile.WriteInt("poll", "PollForeground", m_server->PollForeground());
	myIniFile.WriteInt("poll", "PollFullScreen", m_server->PollFullScreen());

	myIniFile.WriteInt("poll", "OnlyPollConsole", m_server->PollConsoleOnly());
	myIniFile.WriteInt("poll", "OnlyPollOnEvent", m_server->PollOnEventOnly());
	myIniFile.WriteInt("poll", "MaxCpu2", m_server->MaxCpu());
	myIniFile.WriteInt("poll", "MaxFPS", m_server->MaxFPS());

	myIniFile.WriteInt("poll", "EnableDriver", m_server->Driver());
	myIniFile.WriteInt("poll", "EnableHook", m_server->Hook());
	myIniFile.WriteInt("poll", "EnableVirtual", m_server->Virtual());
	int test = m_server->AutoCapt();
	myIniFile.WriteInt("poll", "autocapt", m_server->AutoCapt());	
}
