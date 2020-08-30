Push-Location (Split-Path (Resolve-Path $MyInvocation.MyCommand.Path))

robocopy . E:\Web\gryder\software\clavier-plus\ *.html /XO
