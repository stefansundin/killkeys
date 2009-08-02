@echo off

taskkill /IM KillKeys.exe

if not exist build (
	mkdir build
)

windres -o build\resources.o include\resources.rc

if "%1" == "all" (
	gcc -o build\ini.exe include\ini.c -lshlwapi
	
	@echo.
	echo Building binaries
	if not exist "build\en-US\KillKeys" (
		mkdir "build\en-US\KillKeys"
	)
	gcc -o "build\en-US\KillKeys\KillKeys.exe" killkeys.c build\resources.o -mwindows -lshlwapi -lwininet
	if not exist "build\en-US\KillKeys\KillKeys.exe" (
		exit /b
	)
	strip "build\en-US\KillKeys\KillKeys.exe"
	
	for /D %%f in (localization/*) do (
		@echo.
		echo Putting together %%f
		if not %%f == en-US (
			if not exist "build\%%f\KillKeys" (
				mkdir "build\%%f\KillKeys"
			)
			copy "build\en-US\KillKeys\KillKeys.exe" "build\%%f\KillKeys"
		)
		copy "localization\%%f\info.txt" "build\%%f\KillKeys"
		copy KillKeys.ini "build\%%f\KillKeys"
		"build\ini.exe" "build\%%f\KillKeys\KillKeys.ini" KillKeys Language %%f
	)
	
	@echo.
	echo Building installer
	makensis /V2 installer.nsi
) else (
	gcc -o KillKeys.exe killkeys.c build\resources.o -mwindows -lshlwapi -lwininet -DDEBUG
	
	if "%1" == "run" (
		start KillKeys.exe
	)
	if "%1" == "hide" (
		start KillKeys.exe -hide
	)
)
