/*
	KillKeys - Disable keys from working
	Copyright (C) 2009  Stefan Sundin (recover89@gmail.com)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
*/

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#define _WIN32_IE 0x0600
#include <windows.h>
#include <shlwapi.h>
#include <wininet.h>

//App
#define APP_NAME      L"KillKeys"
#define APP_VERSION   "1.1"
#define APP_URL       L"http://killkeys.googlecode.com/"
#define APP_UPDATEURL L"http://killkeys.googlecode.com/svn/wiki/latest-stable.txt"

//Messages
#define WM_ICONTRAY            WM_USER+1
#define SWM_TOGGLE             WM_APP+1
#define SWM_HIDE               WM_APP+2
#define SWM_AUTOSTART_ON       WM_APP+3
#define SWM_AUTOSTART_OFF      WM_APP+4
#define SWM_AUTOSTART_HIDE_ON  WM_APP+5
#define SWM_AUTOSTART_HIDE_OFF WM_APP+6
#define SWM_UPDATE             WM_APP+7
#define SWM_ABOUT              WM_APP+8
#define SWM_EXIT               WM_APP+9

//Stuff missing in MinGW
#define HWND_MESSAGE ((HWND)-3)
#define NIIF_USER 4
#define NIN_BALLOONSHOW        WM_USER+2
#define NIN_BALLOONHIDE        WM_USER+3
#define NIN_BALLOONTIMEOUT     WM_USER+4
#define NIN_BALLOONUSERCLICK   WM_USER+5

//Localization
struct strings {
	wchar_t *menu_enable;
	wchar_t *menu_disable;
	wchar_t *menu_hide;
	wchar_t *menu_autostart;
	wchar_t *menu_update;
	wchar_t *menu_about;
	wchar_t *menu_exit;
	wchar_t *tray_enabled;
	wchar_t *tray_disabled;
	wchar_t *update_balloon;
	wchar_t *update_dialog;
	wchar_t *about_keys1;
	wchar_t *about_keys2;
	wchar_t *about_keys3;
	wchar_t *about_title;
	wchar_t *about;
};
#include "localization/strings.h"
struct strings *l10n = &en_US;

//Boring stuff
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
HICON icon[2];
NOTIFYICONDATA traydata;
UINT WM_TASKBARCREATED = 0;
UINT WM_UPDATESETTINGS = 0;
UINT WM_ADDTRAY = 0;
UINT WM_HIDETRAY = 0;
int tray_added = 0;
int hide = 0;
struct {
	int CheckForUpdate;
} settings = {0};
wchar_t txt[1000];

//Cool stuff
HINSTANCE hinstDLL = NULL;
HHOOK keyhook = NULL;
int *keys = NULL;
int numkeys = 0;
int *keys_fullscreen = NULL;
int numkeys_fullscreen = 0;
HWND progman = NULL;
HWND desktop = NULL;

//Error() and CheckForUpdate()
#include "include/error.h"
#include "include/update.h"

