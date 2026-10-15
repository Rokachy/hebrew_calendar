@echo off
rem Write user_data.csv (Wi-Fi, family dates) to the "userdata" NVS partition.
rem Run after every change to user_data.csv. The app is not touched; the device
rem reads the new data at its next boot (this script resets it).
rem
rem Usage: flash_user_data.bat [PORT]     (default COM9)
rem
rem Needs the firmware with partitions.csv on the board once (idf.py flash).
rem Offset and size must match the userdata line in partitions.csv.

setlocal
set PORT=%1
if "%PORT%"=="" set PORT=COM9

set PYTHON=C:\Espressif\tools\python_env\idf6.1_py3.11_env\Scripts\python.exe
set OFFSET=0x187000
set SIZE=0x4000
set CSV=%~dp0user_data.csv
rem The image holds the password in plain text: temp folder, deleted afterwards
set BIN=%TEMP%\hebcal_userdata.bin

if not exist "%CSV%" (
    echo user_data.csv not found. Copy user_data.example.csv to user_data.csv and fill it in.
    exit /b 1
)

"%PYTHON%" -m esp_idf_nvs_partition_gen generate "%CSV%" "%BIN%" %SIZE%
if errorlevel 1 goto failed

"%PYTHON%" -m esptool --chip esp32s3 -p %PORT% write-flash %OFFSET% "%BIN%"
if errorlevel 1 goto failed

del "%BIN%"
echo.
echo User data written to %PORT%.
exit /b 0

:failed
if exist "%BIN%" del "%BIN%"
echo.
echo FAILED - nothing or not all of it was written.
exit /b 1
