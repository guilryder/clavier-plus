@echo off
cd /d %~dp0\..
call CopyCode
cd Distrib

echo Generating ClavierBetaSrc.zip...
start /wait ClavierBetaSrc.mus

echo Generating ClavierBeta.zip...
del ClavierBeta.zip 2>nul
start /wait ClavierBeta.mus

for %%i in (ClavierBeta.zip ClavierBetaSrc.zip) do move /y %%i E:\Web\utilfr\dn || echo Error: %%i

rd /s /q Sources
