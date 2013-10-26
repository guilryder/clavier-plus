@echo off
cd /d %~dp0

for %%i in (Clavier32.zip Clavier64.zip ClavierSetup32.exe ClavierSetup64.exe) do move /y %%i E:\Web\utilfr\dn || echo Error: %%i

pause
