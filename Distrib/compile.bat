@echo off
cd /d %~dp0

echo Generating Clavier32.zip...
mystart /wait Clavier32.mus

echo Generating Clavier64.zip...
mystart /wait Clavier64.mus

echo Generating ClavierSetup32.exe...
mystart /wait /verb Compile Clavier.iss

echo Generating ClavierSetup64.exe...
mystart /wait /verb Compile Clavier64.iss
