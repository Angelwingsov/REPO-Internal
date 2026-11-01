@echo off
setlocal
cd /d "%~dp0.."
set "guiVs="
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "guiVs=%%i"
if not defined guiVs exit /b 1
call "%guiVs%\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist build\ui-preview mkdir build\ui-preview
rc /nologo /i src\ui\assets /fo build\ui-preview\gui.res src\ui\assets\gui.rc
if errorlevel 1 exit /b 1
cl /nologo /utf-8 /std:c++17 /EHsc /Od /Gy /D ORIGINAL_GUI_TEST_INPUT /I . /I src tests\ui_preview.cpp src\features\esp.cpp src\ui\menu.cpp src\ui\theme.cpp src\ui\original\framework.cpp src\ui\original\renderer.cpp src\core\config.cpp src\core\config_store.cpp imgui\imgui.cpp imgui\imgui_draw.cpp imgui\imgui_widgets.cpp imgui\imgui_tables.cpp imgui\imgui_impl_dx11.cpp /Fe:build\ui-preview\ui_preview.exe /Fo:build\ui-preview\ /link /OPT:REF build\ui-preview\gui.res d3d11.lib dxgi.lib d3dcompiler.lib user32.lib
if errorlevel 1 exit /b 1
build\ui-preview\ui_preview.exe
if errorlevel 1 exit /b 1
python tests\gui_source_contract.py
