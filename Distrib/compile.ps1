Push-Location (Split-Path (Resolve-Path $MyInvocation.MyCommand.Path))

echo "Generating Clavier.zip..."
pause
rm -ErrorAction Ignore Clavier.zip
7z a -bsp0 -bso0 -tzip -mx9 -sse Clavier.zip Clavier.ini ..\output\x64_Release\Clavier.exe

echo "Generating ClavierSetup.exe..."
pause
start -Verb compile Clavier.nsi