//Entry point
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {
	//Check command line
	if (!strcmp(szCmdLine,"-hide")) {
		hide = 1;
	}
	
	//Look for previous instance
	WM_UPDATESETTINGS = RegisterWindowMessage(L"UpdateSettings");
	WM_ADDTRAY = RegisterWindowMessage(L"AddTray");
	WM_HIDETRAY = RegisterWindowMessage(L"HideTray");
	HWND previnst = FindWindow(APP_NAME,NULL);
	if (previnst != NULL) {
		PostMessage(previnst, WM_UPDATESETTINGS, 0, 0);
		if (hide) {
			PostMessage(previnst, WM_HIDETRAY, 0, 0);
		}
		else {
			PostMessage(previnst, WM_ADDTRAY, 0, 0);
		}
		return 0;
	}
	
	//Load settings
	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL, path, sizeof(path)/sizeof(wchar_t));
	PathRenameExtension(path, L".ini");
	GetPrivateProfileString(L"Update", L"CheckForUpdate", L"0", txt, sizeof(txt)/sizeof(wchar_t), path);
	swscanf(txt, L"%d", &settings.CheckForUpdate);
	GetPrivateProfileString(APP_NAME, L"Language", L"en-US", txt, sizeof(txt)/sizeof(wchar_t), path);
	int i;
	for (i=0; i < num_languages; i++) {
		if (!wcscmp(txt,languages[i].code)) {
			l10n = languages[i].strings;
			break;
		}
	}
	
	//Create window class
	WNDCLASSEX wnd;
	wnd.cbSize = sizeof(WNDCLASSEX);
	wnd.style = 0;
	wnd.lpfnWndProc = WindowProc;
	wnd.cbClsExtra = 0;
	wnd.cbWndExtra = 0;
	wnd.hInstance = hInst;
	wnd.hIcon = NULL;
	wnd.hIconSm = NULL;
	wnd.hCursor = LoadImage(NULL,IDC_ARROW,IMAGE_CURSOR,0,0,LR_DEFAULTCOLOR|LR_SHARED);
	wnd.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wnd.lpszMenuName = NULL;
	wnd.lpszClassName = APP_NAME;
	
	//Register class
	RegisterClassEx(&wnd);
	
	//Create window
	HWND hwnd = CreateWindowEx(0, wnd.lpszClassName, APP_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hInst, NULL);
	
	//Load icons
	icon[0] = LoadImage(hInst,L"tray_disabled",IMAGE_ICON,0,0,LR_DEFAULTCOLOR);
	icon[1] = LoadImage(hInst,L"tray_enabled",IMAGE_ICON,0,0,LR_DEFAULTCOLOR);
	if (icon[0] == NULL || icon[1] == NULL) {
		Error(L"LoadImage('tray-*')", L"Fatal error.", GetLastError(), TEXT(__FILE__), __LINE__);
		PostQuitMessage(1);
	}
	
	//Create icondata
	traydata.cbSize = sizeof(NOTIFYICONDATA);
	traydata.uID = 0;
	traydata.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
	traydata.hWnd = hwnd;
	traydata.uCallbackMessage = WM_ICONTRAY;
	//Balloon tooltip
	traydata.uTimeout = 10000;
	wcsncpy(traydata.szInfoTitle, APP_NAME, sizeof(traydata.szInfoTitle)/sizeof(wchar_t));
	traydata.dwInfoFlags = NIIF_USER;
	
	//Register TaskbarCreated so we can re-add the tray icon if explorer.exe crashes
	WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");
	
	//Update tray icon
	UpdateTray();
	
	//Hook keyboard
	HookKeyboard();
	
	//Add tray if hooking failed, even though -hide was supplied
	if (hide && !keyhook) {
		hide = 0;
		UpdateTray();
	}
	
	//Check for update
	if (settings.CheckForUpdate) {
		CheckForUpdate();
	}
	
	//Message loop
	MSG msg;
	while (GetMessage(&msg,NULL,0,0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

void ShowContextMenu(HWND hwnd) {
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	
	//Toggle
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_TOGGLE, (keyhook?l10n->menu_disable:l10n->menu_enable));
	
	//Hide
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_HIDE, l10n->menu_hide);
	
	//Check autostart
	int autostart_enabled=0, autostart_hide=0;
	//Registry
	HKEY key;
	wchar_t autostart_value[MAX_PATH+10] = L"";
	DWORD len = sizeof(autostart_value);
	RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &key);
	RegQueryValueEx(key, APP_NAME, NULL, NULL, (LPBYTE)autostart_value, &len);
	RegCloseKey(key);
	//Compare
	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	swprintf(txt, L"\"%s\"", path);
	if (!wcscmp(txt,autostart_value)) {
		autostart_enabled=1;
	}
	else {
		swprintf(txt, L"\"%s\" -hide", path);
		if (!wcscmp(txt,autostart_value)) {
			autostart_enabled = 1;
			autostart_hide = 1;
		}
	}
	//Autostart
	HMENU hAutostartMenu = CreatePopupMenu();
	InsertMenu(hAutostartMenu, -1, MF_BYPOSITION|(autostart_enabled?MF_CHECKED:0), (autostart_enabled?SWM_AUTOSTART_OFF:SWM_AUTOSTART_ON), l10n->menu_autostart);
	InsertMenu(hAutostartMenu, -1, MF_BYPOSITION|(autostart_hide?MF_CHECKED:0), (autostart_hide?SWM_AUTOSTART_HIDE_OFF:SWM_AUTOSTART_HIDE_ON), l10n->menu_hide);
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_POPUP, (UINT)hAutostartMenu, l10n->menu_autostart);
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	
	//Update
	if (update) {
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_UPDATE, l10n->menu_update);
	}
	
	//About
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ABOUT, l10n->menu_about);
	
	//Exit
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, l10n->menu_exit);

	//Track menu
	SetForegroundWindow(hwnd);
	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
	DestroyMenu(hMenu);
}

