@echo off

taskkill /IM KillKeys.exe

if not exist build (
	mkdir build
)

windres -o build/resources.o resources.rc

if "%1" == "all" (
	echo Building all
	
	for /D %%f in (localization/*) do (
		@echo.
		echo Building %%f
		if not exist "build/%%f/KillKeys" (
			mkdir "build\%%f\KillKeys"
		)
		copy "localization/%%f/info.txt" "build/%%f/KillKeys/info.txt"
		
		gcc -o "build/%%f/KillKeys/KillKeys.exe" killkeys.c build/resources.o -mwindows -lshlwapi -DL10N_FILE=\"localization/%%f/strings.h\"
		if exist "build/%%f/KillKeys/KillKeys.exe" (
			strip "build/%%f/KillKeys/KillKeys.exe"
			upx --best -qq "build/%%f/KillKeys/KillKeys.exe"
		)
						
		gcc -c -o "build/%%f/keyhook.o" keyhook.c
		if exist "build/%%f/KillKeys/keyhook.o" (
			gcc -shared -o "build/%%f/KillKeys/keyhook.dll" "build/%%f/keyhook.o"
			strip "build/%%f/KillKeys/keyhook.dll"
			upx --best -qq "build/%%f/KillKeys/keyhook.dll"
		)
	)
) else (
	gcc -o KillKeys.exe killkeys.c build/resources.o -mwindows -lshlwapi
	gcc -c -o "build/keyhook.o" keyhook.c
	gcc -shared -o "keyhook.dll" "build/keyhook.o"
	
	if "%1" == "run" (
		start KillKeys.exe
	)
)
