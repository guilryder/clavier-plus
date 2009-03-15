@echo off
cd /d %~dp0\..
call CopyCode
cd Distrib

echo Copying help files...
call release-help.bat

echo Generating Clavier.zip...
mystart /wait Clavier.mus

echo Generating ClavierSrc.zip...
mystart /wait ClavierSrc.mus

echo Generating ClavierSetup.exe...
mystart /wait /verb CompileWithIS Clavier.iss

for %%i in (Clavier.zip ClavierSrc.zip ClavierSetup.exe) do move /y %%i E:\Web\utilfr\dn || echo Error: %%i

for %%i in (1 2 3) do C:\Soft\Tools\Clavier.exe /quit
copy /y ..\release\Clavier.exe C:\Soft\Tools\Clavier.exe >nul
start C:\Soft\Tools\Clavier.exe

rd /s /q Sources

pause