int UpdateTray() {
	wcsncpy(traydata.szTip, (keyhook?l10n->tray_enabled:l10n->tray_disabled), sizeof(traydata.szTip)/sizeof(wchar_t));
	traydata.hIcon = icon[keyhook?1:0];
	
	//Only add or modify if not hidden or if balloon will be displayed
	if (!hide || traydata.uFlags&NIF_INFO) {
		int tries = 0; //Try at least ten times, sleep 100 ms between each attempt
		while (Shell_NotifyIcon((tray_added?NIM_MODIFY:NIM_ADD),&traydata) == FALSE) {
			tries++;
			if (tries >= 10) {
				Error(L"Shell_NotifyIcon(NIM_ADD/NIM_MODIFY)", L"Failed to update tray icon.", GetLastError(), TEXT(__FILE__), __LINE__);
				return 1;
			}
			Sleep(100);
		}
		
		//Success
		tray_added = 1;
	}
	return 0;
}

int RemoveTray() {
	if (!tray_added) {
		//Tray not added
		return 1;
	}
	
	if (Shell_NotifyIcon(NIM_DELETE,&traydata) == FALSE) {
		Error(L"Shell_NotifyIcon(NIM_DELETE)", L"Failed to remove tray icon.", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Success
	tray_added=0;
	return 0;
}

void SetAutostart(int on, int hide) {
	//Open key
	HKEY key;
	int error = RegCreateKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,NULL,0,KEY_SET_VALUE,NULL,&key,NULL);
	if (error != ERROR_SUCCESS) {
		Error(L"RegCreateKeyEx(HKEY_CURRENT_USER,'Software\\Microsoft\\Windows\\CurrentVersion\\Run')", L"Error opening the registry.", error, TEXT(__FILE__), __LINE__);
		return;
	}
	if (on) {
		//Get path
		wchar_t path[MAX_PATH];
		if (GetModuleFileName(NULL,path,MAX_PATH) == 0) {
			Error(L"GetModuleFileName(NULL)", L"SetAutostart()", GetLastError(), TEXT(__FILE__), __LINE__);
			return;
		}
		//Add
		wchar_t value[MAX_PATH+10];
		swprintf(value, (hide?L"\"%s\" -hide":L"\"%s\""), path);
		error = RegSetValueEx(key,APP_NAME,0,REG_SZ,(LPBYTE)value,(wcslen(value)+1)*sizeof(wchar_t));
		if (error != ERROR_SUCCESS) {
			Error(L"RegSetValueEx('"APP_NAME"')", L"SetAutostart()", error, TEXT(__FILE__), __LINE__);
			return;
		}
	}
	else {
		//Remove
		error = RegDeleteValue(key,APP_NAME);
		if (error != ERROR_SUCCESS) {
			Error(L"RegDeleteValue('"APP_NAME"')", L"SetAutostart()", error, TEXT(__FILE__), __LINE__);
			return;
		}
	}
	//Close key
	RegCloseKey(key);
}

//Monitors
RECT *monitors = NULL;
int nummonitors = 0;

int monitors_alloc = 0;
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	//Make sure we have enough space allocated
	if (nummonitors == monitors_alloc) {
		monitors_alloc++;
		monitors = realloc(monitors,monitors_alloc*sizeof(RECT));
		if (monitors == NULL) {
			Error(L"realloc(monitors)", L"Out of memory?", 0, TEXT(__FILE__), __LINE__);
			return FALSE;
		}
	}
	//Add monitor
	monitors[nummonitors++] = *lprcMonitor;
	return TRUE;
}


