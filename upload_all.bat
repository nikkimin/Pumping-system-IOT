@echo off
echo ========================================
echo   UPLOAD ESP32 va ARDUINO UNO
echo ========================================
echo.

echo [1/4] Uploading ESP32 SPIFFS...
cd /d D:\Pumping-system-IOT\Esp
call pio run --target uploadfs
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: ESP32 SPIFFS upload failed!
    pause
    exit /b 1
)

echo.
echo [2/4] Uploading ESP32 Code...
call pio run --target upload
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: ESP32 code upload failed!
    pause
    exit /b 1
)

echo.
echo [3/4] Uploading Arduino UNO...
cd /d D:\Pumping-system-IOT\Arduino
call pio run --target upload
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Arduino UNO upload failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   THANH CONG! Upload hoan tat
echo ========================================
echo.
pause
