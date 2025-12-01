import os

class Config:
    SECRET_KEY = 'dariomayor'
    
    # Configuración de Base de Datos
    DB_CONFIG = {
        'user': 'root',
        'password': '',
        'host': '127.0.0.1',
        'database': 'gestion_personal_db'
    }