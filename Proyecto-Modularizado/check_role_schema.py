import mysql.connector
from config import Config

def check_schema():
    try:
        print("Conectando a la base de datos...")
        conn = mysql.connector.connect(**Config.DB_CONFIG)
        cursor = conn.cursor()
        
        print("Verificando la columna 'role' en la tabla 'usuarios'...")
        cursor.execute("SHOW COLUMNS FROM usuarios LIKE 'role'")
        result = cursor.fetchone()
        print(f"Definición de columna: {result}")
        
        conn.close()
        
    except mysql.connector.Error as err:
        print(f"Error de base de datos: {err}")
    except Exception as e:
        print(f"Error inesperado: {e}")

if __name__ == "__main__":
    check_schema()
