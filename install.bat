@echo off

set "INSTALL_DIR=%USERPROFILE%\HelpMake"
if defined HELPMAKE_DIR set "INSTALL_DIR=%HELPMAKE_DIR%"
set "BIN_DIR=%USERPROFILE%\.local\bin"
set "REPO=https://github.com/Gag91/Help_Make"

echo [HelpMake] Installing to "$INSTALL_DIR$"

where git 
if errorlevel 1 (
    echo [HelpMake] Error: git not found in PATH.
    echo   Download: https://git-scm.com/download/win
    exit  1
)

if exist "%INSTALL_DIR%" (
    echo [HelpMake] "%INSTALL_DIR%" exists, updating...
    git -C "%INSTALL_DIR%" pull --ff-only
    if errorlevel 1 (
        echo [HelpMake] Warning: git pull failed. Continuing with existing source.
    )
) else (
    git clone --depth=1 "%REPO%" "%INSTALL_DIR%"
    if errorlevel 1 (
        echo [HelpMake] Error: clone failed.
        exit /b 1
    )
)

where g++ >nul 2>nul
if errorlevel 1 (
    echo [HelpMake] Error: g++ not found in PATH.
    echo.
    echo   Install one of:
    echo     MSYS2:  https://www.msys2.org/
    echo             then: pacman -S mingw-w64-ucrt-x86_64-toolchain
    echo     MinGW:  https://winlibs.com/
    echo     LLVM:   winget install LLVM.LLVM  ^(provides clang++^)
    echo.
    echo   After installing, add the compiler bin directory to PATH
    echo   and re-run this script.
    exit /b 1
)

pushd "%INSTALL_DIR%"

echo [HelpMake] Building...

set "SRC="
for %%f in (src\cli\*.cpp src\executor\*.cpp src\fs\*.cpp src\generator\*.cpp src\getters\*.cpp src\init\*.cpp src\parser\*.cpp src\process\*.cpp src\toolchain\*.cpp src\main.cpp) do (
    set "SRC=!SRC! %%f"
)

g++ -std=c++20 !SRC! -o hm.exe -Iinclude
if errorlevel 1 (
    echo [HelpMake] Build failed.
    popd
    exit /b 1
)


if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
copy /Y "%INSTALL_DIR%\hm.exe" "%BIN_DIR%\hm.exe" >nul
if errorlevel 1 (
    echo [HelpMake] Error: could not copy hm.exe to "%BIN_DIR%".
    exit /b 1
)

echo %PATH% | find /I "%BIN_DIR%" >nul
if errorlevel 1 (
    for /f "skip=2 tokens=3*" %%a in ('reg query HKCU\Environment /v PATH 2^>nul') do set "USER_PATH=%%a %%b"
    if defined USER_PATH (
        setx PATH "!USER_PATH!;%BIN_DIR%" >nul
    ) else (
        setx PATH "%BIN_DIR%" >nul
    )
    echo [HelpMake] Added "%BIN_DIR%" to user PATH.
    echo [HelpMake] Restart your terminal for the change to take effect.
) else (
    echo [HelpMake] "%BIN_DIR%" already in PATH.
)

echo.
echo [HelpMake] Installed: %BIN_DIR%\hm.exe
echo [HelpMake] Run: hm --version
endlocal