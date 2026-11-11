@echo off
setlocal enabledelayedexpansion

REM ============================================================
REM  build.bat - builds repo.sln with MSBuild
REM  Usage: build.bat [Release^|Debug] [x64^|Win32]
REM  Defaults: Release x64
REM ============================================================

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"

set "PLATFORM=%~2"
if "%PLATFORM%"=="" set "PLATFORM=x64"

echo [build] Configuration: %CONFIG%  Platform: %PLATFORM%

REM --- Locate MSBuild via vswhere (ships with VS 2017+) ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

set "MSBUILD="
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
        set "MSBUILD=%%i"
    )
)

REM --- Fallback: common MSBuild paths ---
if not defined MSBUILD (
    for %%v in (18 17 16 15) do (
        for %%p in ("%ProgramFiles%\Microsoft Visual Studio\%%v" "%ProgramFiles(x86)%\Microsoft Visual Studio\%%v") do (
            if exist "%%~p\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%%~p\MSBuild\Current\Bin\MSBuild.exe"
        )
    )
)

if not defined MSBUILD (
    echo [build] ERROR: MSBuild not found. Install Visual Studio with C++ build tools.
    exit /b 1
)

echo [build] MSBuild: "%MSBUILD%"

"%MSBUILD%" repo.sln -m -nologo -verbosity:minimal -p:Configuration=%CONFIG% -p:Platform=%PLATFORM%
if errorlevel 1 (
    echo [build] FAILED (errorlevel %errorlevel%^)
    exit /b 1
)

echo [build] OK - output in bin\%PLATFORM%\%CONFIG%\
endlocal
