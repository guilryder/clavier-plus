@echo off
cd /d E:\Docs\Programmes\Clavier
call CopyCode
cd Distrib

echo Generating Clavier.zip...
mystart /wait Clavier.mus

echo Generating ClavierSrc.zip...
mystart /wait ClavierSrc.mus

echo Generating ClavierSetup.exe...
mystart /wait /verb CompileWithIS Clavier.iss

for %%i in (Clavier.zip ClavierSrc.zip ClavierSetup.exe) do move /y %%i E:\Web\utilfr\dn
pause