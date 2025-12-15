from werkzeug.security import generate_password_hash
import mysql.connector
from config import Config

# --- DATOS DEL ADMIN POR DEFECTO ---
legajo = "0000"
password = "dariomayor"
nombre = "Dario Agustin"
apellido = "Mayor"
email = "dariomayor@dariomayor.com" # Opcional, se guarda en columna 'username'
role = "admin"
horas_laborales = "08:00:00"
# -----------------------------------

print(f"Intentando crear/actualizar usuario administrador con Legajo: {legajo}")

try:
    # Generar hash de la contraseña
    password_hash = generate_password_hash(password)
    
    # Conectar a la DB usando la configuración del proyecto
    conn = mysql.connector.connect(**Config.DB_CONFIG)
    cursor = conn.cursor()
    
    # 1. Verificar si ya existe por LEGAJO (identificador principal ahora)
    cursor.execute("SELECT id FROM usuarios WHERE legajo = %s", (legajo,))
    user = cursor.fetchone()
    
    if user:
        print(f"-> El usuario con legajo '{legajo}' ya existe (ID: {user[0]}). Actualizando contraseña, rol y datos...")
        # Actualizamos también el username (email) por si cambió
        sql_update = "UPDATE usuarios SET password_hash=%s, role=%s, username=%s, nombre=%s, apellido=%s, horas_laborales=%s WHERE id=%s"
        cursor.execute(sql_update, (password_hash, role, email, nombre, apellido, horas_laborales, user[0]))
    else:
        print(f"-> Creando nuevo usuario administrador Legajo '{legajo}'...")
        sql_insert = """
            INSERT INTO usuarios (nombre, apellido, legajo, username, password_hash, role, horas_laborales)
            VALUES (%s, %s, %s, %s, %s, %s, %s)
        """
        cursor.execute(sql_insert, (nombre, apellido, legajo, email, password_hash, role, horas_laborales))

    conn.commit()
    print("\n¡Éxito! Puedes iniciar sesión con:")
    print(f"Legajo: {legajo}")
    print(f"Contraseña: {password}")

except mysql.connector.Error as err:
    print(f"\nError de base de datos: {err}")
except Exception as e:
    print(f"\nError inesperado: {e}")
finally:
    if 'conn' in locals() and conn.is_connected():
        cursor.close()
        conn.close()