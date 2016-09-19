@echo off

pushd %~dp0

REM Cleanup
rmdir /S /Q appleseed 2>nul
del appleseed-max2015-x.x.x-yyyy.zip 2>nul
del appleseed-max2017-x.x.x-yyyy.zip 2>nul

REM 3ds Max 2015 archive
mkdir appleseed
copy ..\bin\appleseed-max2015\x64\Ship\*.* appleseed
tools\7z\7z.exe a -r -mx9 appleseed-max2015-x.x.x-yyyy.zip appleseed
rmdir /S /Q appleseed

REM 3ds Max 2017 archive
mkdir appleseed
copy ..\bin\appleseed-max2017\x64\Ship\*.* appleseed
tools\7z\7z.exe a -r -mx9 appleseed-max2017-x.x.x-yyyy.zip appleseed
rmdir /S /Q appleseed

popd

pause
