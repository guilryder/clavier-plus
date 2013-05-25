@echo off
cd /d %~dp0

echo Generating Clavier.zip...
mystart /wait Clavier.mus

echo Generating Clavier64.zip...
mystart /wait Clavier64.mus

echo Generating ClavierUnicode.zip...
mystart /wait ClavierUnicode.mus

echo Generating ClavierSetup.exe...
mystart /wait /verb Compile Clavier.iss

echo Generating ClavierSetup64.exe...
mystart /wait /verb Compile Clavier64.iss
