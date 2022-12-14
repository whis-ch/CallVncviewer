//===========================================================================
//	FullScreen Titlebar Header
//	2004 - All rights reservered
//===========================================================================
//
//	Project/Product :	FullScreenTitlebar
//  FileName		:	FullScreenTitleBar.h
//	Author(s)		:	Lars Werner
//  Homepage		:	http://lars.werner.no
//
//	Description		:	Declarations of the functions in the fullscreen window
//                  
//	Classes			:	CTitleBar
//
//	History
//	Vers.  Date      Aut.  Type     Description
//  -----  --------  ----  -------  -----------------------------------------
//	1.00   20 01 04  LW    Create   Original
//===========================================================================

//Include this .h only once
#pragma once

//All settings is stored here.
#include "FullScreenTitleBarConst.h"
#include "multimon.h"

class VNCOptions;

class CTitleBar
{
public:
	CTitleBar();
	CTitleBar(HINSTANCE hInst, HWND ParentWindow, bool Fit); //Creation
	virtual ~CTitleBar(); //Destruction

	void Create(HINSTANCE hInst, HWND ParentWindow, bool Fit, VNCOptions* opts);

	void SetText(LPTSTR TextOut); //Set the header text eg: hostname, windowtitle ect...

	void DisplayWindow(BOOL Show, BOOL SetHideFlag=FALSE); //Variable like ShowWindow but it triggers the scrolling feature
    void MoveToMonitor(HMONITOR hmon); // 7 May 2008 jdp

	//Returns the window - This is stored in the header cause of it size :)
	HWND GetSafeHwnd()
	{
		return m_hWnd;
	};

private:
	//Init if default variables
	void Init();

	//Creates the window
	void CreateDisplay();

	//Callbacks from buttons, timers, draw, ect ect
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	//StartPaint/EndPaint routine...
	void Draw();
	void SetScale();
	int ScaleDpi(int i);
	int dpi;
	HMONITOR hLastMonitor;

	// DPI Scaled 
	int tbWidth = ctbWidth;
	int tbHeigth = ctbHeigth;
	int tbcxPicture = ctbcxPicture;
	int tbcyPicture = ctbcyPicture;
	int tbTopSpace = ctbTopSpace;
	int tbLeftSpace = ctbLeftSpace;
	int tbRightSpace = ctbRightSpace;
	int tbButtonSpace = ctbButtonSpace;

	//Default variabled used for creation of the window
	HINSTANCE hInstance;
	HWND Parent;
	HWND m_hWnd;

	//Variables for scrolling of the window
	BOOL SlideDown;
	BOOL AutoHide; //Is the pin pushed in or not...
	BOOL Fit;
	BOOL HideAfterSlide; //TRUE = Hide the dialog after slide
	int IntAutoHideCounter;

	//Routines to load pictures and free pictures
	void LoadPictures();
	void FreePictures();

	//Pictures for the menubar
	HBITMAP hClose;
	HBITMAP hMinimize;
	HBITMAP hMaximize;
	HBITMAP hPinUp;
	HBITMAP hPinDown;
	HBITMAP hFitScreen;
	HBITMAP hNoScaleScreen;
	HBITMAP hPhoto;
	HBITMAP hSwitchMonitor;

	HWND Pin;
	HWND Close;
	HWND Maximize;
	HWND Minimize;

	HWND Screen;
	HWND Photo;
	HWND SwitchMonitor;

	HWND ScreenTip;
	HWND PhotoTip;
	HWND SwitchMonitorTip;


	//Text to show on titlebar and it corespondent font! :)
	LPTSTR Text;
	HFONT Font;
	int MonitorTop;

	void CreateToolTipForRect(HWND hwndParent, HWND hwndTip,char * text);
	VNCOptions* m_opts;
};
