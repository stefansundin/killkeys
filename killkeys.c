/*
	Copyright (C) 2010  Stefan Sundin (recover89@gmail.com)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
*/

#define UNICODE
#define _UNICODE
#define _WIN32_IE 0x0600

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlwapi.h>

//App
#define APP_NAME            L"KillKeys"
#define APP_VERSION         "1.2"
#define APP_URL             L"http://code.google.com/p/killkeys/"
#define APP_UPDATE_STABLE   L"http://killkeys.googlecode.com/svn/wiki/latest-stable.txt"
#define APP_UPDATE_UNSTABLE L"http://killkeys.googlecode.com/svn/wiki/latest-unstable.txt"

//Messages
#define WM_TRAY                WM_USER+1
#define SWM_TOGGLE             WM_APP+1
#define SWM_HIDE               WM_APP+2
#define SWM_AUTOSTART_ON       WM_APP+3
#define SWM_AUTOSTART_OFF      WM_APP+4
#define SWM_AUTOSTART_HIDE_ON  WM_APP+5
#define SWM_AUTOSTART_HIDE_OFF WM_APP+6
#define SWM_SETTINGS           WM_APP+7
#define SWM_CHECKFORUPDATE     WM_APP+8
#define SWM_UPDATE             WM_APP+9
#define SWM_ABOUT              WM_APP+10
#define SWM_EXIT               WM_APP+11

//Stuff missing in MinGW
#define HWND_MESSAGE ((HWND)-3)
#ifndef NIIF_USER
#define NIIF_USER 4
#define NIN_BALLOONSHOW        WM_USER+2
#define NIN_BALLOONHIDE        WM_USER+3
#define NIN_BALLOONTIMEOUT     WM_USER+4
#define NIN_BALLOONUSERCLICK   WM_USER+5
#endif

//Boring stuff
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hinst = NULL;
HWND g_hwnd = NULL;
UINT WM_TASKBARCREATED = 0;
UINT WM_UPDATESETTINGS = 0;
UINT WM_ADDTRAY = 0;
UINT WM_HIDETRAY = 0;

//Cool stuff
HHOOK keyhook = NULL;
int *keys = NULL;
int numkeys = 0;
int *keys_fullscreen = NULL;
int numkeys_fullscreen = 0;

//Include stuff
#include "localization/strings.h"
#include "include/error.c"
#include "include/autostart.c"
#include "include/tray.c"
#include "include/update.c"

