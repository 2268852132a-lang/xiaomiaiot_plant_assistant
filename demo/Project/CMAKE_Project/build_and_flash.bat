@echo off
REM ================================================================
REM build_and_flash.bat  -  Build and flash SMART_FAN
REM Adapter: CMSIS-DAP  |  Target: GD32F470ZK (stm32f4x driver)
REM Requires cmake, ninja, arm-none-eabi-gcc, openocd in system PATH.
REM ================================================================
setlocal
pushd "%~dp0"

echo [BUILD] Configuring CMake...
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed!
    pause & exit /b 1
)

echo [BUILD] Compiling...
cmake --build build
if %ERRORLEVEL% NEQ 0 (
    
    echo [ERROR] Build failed!
    pause & exit /b 1
)

set HEX_FILE=%~dp0build\PLANT_ASSISTANT.hex
set HEX_FILE=%HEX_FILE:\=/%

echo [FLASH] Flashing %HEX_FILE% ...
openocd -f "%~dp0gd32f470zk.cfg" -c "program %HEX_FILE% verify reset exit"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Flash failed! Is your debugger connected?
    pause & exit /b 1
)

echo [DONE] Build and flash succeeded!
popd
pause
