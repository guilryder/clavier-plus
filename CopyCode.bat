@echo off
set SRCDIR=.
set DESTDIR=Distrib\Sources
setlocal

rd /s /q %DESTDIR% 2>nul
md %DESTDIR%
md %DESTDIR%\Distrib

call CopyFiles . *.cpp *.h *.rc *.sln *.vcproj *.ico *.bmp *.cur license.txt
call CopyFiles Distrib *.ini *.iss *.htm
