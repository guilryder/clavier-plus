Push-Location (Split-Path (Resolve-Path $MyInvocation.MyCommand.Path))

robocopy . E:\Web\utilfr\util *.html /XO
