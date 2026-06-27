@echo off
setlocal
echo ================================================
echo   RP2040 PS3 Controller Fixer - Build Script
echo ================================================
echo.
:: ---- Pico SDK Environment Setup ----
for /f "delims=" %%i in ('dir /b /ad "%USERPROFILE%\.pico-sdk\sdk\"')       do set PICO_SDK_PATH=%USERPROFILE%\.pico-sdk\sdk\%%i
for /f "delims=" %%i in ('dir /b /ad "%USERPROFILE%\.pico-sdk\toolchain\"') do set PICO_TOOLCHAIN_PATH=%USERPROFILE%\.pico-sdk\toolchain\%%i
for /f "delims=" %%i in ('dir /b /ad "%USERPROFILE%\.pico-sdk\picotool\"')  do set PICOTOOL_PATH=%USERPROFILE%\.pico-sdk\picotool\%%i\picotool
for /f "delims=" %%i in ('dir /b /ad "%USERPROFILE%\.pico-sdk\cmake\"')     do set CMAKE_PATH=%USERPROFILE%\.pico-sdk\cmake\%%i\bin
for /f "delims=" %%i in ('dir /b /ad "%USERPROFILE%\.pico-sdk\ninja\"')     do set NINJA_PATH=%USERPROFILE%\.pico-sdk\ninja\%%i

set PATH=%PICO_TOOLCHAIN_PATH%\bin;%PICOTOOL_PATH%;%CMAKE_PATH%;%NINJA_PATH%;%PATH%

:: Create output folder for final UF2 files
if not exist output mkdir output
set "CFW_LOG=output\build_cfw.log"
set "HEN_LOG=output\build_hen.log"

:: ============================================================
:: Build 1: CFW version - No combo, plain DS3 emulation
:: ============================================================
echo [1/2] Building RP2040_PS3_CFW_Controller_Fixer (no combo)...
echo -------------------------------------------------------
if exist build_cfw rmdir /s /q build_cfw
cmake -G Ninja -S . -B build_cfw -DENABLE_COMBO=OFF > "%CFW_LOG%" 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed for CFW build!
    type "%CFW_LOG%"
    exit /b 1
)
findstr /i /c:"warning:" /c:"CMake Warning" "%CFW_LOG%" >nul
if not errorlevel 1 (
    echo.
    echo WARNING: CMake configuration warnings for CFW build:
    type "%CFW_LOG%"
    exit /b 1
)
ninja -C build_cfw >> "%CFW_LOG%" 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: Ninja build failed for CFW version!
    type "%CFW_LOG%"
    exit /b 1
)
findstr /i /c:"warning:" /c:"CMake Warning" "%CFW_LOG%" >nul
if not errorlevel 1 (
    echo.
    echo WARNING: Ninja build warnings for CFW version:
    type "%CFW_LOG%"
    exit /b 1
)
copy /y "build_cfw\RP2040_PS3_Controller_Fixer.uf2" "output\RP2040_PS3_CFW_Controller_Fixer.uf2" >nul
echo Done: output\RP2040_PS3_CFW_Controller_Fixer.uf2
echo.

:: ============================================================
:: Build 2: PS3HEN version - With startup combo (Left, Left, X)
:: ============================================================
echo [2/2] Building RP2040_PS3_PS3HEN_Controller_Fixer (with combo)...
echo -------------------------------------------------------
if exist build_hen rmdir /s /q build_hen
cmake -G Ninja -S . -B build_hen -DENABLE_COMBO=ON > "%HEN_LOG%" 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed for PS3HEN build!
    type "%HEN_LOG%"
    exit /b 1
)
findstr /i /c:"warning:" /c:"CMake Warning" "%HEN_LOG%" >nul
if not errorlevel 1 (
    echo.
    echo WARNING: CMake configuration warnings for PS3HEN build:
    type "%HEN_LOG%"
    exit /b 1
)
ninja -C build_hen >> "%HEN_LOG%" 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: Ninja build failed for PS3HEN version!
    type "%HEN_LOG%"
    exit /b 1
)
findstr /i /c:"warning:" /c:"CMake Warning" "%HEN_LOG%" >nul
if not errorlevel 1 (
    echo.
    echo WARNING: Ninja build warnings for PS3HEN version:
    type "%HEN_LOG%"
    exit /b 1
)
copy /y "build_hen\RP2040_PS3_Controller_Fixer.uf2" "output\RP2040_PS3_PS3HEN_Controller_Fixer.uf2" >nul
echo Done: output\RP2040_PS3_PS3HEN_Controller_Fixer.uf2
echo.

:: ============================================================
:: Cleanup intermediate build folders
:: ============================================================
echo Cleaning up build folders...
if exist build_cfw rmdir /s /q build_cfw
if exist build_hen rmdir /s /q build_hen
del /q "%CFW_LOG%" "%HEN_LOG%" >nul 2>&1
echo Done.
echo.

echo ================================================
echo   Build complete!  UF2 files are in: output\
echo.
echo   RP2040_PS3_CFW_Controller_Fixer.uf2
echo     ^> Plain DS3 emulation, no button combo.
echo.
echo   RP2040_PS3_PS3HEN_Controller_Fixer.uf2
echo     ^> DS3 emulation + Left, Left, X combo
echo     ^> triggered 35 seconds after boot.
echo ================================================