//Entry point
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {
	g_hinst = hInst;
	
	//Check command line
	if (!strcmp(szCmdLine,"-hide")) {
		hide = 1;
	}
	
	//Look for previous instance
	WM_UPDATESETTINGS = RegisterWindowMessage(L"UpdateSettings");
	WM_ADDTRAY = RegisterWindowMessage(L"AddTray");
	WM_HIDETRAY = RegisterWindowMessage(L"HideTray");
	HWND previnst = FindWindow(APP_NAME, NULL);
	if (previnst != NULL) {
		PostMessage(previnst, WM_UPDATESETTINGS, 0, 0);
		PostMessage(previnst, (hide?WM_HIDETRAY:WM_ADDTRAY), 0, 0);
		return 0;
	}
	
	//Load settings
	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL, path, sizeof(path)/sizeof(wchar_t));
	PathRemoveFileSpec(path);
	wcscat(path, L"\\"APP_NAME".ini");
	wchar_t txt[10];
	GetPrivateProfileString(APP_NAME, L"Language", L"en-US", txt, sizeof(txt)/sizeof(wchar_t), path);
	int i;
	for (i=0; languages[i].code != NULL; i++) {
		if (!wcsicmp(txt,languages[i].code)) {
			l10n = languages[i].strings;
			break;
		}
	}
	
	//Create window
	WNDCLASSEX wnd;
	wnd.cbSize = sizeof(WNDCLASSEX);
	wnd.style = 0;
	wnd.lpfnWndProc = WindowProc;
	wnd.cbClsExtra = 0;
	wnd.cbWndExtra = 0;
	wnd.hInstance = hInst;
	wnd.hIcon = NULL;
	wnd.hIconSm = NULL;
	wnd.hCursor = LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR|LR_SHARED);
	wnd.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wnd.lpszMenuName = NULL;
	wnd.lpszClassName = APP_NAME;
	RegisterClassEx(&wnd);
	g_hwnd = CreateWindowEx(0, wnd.lpszClassName, APP_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hInst, NULL);
	
	//Tray icon
	InitTray();
	UpdateTray();
	
	//Hook keyboard
	HookKeyboard();
	
	//Add tray if hooking failed, even though -hide was supplied
	if (hide && !enabled()) {
		hide = 0;
		UpdateTray();
	}
	
	//Check for update
	GetPrivateProfileString(L"Update", L"CheckOnStartup", L"0", txt, sizeof(txt)/sizeof(wchar_t), path);
	int checkforupdate = _wtoi(txt);
	if (checkforupdate) {
		CheckForUpdate(0);
	}
	
	//Message loop
	MSG msg;
	while (GetMessage(&msg,NULL,0,0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

//Monitors
RECT *monitors = NULL;
int nummonitors = 0;

int monitors_alloc = 0;
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	//Make sure we have enough space allocated
	if (nummonitors == monitors_alloc) {
		monitors_alloc++;
		monitors = realloc(monitors, monitors_alloc*sizeof(RECT));
		if (monitors == NULL) {
			Error(L"realloc(monitors)", L"Out of memory?", 0, TEXT(__FILE__), __LINE__);
			return FALSE;
		}
	}
	//Add monitor
	monitors[nummonitors++] = *lprcMonitor;
	return TRUE;
}

//Fullscreen
int IsFullscreen(HWND window) {
	if (GetWindowLongPtr(window,GWL_STYLE)&WS_CAPTION) {
		return 0;
	}
	
	wchar_t classname[9] = L"";
	GetClassName(window, classname, sizeof(classname)/sizeof(wchar_t));
	if (!wcscmp(classname,L"WorkerW") || !wcscmp(classname,L"Progman")) {
		return 0;
	}
	
	//Get window size
	RECT wnd;
	if (GetWindowRect(window,&wnd) == 0) {
		//Error(L"GetWindowRect(&wnd)", L"IsFullscreen()", GetLastError(), TEXT(__FILE__), __LINE__);
		return 0;
	}
	
	//Enumerate monitors
	nummonitors = 0;
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
	//Loop monitors
	int i;
	for (i=0; i < nummonitors; i++) {
		if (wnd.left == monitors[i].left && wnd.top == monitors[i].top && wnd.right == monitors[i].right && wnd.bottom == monitors[i].bottom) {
			return 1;
		}
	}
}

//Hook
__declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
		int vkey = ((PKBDLLHOOKSTRUCT)lParam)->vkCode;
		
		int stop = 0;
		int i;
		
		HWND window = GetForegroundWindow();
		int fullscreen = IsFullscreen(window);
		
		//Check if the key should be blocked
		if (!fullscreen) {
			for (i=0; i < numkeys; i++) {
				if (vkey == keys[i]) {
					stop = 1;
					break;
				}
			}
		}
		else {
			for (i=0; i < numkeys_fullscreen; i++) {
				if (vkey == keys_fullscreen[i]) {
					stop = 1;
					break;
				}
			}
		}
		
		//Stop this key
		if (stop) {
			return 1;
		}
	}
	
    return CallNextHookEx(NULL, nCode, wParam, lParam); 
}

