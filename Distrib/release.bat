@echo off
cd /d %~dp0

echo Copying help files...
call release-help.bat

echo Copying binary files...
for %%i in (Clavier.zip Clavier64.zip ClavierUnicode.zip ClavierSetup.exe ClavierSetup64.exe) do move /y %%i E:\Web\utilfr\dn || echo Error: %%i

pause
