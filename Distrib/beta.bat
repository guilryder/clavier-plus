@echo off
cd /d E:\Docs\Programmes\Clavier
call CopyCode
cd Distrib

echo Generating ClavierBetaSrc.zip...
start /wait ClavierBetaSrc.mus

echo Generating ClavierBeta.zip...
del ClavierBeta.zip 2>nul
start /wait ClavierBeta.mus

for %%i in (ClavierBeta.zip ClavierBetaSrc.zip) do move /y %%i E:\Web\utilfr\dn || echo Error: %%i

for %%i in (1 2 3) do C:\Soft\Tools\Clavier.exe /quit
copy /y ..\release\Clavier.exe C:\Soft\Tools
start C:\Soft\Tools\Clavier.exe
