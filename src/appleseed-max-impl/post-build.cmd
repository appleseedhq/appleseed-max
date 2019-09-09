@echo off

set solution=%~1
set configuration=%~2
set targetdir=%~3
set targetname=%~4
set toolset=%~5
set maxversion=%~6

set outdirBase=%solution%..\sandbox\max%maxversion%
set outdirA=%outdirBase%\%configuration%
set outdirB=%outdirBase%\Current
set targetPath=%targetdir%%targetname%

mkdir "%outdirA%" 2>nul
mkdir "%outdirB%" 2>nul

copy /Y "%targetPath%.dll" "%outdirA%\"
copy /Y "%targetPath%.dll" "%outdirB%\"

if not "%configuration%"=="Ship" (
  copy /Y "%targetPath%.pdb" "%outdirA%\"
  copy /Y "%targetPath%.pdb" "%outdirB%\"
)

set targetPath=%solution%\..\..\appleseed\sandbox\bin\vc%toolset%\%configuration%\appleseed
copy /Y "%targetPath%.dll" "%outdirA%\"
copy /Y "%targetPath%.dll" "%outdirB%\"

if not "%configuration%"=="Ship" (
  copy /Y "%targetPath%.pdb" "%outdirA%\"
  copy /Y "%targetPath%.pdb" "%outdirB%\"
)

rmdir /S /Q "%outdirA%\shaders" 2>nul
rmdir /S /Q "%outdirB%\shaders" 2>nul

mkdir "%outdirA%\shaders\max\" 2>nul
mkdir "%outdirA%\shaders\appleseed\" 2>nul
mkdir "%outdirB%\shaders\max\" 2>nul
mkdir "%outdirB%\shaders\appleseed\" 2>nul

del /Q "%outdirA%\shaders\max\*.oso" 2>nul
del /Q "%outdirA%\shaders\appleseed\*.oso" 2>nul
del /Q "%outdirB%\shaders\max\*.oso" 2>nul
del /Q "%outdirB%\shaders\appleseed\*.oso" 2>nul

set shadersdir=%solution%..\..\appleseed\sandbox\shaders
copy /Y "%shadersdir%\max\*.oso" "%outdirA%\shaders\max\"
copy /Y "%shadersdir%\appleseed\*.oso" "%outdirA%\shaders\appleseed\"
copy /Y "%shadersdir%\max\*.oso" "%outdirB%\shaders\max\"
copy /Y "%shadersdir%\appleseed\*.oso" "%outdirB%\shaders\appleseed\"
