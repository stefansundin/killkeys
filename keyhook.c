/*
	KillKeys - Disable certain keys from working
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

static int keys[100];
static int numkeys=0;

static char msg[100];

_declspec(dllexport) void Settings(int *new_keys, int length) {
	int i;
	for (i=0; i < length; i++) {
		keys[i]=new_keys[i];
	}
	numkeys=length;
}

_declspec(dllexport) LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		int vkey=((PKBDLLHOOKSTRUCT)lParam)->vkCode;
		
		int stop=0;
		int i;
		for (i=0; i < numkeys; i++) {
			if (vkey == keys[i]) {
				stop=1;
				break;
			}
		}
		
		if (stop) {
			//Open log
			FILE *output;
			if ((output=fopen("log-keyhook.txt","ab")) == NULL) {
				sprintf(msg,"fopen() failed in file %s, line %d.",__FILE__,__LINE__);
				MessageBox(NULL, msg, "KillKeys Error", MB_ICONERROR|MB_OK);
				return 1;
			}
			
			//Print timestamp
			time_t rawtime;
			struct tm *timeinfo;
			time(&rawtime);
			timeinfo=localtime(&rawtime);
			strftime(msg,sizeof(msg),"[%Y-%m-%d %H:%M:%S]",timeinfo);
			fprintf(output,"\n%s\n",msg);
			
			fprintf(output,"%02X pressed, stopped!\n",vkey);
			
			//Close log
			if (fclose(output) == EOF) {
				sprintf(msg,"fclose() failed in file %s, line %d.",__FILE__,__LINE__);
				fprintf(output,"%s\n",msg);
				fclose(output);
				MessageBox(NULL, msg, "KillKeys Error", MB_ICONERROR|MB_OK);
				return 1;
			}
			
			//Stop this key
			return 1;
		}
	}
	
    return CallNextHookEx(NULL, nCode, wParam, lParam); 
}

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD reason, LPVOID reserved) {
	return TRUE;
}
