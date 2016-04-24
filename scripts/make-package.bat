@echo off

copy /Y "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\redist\x64\Microsoft.VC110.CRT\*.*" ..\bin\x64\Ship\
tools\7z\7z.exe a -r -mx9 appleseed-max2015-x.x.x-yyyy.zip ..\bin\x64\Ship\*.*

pause
