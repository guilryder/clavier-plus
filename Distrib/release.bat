@echo off
cd /d %~dp0\..
call CopyCode
cd Distrib

echo Copying help files...
call release-help.bat

echo Generating Clavier.zip...
mystart /wait Clavier.mus

echo Generating Clavier64.zip...
mystart /wait Clavier64.mus

echo Generating ClavierUnicode.zip...
mystart /wait ClavierUnicode.mus

echo Generating ClavierSrc.zip...
mystart /wait ClavierSrc.mus

echo Generating ClavierSetup.exe...
mystart /wait /verb CompileWithIS Clavier.iss

echo Generating ClavierSetup64.exe...
mystart /wait /verb CompileWithIS Clavier64.iss

for %%i in (Clavier.zip Clavier64.zip ClavierUnicode.zip ClavierSrc.zip ClavierSetup.exe ClavierSetup64.exe) do move /y %%i E:\Web\utilfr\dn || echo Error: %%i

rd /s /q Sources

pause
