from flask_socketio import SocketIO

# Inicializamos SocketIO sin la app (se la pasamos después)
# Este objeto manejará la comunicación en tiempo real vía WebSockets.
socketio = SocketIO()