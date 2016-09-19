@echo off
tools\7z\7z.exe a -r -mx9 appleseed-max2015-x.x.x-yyyy-debug.zip ..\bin\appleseed-max2015\x64\Debug\*.*
tools\7z\7z.exe a -r -mx9 appleseed-max2017-x.x.x-yyyy-debug.zip ..\bin\appleseed-max2017\x64\Debug\*.*
pause
