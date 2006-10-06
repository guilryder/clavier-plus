@echo off
setlocal
set THE_SRCDIR=%SRCDIR%\%1
set THE_DESTDIR=%DESTDIR%\%1

md %DESTDIR% 1>nul 2>nul

echo.
echo Copying from %THE_SRCDIR% to %THE_DESTDIR%
:loop
if %2!==! goto end
echo    %2
copy %THE_SRCDIR%\%2 %THE_DESTDIR% >nul
shift
goto loop

:end
