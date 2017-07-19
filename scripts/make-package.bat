@echo off

set config=%1
set zipcmd=%~dp0\tools\7z\7z.exe

pushd %~dp0

echo ============ Cleaning up ============
rmdir /S /Q appleseed 2>nul
del appleseed-max2015-x.x.x-yyyy.zip 2>nul
del appleseed-max2016-x.x.x-yyyy.zip 2>nul
del appleseed-max2017-x.x.x-yyyy.zip 2>nul
echo.

echo ============ Creating 3ds Max 2015 archive ============
mkdir appleseed
xcopy /S ..\sandbox\max2015\%config%\*.* appleseed
%zipcmd% a -r -mx=9 appleseed-max2015-x.x.x-yyyy.zip appleseed
rmdir /S /Q appleseed
echo.

echo ============ Creating 3ds Max 2016 archive ============
mkdir appleseed
xcopy /S ..\sandbox\max2016\%config%\*.* appleseed
%zipcmd% a -r -mx=9 appleseed-max2016-x.x.x-yyyy.zip appleseed
rmdir /S /Q appleseed
echo.

echo ============ Creating 3ds Max 2017 archive ============
mkdir appleseed
xcopy /S ..\sandbox\max2017\%config%\*.* appleseed
%zipcmd% a -r -mx=9 appleseed-max2017-x.x.x-yyyy.zip appleseed
rmdir /S /Q appleseed
echo.

popd

pause
