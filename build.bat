
taskkill /IM KillKeys.exe

gcc -c keyhook.c
gcc -shared -o keyhook.dll keyhook.o

windres -o resources.o resources.rc
gcc -o KillKeys killkeys.c resources.o -mwindows -lshlwapi

strip KillKeys.exe
strip keyhook.dll

upx --best -qq KillKeys.exe
upx --best -qq keyhook.dll
