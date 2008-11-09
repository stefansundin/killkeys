/*
	KillKeys - Disable keys from working
	Copyright (C) 2008  Stefan Sundin (recover89@gmail.com)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

static int *keys;
static int numkeys=0;
static int *keys_fullscreen;
static int numkeys_fullscreen=0;
static FILE *log;

static char txt[100];

char* GetTimestamp(char *buf, size_t maxsize, char *format) {
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	strftime(buf,maxsize,format,timeinfo);
	return buf;
}

_declspec(dllexport) void Settings(int *new_keys, int new_numkeys, int *new_keys_fullscreen, int new_numkeys_fullscreen) {
	keys=new_keys;
	numkeys=new_numkeys;
	keys_fullscreen=new_keys_fullscreen;
	numkeys_fullscreen=new_numkeys_fullscreen;
}

_declspec(dllexport) LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
		int vkey=((PKBDLLHOOKSTRUCT)lParam)->vkCode;
		HWND hwnd=GetForegroundWindow();
		int stop=0;
		int fullscreen=0;
		int i;
		
		//Get window and desktop size
		RECT wnd;
		if (GetWindowRect(hwnd,&wnd) == 0) {
			fprintf(log,"Error: GetWindowRect() failed (error code: %d) in file %s, line %d.\n",GetLastError(),__FILE__,__LINE__);
			fflush(log);
			return 0;
		}
		RECT desk;
		if (GetWindowRect(GetDesktopWindow(),&desk) == 0) {
			fprintf(log,"Error: GetWindowRect() failed (error code: %d) in file %s, line %d.\n",GetLastError(),__FILE__,__LINE__);
			fflush(log);
			return 0;
		}
		
		//Are we in a fullscreen window?
		if (wnd.left == desk.left && wnd.top == desk.top && wnd.right == desk.right && wnd.bottom == desk.bottom) {
			fullscreen=1;
		}
		//The desktop (explorer.exe) doesn't count as a fullscreen window.
		//HWND desktop = FindWindow("WorkerW", NULL); //This is the window that's focused after pressing [the windows button]+D
		HWND desktop = FindWindow("Progman", "Program Manager");
		if (fullscreen && desktop != NULL) {
			DWORD desktop_pid, hwnd_pid;
			GetWindowThreadProcessId(desktop,&desktop_pid);
			GetWindowThreadProcessId(hwnd,&hwnd_pid);
			if (desktop_pid == hwnd_pid) {
				fullscreen=0;
			}
		}
		
		//Check if the key should be blocked
		if (!fullscreen) {
			for (i=0; i < numkeys; i++) {
				if (vkey == keys[i]) {
					stop=1;
					break;
				}
			}
		}
		else {
			for (i=0; i < numkeys_fullscreen; i++) {
				if (vkey == keys_fullscreen[i]) {
					stop=1;
					break;
				}
			}
		}
		
		//Block the key
		if (stop) {
			fprintf(log,"%s Blocking key %02X%s.\n",GetTimestamp(txt,sizeof(txt),"[%Y-%m-%d %H:%M:%S]"),vkey,(fullscreen?" in fullscreen window":""));
			fflush(log);
			
			//Stop this key
			return 1;
		}
	}
	
    return CallNextHookEx(NULL, nCode, wParam, lParam); 
}

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		//Open log
		log=fopen("killkeys-log.txt","ab");
		fprintf(log,"\n%s ",GetTimestamp(txt,sizeof(txt),"[%Y-%m-%d %H:%M:%S]"));
		fprintf(log,"New session.\n");
		fflush(log);
	}
	else if (reason == DLL_PROCESS_DETACH) {
		//Close log
		fclose(log);
	}
	
	return TRUE;
}
