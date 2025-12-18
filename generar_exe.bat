@echo off
echo ==========================================
echo   GENERADOR DE EJECUTABLE - FICHADOR INTI
echo ==========================================
echo.

:: 1. Limpieza previa
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist
if exist FichadorINTI.spec del FichadorINTI.spec

echo Compilando proyecto... Por favor espere...
echo.

:: 2. Comando PyInstaller
:: --onefile: Todo en un solo archivo .exe
:: --noconsole: (Opcional) Si quieres ocultar la ventana negra, usa --noconsole. 
::              Por ahora la dejamos visible para ver errores.
:: --add-data: Incluye las carpetas html y css dentro del exe.
:: --hidden-import: Fuerza la inclusion de librerias que PyInstaller a veces no ve.

pyinstaller --noconfirm --onefile --name "FichadorINTI" ^
 --add-data "templates;templates" ^
 --add-data "static;static" ^
 --hidden-import "mysql.connector.plugins.mysql_native_password" ^
 --hidden-import "engineio.async_drivers.threading" ^
 --hidden-import "flask_socketio" ^
 app.py

echo.
echo ==========================================
echo   COMPILACION FINALIZADA
echo ==========================================
echo.
echo Tu archivo FichadorINTI.exe esta en la carpeta "dist".
echo Puedes mover ese archivo a cualquier PC con Windows.
echo.
pause