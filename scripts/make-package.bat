@echo off

set config=%1
set zipcmd=%~dp0\tools\7z\7z.exe

pushd %~dp0

echo ============ Cleaning up ============
rmdir /S /Q appleseed-max2016 2>nul
rmdir /S /Q appleseed-max2017 2>nul
rmdir /S /Q appleseed-max2018 2>nul
rmdir /S /Q appleseed-max2019 2>nul
del appleseed-max2016-x.x.x-yyyy.zip 2>nul
del appleseed-max2017-x.x.x-yyyy.zip 2>nul
del appleseed-max2018-x.x.x-yyyy.zip 2>nul
del appleseed-max2019-x.x.x-yyyy.zip 2>nul
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

echo ============ Creating 3ds Max 2018 archive ============
mkdir appleseed-max2018
xcopy /S ..\sandbox\max2018\%config%\*.* appleseed-max2018
%zipcmd% a -r -mx=9 appleseed-max2018-x.x.x-yyyy.zip appleseed-max2018
rmdir /S /Q appleseed-max2018
echo.

echo ============ Creating 3ds Max 2019 archive ============
mkdir appleseed-max2019
xcopy /S ..\sandbox\max2019\%config%\*.* appleseed-max2019
%zipcmd% a -r -mx=9 appleseed-max2019-x.x.x-yyyy.zip appleseed-max2019
rmdir /S /Q appleseed-max2019
echo.

popd

pause
