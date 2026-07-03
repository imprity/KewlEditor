@echo off

set "BUILD_DIR=build_win_x64"

cl /std:c17 ^
    /utf-8 ^
    /FeKewlEditor.exe ^
    .\src\*.c .\src\windows\*.c .\UTF8String\*.c ^
    /Ithirdparty_windows\SDL2-2.32.8\ ^
    /Ithirdparty_windows\SDL2_ttf-2.24.0\ ^
    /Ithirdparty_windows\SDL2-2.32.8\SDL2 ^
    /Ithirdparty_windows\SDL2_ttf-2.24.0\SDL2 ^
    /IUTF8String ^
    /link ^
    .\thirdparty_windows\SDL2-2.32.8\lib\x64\*.lib ^
    .\thirdparty_windows\SDL2_ttf-2.24.0\lib\x64\*.lib ^
    Gdi32.lib ^
    User32.lib ^
    Imm32.lib


if %errorlevel% neq 0 (
    exit /b 1
)

rmdir /S /Q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

copy /Y KewlEditor.exe "%BUILD_DIR%\KewlEditor.exe"

copy /Y thirdparty_windows\SDL2-2.32.8\lib\x64\SDL2.dll "%BUILD_DIR%\SDL2.dll"

copy /Y thirdparty_windows\SDL2_ttf-2.24.0\lib\x64\SDL2_ttf.dll "%BUILD_DIR%\SDL2_ttf.dll"
