@echo off
cd /d %~dp0
for %%i in (*.html) do copy /y %%i E:\Web\utilfr\util\Clavier%%i >nul
