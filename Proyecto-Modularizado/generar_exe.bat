@echo off
echo ==========================================
echo   GENERADOR DE EJECUTABLE - FICHADOR INTI
echo ==========================================
echo.

:: 1. Activar entorno virtual (si existe)
if exist venv\Scripts\activate.bat (
    echo Activando entorno virtual...
    call venv\Scripts\activate.bat
) else (
    echo [INFO] No se detecto carpeta venv. Usando Python global.
)

:: 2. Instalar dependencias
echo Instalando dependencias desde requirements.txt...
if exist requirements.txt (
    pip install -r requirements.txt
) else (
    echo [ALERTA] No se encontro requirements.txt. Instalando paquetes manualmente...
    pip install flask mysql-connector-python flask-socketio gtts pygame eventlet
)

:: 3. Instalar PyInstaller (por si falta)
echo Verificando PyInstaller...
pip install pyinstaller

:: 4. Limpieza previa
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist
if exist *.spec del *.spec

echo.
echo Compilando proyecto... Esto puede tardar unos minutos.
echo.

:: 5. Comando PyInstaller
:: --onefile: Todo en un solo archivo .exe
:: --clean: Limpia cache de compilaciones anteriores
:: --add-data: Incluye la carpeta templates (formato windows: origen;destino)
:: --hidden-import: Fuerza la inclusion de librerias dinamicas

pyinstaller --noconfirm --onefile --clean --name "FichadorINTI" ^
 --add-data "templates;templates" ^
 --hidden-import "mysql.connector" ^
 --hidden-import "mysql.connector.plugins.mysql_native_password" ^
 --hidden-import "engineio.async_drivers.threading" ^
 --hidden-import "flask_socketio" ^
 --hidden-import "gtts" ^
 --hidden-import "pygame" ^
 --hidden-import "pandas" ^
 --hidden-import "openpyxl" ^
 --hidden-import "fpdf" ^
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