@echo off

set config=%1
set zipcmd=%~dp0\tools\7z\7z.exe

pushd %~dp0

echo ============ Cleaning up ============
rmdir /S /Q appleseed-max2015 2>nul
rmdir /S /Q appleseed-max2016 2>nul
rmdir /S /Q appleseed-max2017 2>nul
del appleseed-max2015-x.x.x-yyyy.zip 2>nul
del appleseed-max2016-x.x.x-yyyy.zip 2>nul
del appleseed-max2017-x.x.x-yyyy.zip 2>nul
echo.

echo ============ Creating 3ds Max 2015 archive ============
mkdir appleseed-max2015
xcopy /S ..\sandbox\max2015\%config%\*.* appleseed-max2015
%zipcmd% a -r -mx=9 appleseed-max2015-x.x.x-yyyy.zip appleseed-max2015
rmdir /S /Q appleseed-max2015
echo.

echo ============ Creating 3ds Max 2016 archive ============
mkdir appleseed-max2016
xcopy /S ..\sandbox\max2016\%config%\*.* appleseed-max2016
%zipcmd% a -r -mx=9 appleseed-max2016-x.x.x-yyyy.zip appleseed-max2016
rmdir /S /Q appleseed-max2016
echo.

echo ============ Creating 3ds Max 2017 archive ============
mkdir appleseed-max2017
xcopy /S ..\sandbox\max2017\%config%\*.* appleseed-max2017
%zipcmd% a -r -mx=9 appleseed-max2017-x.x.x-yyyy.zip appleseed-max2017
rmdir /S /Q appleseed-max2017
echo.

popd

pause
