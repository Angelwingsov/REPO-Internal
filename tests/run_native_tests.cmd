@echo off
setlocal
cd /d "%~dp0.."
set "stabilityVs="
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "stabilityVs=%%i"
if not defined stabilityVs exit /b 1
call "%stabilityVs%\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist build mkdir build
cl /nologo /utf-8 /std:c++17 /EHsc /Gy /I . /I src tests\stability_test.cpp src\core\config.cpp src\memory\memory.cpp src\mono\mono_bridge.cpp src\unity\unity_engine.cpp /Fe:build\stability_test.exe /Fo:build\ /link /OPT:REF user32.lib
if errorlevel 1 exit /b 1
build\stability_test.exe
if errorlevel 1 exit /b 1
cl /nologo /utf-8 /std:c++17 /EHsc /I . /I src tests\config_esp_test.cpp src\core\config.cpp src\core\config_store.cpp /Fe:build\config_esp_test.exe /Fo:build\ /link user32.lib
if errorlevel 1 exit /b 1
build\config_esp_test.exe
if errorlevel 1 exit /b 1
cl /nologo /utf-8 /std:c++17 /EHsc /I . /I src tests\removals_camera_test.cpp src\core\config.cpp /Fe:build\removals_camera_test.exe /Fo:build\ /link user32.lib
if errorlevel 1 exit /b 1
build\removals_camera_test.exe
if errorlevel 1 exit /b 1
cl /nologo /utf-8 /std:c++17 /EHsc /Gy /I . /I src tests\animation_follow_test.cpp src\core\config.cpp /Fe:build\animation_follow_test.exe /Fo:build\ /link /OPT:REF user32.lib
if errorlevel 1 exit /b 1
build\animation_follow_test.exe
