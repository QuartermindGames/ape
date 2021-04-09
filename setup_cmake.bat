@echo off
cls

set RUNTIME_DIR=.\release\runtime\
set RUNTIME_X64_DIR=%RUNTIME_DIR%win32-x64\
set RUNTIME_X86_DIR=%RUNTIME_DIR%win32-x86\
set SOURCE_DIR=.\src\
set THIRDPARTY_DIR=%SOURCE_DIR%3rdparty\

rem Ensure SDL2 is setup, if not download it and awkwardly put it together
set SDL2_VERSION=-2.0.14
set SDL2_DIR=%THIRDPARTY_DIR%sdl2\
set SDL2_DOWNLOAD=http://www.libsdl.org/release/SDL2-devel%SDL_VERSION%-mingw.tar.gz
if not exist %SDL2_DIR%include\SDL2\SDL.h (
	rem Download and extract SDL2 data
	echo SDL2 dir not found under '%SDL2_DIR%', downloading '%SDL2_DOWNLOAD%'...
	bitsadmin /transfer yin_setup_job /download %SDL2_DOWNLOAD% %THIRDPARTY_DIR%SDL2-devel%SDL_VERSION%-mingw.tar.gz
	
	if not exist %THIRDPARTY_DIR%SDL2-devel%SDL_VERSION%-mingw.tar.gz (
		echo Failed to download SDL2!
		pause
		exit
	)
	
	echo Extracting data...
	powershell -Command "tar -x -v -C .\3rdparty\ -f %THIRDPARTY_DIR%SDL2-devel%SDL_VERSION%-mingw.tar.gz"
	echo Deleting downloaded package...
	del %THIRDPARTY_DIR%SDL2-devel%SDL_VERSION%-mingw.tar.gz
	echo Done!
	
	rem SDL2 has been downloaded, now we have to shuffle everything
	echo Creating sdl2 directory and copying data...
	mkdir %SDL2_DIR%
	mkdir %SDL2_DIR%include\
	mkdir %SDL2_DIR%lib\
	mkdir %SDL2_DIR%lib\x64\
	mkdir %SDL2_DIR%lib\x86\
	Xcopy /E /I %THIRDPARTY_DIR%SDL2%SDL_VERSION%\x86_64-w64-mingw32\include %SDL2_DIR%include
	Xcopy /E /I %THIRDPARTY_DIR%SDL2%SDL_VERSION%\x86_64-w64-mingw32\lib %SDL2_DIR%lib\x64
	Xcopy /E /I %THIRDPARTY_DIR%SDL2%SDL_VERSION%\x86_64-w64-mingw32\bin %RUNTIME_X64_DIR%
	Xcopy /E /I %THIRDPARTY_DIR%SDL2%SDL_VERSION%\i686-w64-mingw32\lib %SDL2_DIR%lib\x86
	Xcopy /E /I %THIRDPARTY_DIR%SDL2%SDL_VERSION%\i686-w64-mingw32\bin %RUNTIME_X86_DIR%
	echo Done!
	
	rem Now delete the SDL2-2.0.14 dir
	del /F /Q /S %THIRDPARTY_DIR%SDL2-%SDL_VERSION%\*.*
	rmdir /S /Q %THIRDPARTY_DIR%SDL2-%SDL_VERSION%\
)
echo SDL2 dir found, proceeding to setup build directory...

if not exist .\build\ mkdir .\build\
cd .\build\
cmake .\..\

pause
