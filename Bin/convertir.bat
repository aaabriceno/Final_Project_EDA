@echo off
chcp 65001 >nul
echo ========================================
echo   CONVERTIDOR CSV A BINARIO
echo   GeoCluster-Tree
echo ========================================
echo.

if "%1"=="" (
    echo Uso: convertir.bat archivo.csv [archivo.bin]
    echo.
    echo Ejemplos:
    echo   convertir.bat Database/puntos500k.csv
    echo   convertir.bat puntos10k.csv puntos10k.bin
    echo   convertir.bat Database/puntos500k.csv Database/puntos500k.bin
    echo.
    pause
    exit /b 1
)

set INPUT_FILE=%1
set OUTPUT_FILE=%2

if "%OUTPUT_FILE%"=="" (
    for %%i in ("%INPUT_FILE%") do set OUTPUT_FILE=%%~ni.bin
)

echo 🔄 Convirtiendo: %INPUT_FILE%
echo 📁 Salida: %OUTPUT_FILE%
echo.

python csv_to_binary.py "%INPUT_FILE%" "%OUTPUT_FILE%"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo   ✅ CONVERSIÓN EXITOSA
    echo ========================================
    echo.
    echo 📁 Archivo binario creado: %OUTPUT_FILE%
    echo 🚀 Ahora puedes usar este archivo en tu programa C++
    echo.
) else (
    echo.
    echo ========================================
    echo   ❌ ERROR EN LA CONVERSIÓN
    echo ========================================
    echo.
)

pause 