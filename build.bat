@echo off

setlocal EnableDelayedExpansion

set ARCH=x64
set PRINT_HELP=0

rem argument parsing
for %%x in (%*) do (
    rem parse help flag
    if "%%~x"=="--help" (
        set PRINT_HELP=1
    )
    if "%%~x"=="help" (
        set PRINT_HELP=1
    )
    if "%%~x"=="-h" (
        set PRINT_HELP=1
    )
    if "%%~x"=="/?" (
        set PRINT_HELP=1
    )

    rem parse arch flag
    if "%%~x"=="--x86" (
        set ARCH=x86
    )
    if "%%~x"=="--x64" (
        set ARCH=x64
    )
)

if %PRINT_HELP%==1 (
    echo --help : prints this message
    echo --x86 : build for x86
    echo --x64 : build for x64
    exit /b 1
)

rem set build dir
if %ARCH% equ x64 (
    set "BUILD_DIR=build_win_x64"
) else (
    set "BUILD_DIR=build_win_x86"
)

rem create folder for compiler to put obj files in
rmdir /S /Q obj_files
mkdir obj_files

rem build build flags
set "FLAGS=/std:c17"
set "FLAGS=%FLAGS% /utf-8"
set "FLAGS=%FLAGS% /W4"
set "FLAGS=%FLAGS% /experimental:external"
set "FLAGS=%FLAGS% /external:W0"
set "FLAGS=%FLAGS% /FeKewlEditor.exe"
set "FLAGS=%FLAGS% /Foobj_files\"
set "FLAGS=%FLAGS% .\src\*.c .\src\windows\*.c .\UTF8String\*.c"
set "FLAGS=%FLAGS% /IUTF8String"
set "FLAGS=%FLAGS% /external:Ithirdparty_windows\SDL2-2.32.8\"
set "FLAGS=%FLAGS% /external:Ithirdparty_windows\SDL2_ttf-2.24.0\"
set "FLAGS=%FLAGS% /external:Ithirdparty_windows\SDL2-2.32.8\SDL2"
set "FLAGS=%FLAGS% /external:Ithirdparty_windows\SDL2_ttf-2.24.0\SDL2"
set "FLAGS=%FLAGS% /link"

rem add lib according to architecture
if %ARCH%==x64 (
    set "FLAGS=!FLAGS! .\thirdparty_windows\SDL2-2.32.8\lib\x64\*.lib"
    set "FLAGS=!FLAGS! .\thirdparty_windows\SDL2_ttf-2.24.0\lib\x64\*.lib"
) else (
    set "FLAGS=!FLAGS! .\thirdparty_windows\SDL2-2.32.8\lib\x86\*.lib"
    set "FLAGS=!FLAGS! .\thirdparty_windows\SDL2_ttf-2.24.0\lib\x86\*.lib"
)

set "FLAGS=%FLAGS% Gdi32.lib"
set "FLAGS=%FLAGS% User32.lib"
set "FLAGS=%FLAGS% Imm32.lib"

echo %FLAGS%

cl %FLAGS%

if %errorlevel% neq 0 (
    exit /b 1
)

rem move things to build directory
rmdir /S /Q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

copy /Y KewlEditor.exe "%BUILD_DIR%\KewlEditor.exe"
copy /Y NotoSansKR-Medium.otf "%BUILD_DIR%\NotoSansKR-Medium.otf"

if %ARCH%==x64 (
    copy /Y thirdparty_windows\SDL2-2.32.8\lib\x64\SDL2.dll "%BUILD_DIR%\SDL2.dll"
    copy /Y thirdparty_windows\SDL2_ttf-2.24.0\lib\x64\SDL2_ttf.dll "%BUILD_DIR%\SDL2_ttf.dll"
) else (
    copy /Y thirdparty_windows\SDL2-2.32.8\lib\x86\SDL2.dll "%BUILD_DIR%\SDL2.dll"
    copy /Y thirdparty_windows\SDL2_ttf-2.24.0\lib\x86\SDL2_ttf.dll "%BUILD_DIR%\SDL2_ttf.dll"
)

endlocal
