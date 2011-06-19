@echo off
cd /d %~dp0

echo Generating ClavierBeta.zip...
del ClavierBeta.zip 2>nul
start /wait ClavierBeta.mus

for %%i in (ClavierBeta.zip) do move /y %%i E:\Web\utilfr\dn || echo Error: %%i
