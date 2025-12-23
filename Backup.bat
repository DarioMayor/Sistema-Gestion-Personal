@echo off
:: CONFIGURACIÓN
:: Usuario de base de datos 
set dbUser=root
:: Contraseña 
set dbPass=
:: El nombre exacto de tu base de datos
set dbName=gestion_personal_db
:: La ruta donde está mysqldump.exe 
set mysqldumpPath="C:\xampp\mysql\bin\mysqldump.exe"
:: La ruta de destino en OneDrive (¡Cámbiala por tu ruta real!)
set backupPath="C:\Users\Personal\Desktop\Proyecto\BackupDB"

:: OBTENER FECHA Y HORA PARA EL NOMBRE DEL ARCHIVO
set year=%date:~6,4%
set month=%date:~3,2%
set day=%date:~0,2%
:: Reemplaza espacios en la hora con ceros (ej. " 9" -> "09")
set hour=%time:~0,2%
if "%hour:~0,1%" == " " set hour=0%hour:~1,1%
set min=%time:~3,2%

set filename=backup_%dbName%_%year%-%month%-%day%_%hour%%min%.sql

:: EJECUTAR EL RESPALDO
if "%dbPass%"=="" (
    %mysqldumpPath% -u %dbUser% --opt %dbName% > "%backupPath%\%filename%"
) else (
    %mysqldumpPath% -u %dbUser% -p%dbPass% --opt %dbName% > "%backupPath%\%filename%"
)

:: BORRAR RESPALDOS VIEJOS (Mantiene solo los últimos 30 días)
forfiles /p %backupPath% /s /m *.sql /d -30 /c "cmd /c del @path"