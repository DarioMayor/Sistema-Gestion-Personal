from flask import Blueprint, request, jsonify, render_template
import mysql.connector
import datetime
from config import Config
from extensions import socketio # Importamos la instancia compartida
import threading
import os
import time
from gtts import gTTS
import pygame
import uuid

# Inicializar pygame mixer para el audio
try:
    pygame.mixer.init()
except Exception as e:
    print(f"Advertencia: No se pudo inicializar el audio: {e}")

def reproducir_audio(texto):
    try:
        # Generar audio con gTTS
        tts = gTTS(text=texto, lang='es')
        filename = f"temp_audio_{uuid.uuid4()}.mp3"
        tts.save(filename)
        
        # Reproducir con pygame
        pygame.mixer.music.load(filename)
        pygame.mixer.music.play()
        
        # Esperar a que termine
        while pygame.mixer.music.get_busy():
            time.sleep(0.1)
            
        pygame.mixer.music.unload()
        os.remove(filename)
    except Exception as e:
        print(f"Error al reproducir audio: {e}")
        if 'filename' in locals() and os.path.exists(filename):
            try:
                os.remove(filename)
            except:
                pass

main_bp = Blueprint('main', __name__)

# --- RUTA 1: El monitor visual ---
@main_bp.route("/monitor")
def monitor():
    return render_template('monitor.html')

# --- RUTA 2: El endpoint para el ESP32  ---
@main_bp.route("/fichar", methods=['POST'])
def recibir_fichaje():
    datos = request.json
    sensor_id = datos.get('id_huella') 
    
    if not sensor_id:
        return jsonify({"status": "error", "mensaje": "No se recibió id_huella"}), 400

    print(f"--- Nuevo Fichaje ---")
    print(f"Sensor ID recibido: {sensor_id}")

    try:
        conn = mysql.connector.connect(**Config.DB_CONFIG)
        cursor = conn.cursor(dictionary=True) 

        # PASO A: Buscar el usuario
        cursor.execute("SELECT usuario_id FROM huellas WHERE huella_id = %s", (sensor_id,))
        usuario_encontrado = cursor.fetchone() 

        if not usuario_encontrado:
            print(f"Error: Sensor ID {sensor_id} no está registrado.")
            return jsonify({"status": "error", "mensaje": "Huella no registrada"}), 404

        usuario_id = usuario_encontrado['usuario_id']
        print(f"Usuario encontrado: ID {usuario_id}")

        # PASO B: Decidir si es ENTRADA o SALIDA (Lógica Diaria)
        
        # 1. Buscamos si hay fichajes HOY para este usuario
        today = datetime.date.today()
        
        sql_ultimo_hoy = """
            SELECT tipo FROM fichajes 
            WHERE usuario_id = %s AND DATE(timestamp) = %s
            ORDER BY timestamp DESC 
            LIMIT 1
        """
        cursor.execute(sql_ultimo_hoy, (usuario_id, today))
        ultimo_fichaje_hoy = cursor.fetchone()

        nuevo_tipo = ""
        
        if not ultimo_fichaje_hoy:
            # CASO 1: No hay fichajes hoy.
            # Por lo tanto, es el primer movimiento del día -> ENTRADA
            # (Esto arregla automáticamente el olvido de salida de ayer)
            nuevo_tipo = "entrada"
            print(">> Primer fichaje del día: ENTRADA")
        else:
            # CASO 2: Ya fichó hoy. Seguimos la secuencia normal.
            tipo_anterior = ultimo_fichaje_hoy['tipo']
            
            if tipo_anterior == "entrada":
                nuevo_tipo = "salida"
            else: # Si el último fue salida (ej: salió a almorzar)
                nuevo_tipo = "entrada" # Vuelve a entrar
            
            print(f">> Fichaje previo hoy fue {tipo_anterior}. Nuevo: {nuevo_tipo}")
        
        # PASO C: Insertar el nuevo fichaje
        sql_insert = "INSERT INTO fichajes (usuario_id, timestamp, tipo) VALUES (%s, NOW(), %s)"
        valores = (usuario_id, nuevo_tipo)
        cursor.execute(sql_insert, valores)
        id_fichaje_nuevo = cursor.lastrowid
        conn.commit() 

        print(f"¡Éxito! Fichaje guardado en la DB.")
        
        # --- Notificar al Monitor ---
        sql_get_data = """
            SELECT f.id, f.timestamp, f.tipo, u.nombre, u.apellido 
            FROM fichajes AS f
            JOIN usuarios AS u ON f.usuario_id = u.id
            WHERE f.id = %s
        """
        cursor.execute(sql_get_data, (id_fichaje_nuevo,))
        datos_para_monitor = cursor.fetchone()
        
        if datos_para_monitor:
            datos_para_monitor['timestamp'] = datos_para_monitor['timestamp'].isoformat()
            socketio.emit('nuevo_fichaje', datos_para_monitor)
            print("Evento WebSocket emitido.")

            # --- AUDIO TTS ---
            try:
                nombre = datos_para_monitor['nombre']
                # Mensaje simple sin genero
                mensaje = f"Bienvenido {nombre}" if nuevo_tipo == "entrada" else f"Hasta luego {nombre}"
                threading.Thread(target=reproducir_audio, args=(mensaje,)).start()
            except Exception as e:
                print(f"Error lanzando hilo de audio: {e}")
            
        print("-----------------------\n")
        cursor.close()
        conn.close()

    except mysql.connector.Error as err:
        print(f"Error de Base de Datos: {err}")
        return jsonify({"status": "error", "mensaje": "Error de base de datos"}), 500

    return jsonify({"status": "ok", "recibido": sensor_id, "tipo": nuevo_tipo}), 200

@main_bp.route("/fichar_error", methods=['POST'])
def recibir_error_fichaje():
    """
    El ESP32 llama a esta ruta cuando el sensor no reconoce la huella.
    No toca la DB, solo avisa a los monitores.
    """
    print("--- Fichaje Fallido ---")
    print("Evento WebSocket 'huella_no_reconocida' emitido al monitor.")
    
    # Emite el nuevo evento a todos los monitores
    socketio.emit('huella_no_reconocida')
    
    # Responde OK al ESP32
    return jsonify({"status": "error_notificado"}), 200