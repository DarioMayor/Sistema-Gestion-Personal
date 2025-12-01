from flask import Flask
from config import Config
from extensions import socketio
import datetime

# Importamos los Blueprints
from routes.auth_routes import auth_bp
from routes.main_routes import main_bp
from routes.admin_routes import admin_bp

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

# Filtro de fecha para Jinja
def format_date_html_filter(date_str):
    try:
        return datetime.datetime.strptime(date_str, '%Y-%m-%d').strftime('%d/%m/%Y')
    except:
        return date_str
app.jinja_env.filters['format_date_html'] = format_date_html_filter

if __name__ == "__main__":
    print("Iniciando servidor Modular...")
    socketio.run(app, host='0.0.0.0', port=5000, debug=True, allow_unsafe_werkzeug=True)