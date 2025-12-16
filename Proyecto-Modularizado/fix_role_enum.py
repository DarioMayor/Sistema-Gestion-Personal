import mysql.connector
from config import Config

def fix_role_enum():
    try:
        print("Conectando a la base de datos...")
        conn = mysql.connector.connect(**Config.DB_CONFIG)
        cursor = conn.cursor()
        
        print("Modificando la columna 'role' para incluir 'otro'...")
        # Alteramos la columna role para añadir 'otro' al ENUM
        cursor.execute("ALTER TABLE usuarios MODIFY COLUMN role ENUM('admin', 'user', 'otro') DEFAULT 'user'")
        
        conn.commit()
        print("¡Éxito! La columna 'role' ahora acepta 'otro'.")
        
    except mysql.connector.Error as err:
        print(f"Error de base de datos: {err}")
    except Exception as e:
        print(f"Error inesperado: {e}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()
            print("Conexión cerrada.")

if __name__ == "__main__":
    fix_role_enum()
