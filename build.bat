@echo off

:: For traditional MinGW, set prefix32 to empty string
:: For mingw-w32, set prefix32 to i686-w64-mingw32-
:: For mingw-w64, set prefix64 to x86_64-w64-mingw32-

set prefix32=i686-w64-mingw32-
set prefix64=x86_64-w64-mingw32-
set l10n=en-US es-ES gl-ES

taskkill /IM KillKeys.exe

if not exist build. mkdir build

if "%1" == "all" (
	%prefix32%windres -o build\killkeys.o include\killkeys.rc
	%prefix32%gcc -o build\ini.exe include\ini.c -lshlwapi
	
	@echo.
	echo Building binaries
	%prefix32%gcc -o "build\KillKeys.exe" killkeys.c build\killkeys.o -mwindows -lshlwapi -lwininet -O2 -s
	if not exist "build\KillKeys.exe". exit /b
	
	if "%2" == "x64" (
		if not exist "build\x64". mkdir "build\x64"
		%prefix64%windres -o build\x64\killkeys.o include\killkeys.rc
		%prefix64%gcc -o "build\x64\KillKeys.exe" killkeys.c build\x64\killkeys.o -mwindows -lshlwapi -lwininet -O2 -s
		if not exist "build\x64\KillKeys.exe". exit /b
	)
	
	for %%f in (%l10n%) do (
		@echo.
		echo Putting together %%f
		if not exist "build\%%f\KillKeys". mkdir "build\%%f\KillKeys"
		copy "build\KillKeys.exe" "build\%%f\KillKeys"
		copy "localization\%%f\info.txt" "build\%%f\KillKeys"
		copy "KillKeys.ini" "build\%%f\KillKeys"
		"build\ini.exe" "build\%%f\KillKeys\KillKeys.ini" KillKeys Language %%f
		if "%2" == "x64" (
			if not exist "build\x64\%%f\KillKeys". mkdir "build\x64\%%f\KillKeys"
			copy "build\x64\KillKeys.exe" "build\x64\%%f\KillKeys"
			copy "build\%%f\KillKeys\info.txt" "build\x64\%%f\KillKeys"
			copy "build\%%f\KillKeys\KillKeys.ini" "build\x64\%%f\KillKeys\KillKeys.ini"
		)
	)
	
	@echo.
	echo Building installer
	if "%2" == "x64" (
		makensis /V2 /Dx64 installer.nsi
	) else (
		makensis /V2 installer.nsi
	)
) else if "%1" == "x64" (
	if not exist "build\x64". mkdir "build\x64"
	%prefix64%windres -o build\x64\killkeys.o include\killkeys.rc
	%prefix64%gcc -o KillKeys.exe killkeys.c build\x64\killkeys.o -mwindows -lshlwapi -lwininet -g -DDEBUG
) else (
	%prefix32%windres -o build\killkeys.o include\killkeys.rc
	%prefix32%gcc -o KillKeys.exe killkeys.c build\killkeys.o -mwindows -lshlwapi -lwininet -g -DDEBUG
	
	if "%1" == "run" (
		start KillKeys.exe
	)
)
