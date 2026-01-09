from flask import Flask
from config import Config
from extensions import socketio
import datetime

# Importamos los Blueprints
from routes.auth_routes import auth_bp
from routes.main_routes import main_bp
from routes.admin_routes import admin_bp
from routes.fichajes_routes import fichajes_bp

import webbrowser
from threading import Timer
import os
import subprocess

app = Flask(__name__)
app.config.from_object(Config)

# Inicializamos extensiones
socketio.init_app(app)

# Registramos los Blueprints
# Auth y Main van en la raíz ("/")
app.register_blueprint(auth_bp)
app.register_blueprint(main_bp)
# Admin también va en raíz porque definimos las rutas completas dentro (/admin/...)
app.register_blueprint(admin_bp)
app.register_blueprint(fichajes_bp)

# Filtro de fecha para Jinja
# Esta función permite formatear fechas en los templates HTML de 'YYYY-MM-DD' a 'DD/MM/YYYY'.
def format_date_html_filter(date_str):
    try:
        return datetime.datetime.strptime(date_str, '%Y-%m-%d').strftime('%d/%m/%Y')
    except:
        return date_str
app.jinja_env.filters['format_date_html'] = format_date_html_filter

# Esta función abre automáticamente el navegador en la ventana de monitor al iniciar la app.
def open_browser():
    
    # Ruta estandar de Edge (verifica si en la PC es esta)
    edge_path = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
    
    # Lista de URLs a abrir
    urls_to_open = [
        "http://localhost:5000/monitor"
    ]
    
    for url in urls_to_open:
        # ARGUMENTOS MAGICOS:
        # --new-window: Abre ventana nueva siempre
        # --app=...: Abre sin barra de direcciones (parece un programa nativo)
        # --disable-session-crashed-bubble: ELIMINA EL AVISO DE RESTAURAR SESION
        # --disable-infobars: Quita avisos de "está siendo controlado por soft de prueba"
        cmd = [
            edge_path, 
            "--new-window", 
            f"--app={url}", 
            "--disable-session-crashed-bubble", 
            "--disable-infobars" 
        ]
        
        try:
            subprocess.Popen(cmd)
        except (FileNotFoundError, OSError):
            # Si no encuentra Edge, usa el metodo viejo como respaldo
            print(f"No se encontró Edge en {edge_path}, usando navegador por defecto.")
            webbrowser.open_new(url)

if __name__ == "__main__":
    print("Iniciando servidor Modular...")
    # Evitar que se abra dos veces al usar debug=True (reloader del servidor flask)
    if not os.environ.get("WERKZEUG_RUN_MAIN"):
        Timer(1.5, open_browser).start()
    socketio.run(app, host='0.0.0.0', port=5000, debug=False)