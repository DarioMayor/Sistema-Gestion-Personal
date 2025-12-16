import mysql.connector
from config import Config

def add_gender_column():
    try:
        print("Conectando a la base de datos...")
        conn = mysql.connector.connect(**Config.DB_CONFIG)
        cursor = conn.cursor()
        
        print("Añadiendo columna 'genero' a la tabla 'usuarios'...")
        # Añadimos columna genero: M (Masculino), F (Femenino), X (Otro/No binario)
        # Default 'X' para no romper usuarios existentes
        cursor.execute("ALTER TABLE usuarios ADD COLUMN genero ENUM('M', 'F', 'X') DEFAULT 'X'")
        
        conn.commit()
        print("¡Éxito! Columna 'genero' añadida.")
        
    except mysql.connector.Error as err:
        # Si el error es 1060 (Duplicate column name), es que ya existe
        if err.errno == 1060:
            print("La columna 'genero' ya existe.")
        else:
            print(f"Error de base de datos: {err}")
    except Exception as e:
        print(f"Error inesperado: {e}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()
            print("Conexión cerrada.")

if __name__ == "__main__":
    add_gender_column()
