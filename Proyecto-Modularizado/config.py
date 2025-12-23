import os

class Config:
    SECRET_KEY = 'dariomayor'
    
    # Configuración de Base de Datos
    DB_CONFIG = {
        'user': 'root',
        'password': 'eduardolasabe000',
        'host': '127.0.0.1',
        'database': 'gestion_personal_db',
        'use_pure': True  # Forzar modo Python puro para evitar errores con PyInstaller
    }