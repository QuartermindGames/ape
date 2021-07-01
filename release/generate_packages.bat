@echo off
for %%I in (develop\*.pms) do "%~dp0runtime\win32-x86_64\pkgman.exe" %%I
pause
