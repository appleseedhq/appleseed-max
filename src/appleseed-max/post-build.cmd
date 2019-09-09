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

copy /Y "%targetPath%.dlr" "%outdirA%\"
copy /Y "%targetPath%.dlr" "%outdirB%\"

if not "%configuration%"=="Ship" (
  copy /Y "%targetPath%.pdb" "%outdirA%\"
  copy /Y "%targetPath%.pdb" "%outdirB%\"
)