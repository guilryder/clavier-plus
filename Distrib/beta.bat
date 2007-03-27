@echo off
cd /d E:\Docs\Programmes\Clavier\Distrib

..\release\Clavier.exe /quit
..\release\Clavier.exe /quit
..\release\Clavier.exe /quit
copy /y ..\release\Clavier.exe C:\Soft\Tools
start C:\Soft\Tools\Clavier.exe

del ClavierBeta.zip 2>nul
start /wait ClavierBeta.mus
