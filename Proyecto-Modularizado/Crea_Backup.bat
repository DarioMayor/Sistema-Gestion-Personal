@echo off
:: --- CONFIGURACIÓN ---
set dbUser=root
:: OJO: Pon tu contraseña real aquí (sin comillas)
set dbPass=eduardolasabe000
set dbName=gestion_personal_db

:: --- RUTAS INTELIGENTES ---
:: %USERPROFILE% detecta automáticamente si eres "Juan", "Admin", etc.
set backupPath=%USERPROFILE%\Desktop\Proyecto\BackupDB
set mysqldumpPath=C:\xampp\mysql\bin\mysqldump.exe

:: --- VERIFICACIÓN DE CARPETA ---
if not exist "%backupPath%" mkdir "%backupPath%"

:: --- FECHA Y HORA ---
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set year=%datetime:~0,4%
set month=%datetime:~4,2%
set day=%datetime:~6,2%
set hour=%datetime:~8,2%
set min=%datetime:~10,2%

set filename=backup_%dbName%_%year%-%month%-%day%_%hour%%min%.sql

:: --- EJECUCIÓN DEL RESPALDO ---
echo Generando respaldo en: "%backupPath%\%filename%"

if "%dbPass%"=="" (
    "%mysqldumpPath%" -u %dbUser% --opt %dbName% > "%backupPath%\%filename%"
) else (
    "%mysqldumpPath%" -u %dbUser% -p%dbPass% --opt %dbName% > "%backupPath%\%filename%"
)

:: --- VERIFICACIÓN DE ÉXITO ---
if exist "%backupPath%\%filename%" (
    echo [EXITO] Respaldo creado correctamente.
) else (
    echo [ERROR] No se pudo crear el archivo. Revisa tu contraseña o la ruta de XAMPP.
    pause
)

:: --- LIMPIEZA ---
:: El "2>nul" oculta el error si no hay archivos viejos para borrar
forfiles /p "%backupPath%" /s /m *.sql /d -30 /c "cmd /c del @path" 2>nul