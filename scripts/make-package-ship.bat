@echo off

pushd %~dp0

echo ============ Cleaning up ============
rmdir /S /Q appleseed 2>nul
del appleseed-max2015-x.x.x-yyyy.zip 2>nul
del appleseed-max2017-x.x.x-yyyy.zip 2>nul
echo.

echo ============ Creating 3ds Max 2015 archive ============
mkdir appleseed
copy ..\bin\appleseed-max2015\x64\Ship\*.* appleseed
tools\7z\7z.exe a -r -mx=9 appleseed-max2015-x.x.x-yyyy.zip appleseed
rmdir /S /Q appleseed
echo.

echo ============ Creating 3ds Max 2017 archive ============
mkdir appleseed
copy ..\bin\appleseed-max2017\x64\Ship\*.* appleseed
tools\7z\7z.exe a -r -mx=9 appleseed-max2017-x.x.x-yyyy.zip appleseed
rmdir /S /Q appleseed
echo.

popd

pause