int HookKeyboard() {
	if (keyhook) {
		//Keyboard already hooked
		return 1;
	}
	
	//Update settings
	SendMessage(g_hwnd, WM_UPDATESETTINGS, 0, 0);
	
	//Name decoration
	//This is really an ugly hack to make both MinGW/mingw-w32 and mingw-w64 use their respective name decorations
	#ifdef _WIN64
	#define KeyhookNameDecoration ""    //mingw-w64
	#else
	#define KeyhookNameDecoration "@12" //MinGW/mingw-w32
	#endif
	
	//Get address to keyboard hook
	HOOKPROC procaddr = (HOOKPROC)GetProcAddress(g_hinst, "LowLevelKeyboardProc"KeyhookNameDecoration);
	if (procaddr == NULL) {
		Error(L"GetProcAddress('LowLevelKeyboardProc'"KeyhookNameDecoration")", L"Could not find hook function. Try restarting "APP_NAME".", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Set up the hook
	keyhook = SetWindowsHookEx(WH_KEYBOARD_LL, procaddr, g_hinst, 0);
	if (keyhook == NULL) {
		Error(L"SetWindowsHookEx(WH_KEYBOARD_LL)", L"Could not hook keyboard. Another program might be interfering.", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Success
	UpdateTray();
	return 0;
}

int UnhookKeyboard() {
	if (!keyhook) {
		//Keyboard not hooked
		return 1;
	}
	
	//Reset keys
	numkeys = 0;
	numkeys_fullscreen = 0;
	free(keys);
	free(keys_fullscreen);
	keys = NULL;
	keys_fullscreen = NULL;
	
	//Remove keyboard hook
	if (UnhookWindowsHookEx(keyhook) == 0) {
		Error(L"UnhookWindowsHookEx(keyhook)", L"Could not unhook keyboard. Try restarting "APP_NAME".", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Success
	keyhook = NULL;
	UpdateTray();
	return 0;
}

int enabled() {
	return keyhook != NULL;
}

void ToggleState() {
	if (enabled()) {
		UnhookKeyboard();
	}
	else {
		HookKeyboard();
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_TRAY) {
		if (lParam == WM_LBUTTONDOWN || lParam == WM_LBUTTONDBLCLK) {
			ToggleState();
		}
		else if (lParam == WM_RBUTTONDOWN) {
			ShowContextMenu(hwnd);
		}
		else if (lParam == NIN_BALLOONUSERCLICK) {
			hide=0;
			SendMessage(hwnd, WM_COMMAND, SWM_UPDATE, 0);
		}
		else if (lParam == NIN_BALLOONTIMEOUT) {
			if (hide) {
				RemoveTray();
			}
		}
	}
	else if (msg == WM_UPDATESETTINGS) {
		//Load settings
		wchar_t path[MAX_PATH];
		GetModuleFileName(NULL, path, sizeof(path)/sizeof(wchar_t));
		PathRemoveFileSpec(path);
		wcscat(path, L"\\"APP_NAME".ini");
		wchar_t txt[1000];
		int keys_alloc = 0;
		int temp;
		int numread;
		//Keys
		GetPrivateProfileString(APP_NAME, L"Keys", L"", txt, sizeof(txt)/sizeof(wchar_t), path);
		wchar_t *pos = txt;
		numkeys = 0;
		while (*pos != '\0' && swscanf(pos,L"%02X%n",&temp,&numread) != EOF) {
			//Make sure we have enough space
			if (numkeys == keys_alloc) {
				keys_alloc += 100;
				keys = realloc(keys,keys_alloc*sizeof(int));
				if (keys == NULL) {
					Error(L"realloc(keys)", L"Out of memory?", GetLastError(), TEXT(__FILE__), __LINE__);
					break;
				}
			}
			//Store key
			keys[numkeys++] = temp;
			pos += numread;
		}
		//Keys_Fullscreen
		GetPrivateProfileString(APP_NAME, L"Keys_Fullscreen", L"", txt, sizeof(txt)/sizeof(wchar_t), path);
		pos = txt;
		numkeys_fullscreen = 0;
		keys_alloc = 0;
		while (*pos != '\0' && swscanf(pos,L"%02X%n",&temp,&numread) != EOF) {
			//Make sure we have enough space
			if (numkeys_fullscreen == keys_alloc) {
				keys_alloc += 100;
				keys_fullscreen = realloc(keys_fullscreen,keys_alloc*sizeof(int));
				if (keys_fullscreen == NULL) {
					Error(L"realloc(keys_fullscreen)", L"Out of memory?", GetLastError(), TEXT(__FILE__), __LINE__);
					break;
				}
			}
			//Store key
			keys_fullscreen[numkeys_fullscreen++] = temp;
			pos += numread;
		}
		//Language
		GetPrivateProfileString(APP_NAME, L"Language", L"en-US", txt, sizeof(txt)/sizeof(wchar_t), path);
		int i;
		for (i=0; languages[i].code != NULL; i++) {
			if (!wcsicmp(txt,languages[i].code)) {
				l10n = languages[i].strings;
				break;
			}
		}
	}
	else if (msg == WM_ADDTRAY) {
		hide = 0;
		UpdateTray();
	}
	else if (msg == WM_HIDETRAY) {
		hide = 1;
		RemoveTray();
	}
	else if (msg == WM_TASKBARCREATED) {
		tray_added = 0;
		UpdateTray();
	}
	else if (msg == WM_COMMAND) {
		int wmId=LOWORD(wParam), wmEvent=HIWORD(wParam);
		if (wmId == SWM_TOGGLE) {
			ToggleState();
		}
		else if (wmId == SWM_HIDE) {
			hide = 1;
			RemoveTray();
		}
		else if (wmId == SWM_AUTOSTART_ON) {
			SetAutostart(1, 0);
		}
		else if (wmId == SWM_AUTOSTART_OFF) {
			SetAutostart(0, 0);
		}
		else if (wmId == SWM_AUTOSTART_HIDE_ON) {
			SetAutostart(1, 1);
		}
		else if (wmId == SWM_AUTOSTART_HIDE_OFF) {
			SetAutostart(1, 0);
		}
		else if (wmId == SWM_SETTINGS) {
			wchar_t path[MAX_PATH];
			GetModuleFileName(NULL, path, sizeof(path)/sizeof(wchar_t));
			PathRemoveFileSpec(path);
			wcscat(path, L"\\"APP_NAME".ini");
			ShellExecute(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
		}
		else if (wmId == SWM_CHECKFORUPDATE) {
			CheckForUpdate(1);
		}
		else if (wmId == SWM_UPDATE) {
			if (MessageBox(NULL,l10n->update_dialog,APP_NAME,MB_ICONINFORMATION|MB_YESNO|MB_SYSTEMMODAL) == IDYES) {
				ShellExecute(NULL, L"open", APP_URL, NULL, NULL, SW_SHOWNORMAL);
			}
		}
		else if (wmId == SWM_ABOUT) {
			wchar_t txt[1000], buffer[1000];
			if (enabled()) {
				wcscpy(txt, l10n->about_keys1);
				int i;
				for (i=0; i < numkeys; i++) {
					swprintf(txt, L"%s %02X", txt, keys[i]);
				}
				wcscat(txt, L"\n");
				wcscat(txt, l10n->about_keys2);
				for (i=0; i < numkeys_fullscreen; i++) {
					swprintf(txt, L"%s %02X", txt, keys_fullscreen[i]);
				}
			}
			else {
				wcscpy(txt, l10n->about_keys3);
			}
			swprintf(buffer, l10n->about, txt);
			MessageBox(NULL, buffer, l10n->about_title, MB_ICONINFORMATION|MB_OK);
		}
		else if (wmId == SWM_EXIT) {
			DestroyWindow(hwnd);
		}
	}
	else if (msg == WM_DESTROY) {
		showerror = 0;
		UnhookKeyboard();
		RemoveTray();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
