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

# Esta función abre automáticamente el navegador en las ventanas de login y monitor al iniciar la app.
def open_browser():
    print("Abriendo navegador en el Monitor...")
    webbrowser.open_new('http://localhost:5000/login')
    webbrowser.open_new('http://localhost:5000/monitor')

if __name__ == "__main__":
    print("Iniciando servidor Modular...")
    # Evitar que se abra dos veces al usar debug=True (reloader del servidor flask)
    if not os.environ.get("WERKZEUG_RUN_MAIN"):
        Timer(1.5, open_browser).start()
    socketio.run(app, host='0.0.0.0', port=5000, debug=False)