from werkzeug.security import generate_password_hash
import mysql.connector

# Configura tu conexión
db_config = {
    'user': 'root',
    'password': '',
    'host': '127.0.0.1',
    'database': 'gestion_personal_db' # Revisa que sea tu DB
}

# --- DATOS DEL USUARIO A ACTUALIZAR ---
# El ID del usuario en la tabla 'usuarios' (Ej: 1 para 'Dario Agustin')
usuario_id_a_actualizar = 1

# Las nuevas credenciales
nuevo_username = "dariomayor"
nueva_password = "dariomayor" # Pon una clave segura
nuevo_rol = "admin" # 'admin' o 'user'
# -------------------------------------

try:
    # Hashea la contraseña
    password_hash = generate_password_hash(nueva_password)
    
    conn = mysql.connector.connect(**db_config)
    cursor = conn.cursor()
    
    # Actualiza el usuario existente
    sql_update = """
        UPDATE usuarios 
        SET username = %s, password_hash = %s, role = %s 
        WHERE id = %s
    """
    cursor.execute(sql_update, (nuevo_username, password_hash, nuevo_rol, usuario_id_a_actualizar))
    conn.commit()
    
    if cursor.rowcount == 0:
        print(f"\nERROR: No se encontró ningún usuario con el ID {usuario_id_a_actualizar}.")
    else:
        print(f"\n¡Éxito! Usuario con ID {usuario_id_a_actualizar} actualizado.")
        print(f"Username: {nuevo_username}")
        print(f"Rol: {nuevo_rol}")

except mysql.connector.Error as err:
    if err.errno == 1062: # Error de 'Duplicate entry'
        print(f"\nERROR: El username '{nuevo_username}' ya está en uso por otra persona.")
    else:
        print(f"\nError de base de datos: {err}")
finally:
    if 'conn' in locals() and conn.is_connected():
        cursor.close()
        conn.close()