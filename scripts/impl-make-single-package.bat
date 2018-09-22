@echo off

REM
REM This source file is part of appleseed.
REM Visit https://appleseedhq.net/ for additional information and resources.
REM
REM This software is released under the MIT license.
REM
REM Copyright (c) 2016-2018 Francois Beaune, The appleseedhq Organization
REM
REM Permission is hereby granted, free of charge, to any person obtaining a copy
REM of this software and associated documentation files (the "Software"), to deal
REM in the Software without restriction, including without limitation the rights
REM to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
REM copies of the Software, and to permit persons to whom the Software is
REM furnished to do so, subject to the following conditions:
REM
REM The above copyright notice and this permission notice shall be included in
REM all copies or substantial portions of the Software.
REM
REM THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
REM IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
REM FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
REM AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
REM LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
REM OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
REM THE SOFTWARE.
REM

set config=%1
set maxversion=%2

set zipcmd=%~dp0\tools\7z\7z.exe

echo ============ Creating 3ds Max %maxversion% Archive ============
echo.

rmdir /S /Q appleseed-max%maxversion% 2>nul
del appleseed-max%maxversion%-x.x.x-yyyy.zip 2>nul

if exist ..\sandbox\max%maxversion%\%config%\ (
    mkdir appleseed-max%maxversion%
    xcopy /S ..\sandbox\max%maxversion%\%config%\*.* appleseed-max%maxversion%
    %zipcmd% a -r -mx=9 appleseed-max%maxversion%-x.x.x-yyyy.zip appleseed-max%maxversion%
    rmdir /S /Q appleseed-max%maxversion%
) else (
    echo Warning: No 3ds Max %maxversion% build in config %config% found.
)

echo.
