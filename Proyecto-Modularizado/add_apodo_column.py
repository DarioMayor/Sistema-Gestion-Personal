import mysql.connector
from config import Config

def add_apodo_column():
    try:
        # Forzamos use_pure=True para evitar problemas si se ejecuta en ciertos entornos,
        # aunque para un script suelto no es estrictamente necesario, es buena práctica si usamos la misma config.
        db_config = Config.DB_CONFIG.copy()
        
        print("Conectando a la base de datos...")
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()

        print("Verificando si la columna 'apodo' ya existe...")
        cursor.execute("SHOW COLUMNS FROM usuarios LIKE 'apodo'")
        result = cursor.fetchone()

        if result:
            print("La columna 'apodo' YA EXISTE. No se realizaron cambios.")
        else:
            print("Agregando columna 'apodo' a la tabla 'usuarios'...")
            # Agregamos la columna apodo despues de apellido
            sql = "ALTER TABLE usuarios ADD COLUMN apodo VARCHAR(100) NULL AFTER apellido"
            cursor.execute(sql)
            conn.commit()
            print("¡Columna 'apodo' agregada exitosamente!")

        cursor.close()
        conn.close()

    except mysql.connector.Error as err:
        print(f"Error de Base de Datos: {err}")
    except Exception as e:
        print(f"Error inesperado: {e}")

if __name__ == "__main__":
    add_apodo_column()