//Hook
_declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
		int vkey = ((PKBDLLHOOKSTRUCT)lParam)->vkCode;
		
		int stop = 0;
		int fullscreen = 0;
		int i;
		
		HWND window = GetForegroundWindow();
		//Check if this window is fullscreen
		if (!(GetWindowLongPtr(window,GWL_STYLE)&WS_CAPTION)) {
			if (!IsWindow(progman)) {
				progman = FindWindow(L"Progman",L"Program Manager");
			}
			if (!IsWindow(desktop)) {
				desktop = FindWindow(L"WorkerW",NULL);
			}
			if (window != progman && window != desktop) {
				//Enumerate monitors
				nummonitors = 0;
				EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
				//Get window size
				RECT wnd;
				if (GetWindowRect(window,&wnd) == 0) {
					Error(L"GetWindowRect(&wnd)", L"LowLevelKeyboardProc()", GetLastError(), TEXT(__FILE__), __LINE__);
				}
				//Loop monitors
				int i;
				for (i=0; i < nummonitors; i++) {
					if (wnd.left == monitors[i].left && wnd.top == monitors[i].top && wnd.right == monitors[i].right && wnd.bottom == monitors[i].bottom) {
						fullscreen = 1;
						break;
					}
				}
			}
		}
		
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
		//Hook already installed
		return 1;
	}
	
	//Update settings
	SendMessage(traydata.hWnd, WM_UPDATESETTINGS, 0, 0);
	
	//Load library
	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL, path, sizeof(path)/sizeof(wchar_t));
	hinstDLL = LoadLibrary(path);
	if (hinstDLL == NULL) {
		Error(L"LoadLibrary()", L"Check the "APP_NAME" website if there is an update, if the latest version doesn't fix this, please report it.", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Get address to keyboard hook (beware name mangling)
	HOOKPROC procaddr = (HOOKPROC)GetProcAddress(hinstDLL,"LowLevelKeyboardProc@12");
	if (procaddr == NULL) {
		Error(L"GetProcAddress('LowLevelKeyboardProc@12')", L"Check the "APP_NAME" website if there is an update, if the latest version doesn't fix this, please report it.", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Set up the hook
	keyhook = SetWindowsHookEx(WH_KEYBOARD_LL,procaddr,hinstDLL,0);
	if (keyhook == NULL) {
		Error(L"SetWindowsHookEx(WH_KEYBOARD_LL)", L"Check the "APP_NAME" website if there is an update, if the latest version doesn't fix this, please report it.", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Success
	UpdateTray();
	return 0;
}

int UnhookKeyboard() {
	if (!keyhook) {
		//Hook not installed
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
		Error(L"UnhookWindowsHookEx(keyhook)", L"Check the "APP_NAME" website if there is an update, if the latest version doesn't fix this, please report it.", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Unload library
	if (FreeLibrary(hinstDLL) == 0) {
		Error(L"FreeLibrary()", L"Check the "APP_NAME" website if there is an update, if the latest version doesn't fix this, please report it.", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Success
	keyhook = NULL;
	UpdateTray();
	return 0;
}

void ToggleState() {
	if (keyhook) {
		UnhookKeyboard();
	}
	else {
		HookKeyboard();
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_ICONTRAY) {
		if (lParam == WM_LBUTTONDOWN || lParam == WM_LBUTTONDBLCLK) {
			ToggleState();
		}
		else if (lParam == WM_RBUTTONDOWN) {
			ShowContextMenu(hwnd);
		}
		else if (lParam == NIN_BALLOONTIMEOUT) {
			if (hide) {
				RemoveTray();
			}
		}
		else if (lParam == NIN_BALLOONUSERCLICK) {
			hide=0;
			SendMessage(hwnd, WM_COMMAND, SWM_UPDATE, 0);
		}
	}
	else if (msg == WM_UPDATESETTINGS) {
		//Load settings
		wchar_t path[MAX_PATH];
		GetModuleFileName(NULL, path, sizeof(path)/sizeof(wchar_t));
		PathRenameExtension(path, L".ini");
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
		for (i=0; i < num_languages; i++) {
			if (!wcscmp(txt,languages[i].code)) {
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
		else if (wmId == SWM_UPDATE) {
			if (MessageBox(NULL,l10n->update_dialog,APP_NAME,MB_ICONINFORMATION|MB_YESNO|MB_SYSTEMMODAL) == IDYES) {
				ShellExecute(NULL, L"open", APP_URL, NULL, NULL, SW_SHOWNORMAL);
			}
		}
		else if (wmId == SWM_ABOUT) {
			if (keyhook) {
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
			wchar_t buffer[1000];
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
