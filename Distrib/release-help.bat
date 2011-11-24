@echo off
cd /d %~dp0
xcopy /Y /D ClavierHelp.css E:\Web\utilfr\util
for %%i in (*.html) do xcopy /Y /D %%i E:\Web\utilfr\util\Clavier%%i
