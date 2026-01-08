from flask import Blueprint, render_template, request, flash, redirect, url_for, session
import mysql.connector
import datetime
from datetime import date
from config import Config
from utils.decorators import admin_required

# Creamos el Blueprint con nombre 'fichajes'
fichajes_bp = Blueprint('fichajes', __name__)

# 1. VER LISTA Y AGREGAR NUEVO (MANUAL)
# Esta función permite al administrador ver la lista de fichajes de una fecha específica,
# calcular alertas de fichajes impares (entradas sin salidas o viceversa) y agregar nuevos fichajes manualmente.
@fichajes_bp.route("/admin/fichajes", methods=['GET', 'POST'])
@admin_required
def admin_fichajes():
    conn = mysql.connector.connect(**Config.DB_CONFIG)
    cursor = conn.cursor(dictionary=True)
    
    fecha_filtro = request.args.get('fecha', date.today().isoformat())

    if request.method == 'POST':
        try:
            usuario_id = request.form['usuario_id']
            fecha_input = request.form['fecha']
            hora_input = request.form['hora']
            tipo = request.form['tipo']
            
            timestamp_str = f"{fecha_input} {hora_input}"
            
            sql_insert = "INSERT INTO fichajes (usuario_id, timestamp, tipo) VALUES (%s, %s, %s)"
            cursor.execute(sql_insert, (usuario_id, timestamp_str, tipo))
            conn.commit()
            flash("Fichaje agregado manualmente con éxito.", "success")
            return redirect(url_for('fichajes.admin_fichajes', fecha=fecha_input))
            
        except mysql.connector.Error as err:
            flash(f"Error al agregar: {err}", "error")
    
    # Lógica GET
    cursor.execute("SELECT id, nombre, apellido, legajo FROM usuarios ORDER BY apellido, nombre")
    lista_usuarios = cursor.fetchall()
    
    start_dt = f"{fecha_filtro} 00:00:00"
    end_dt = f"{fecha_filtro} 23:59:59"
    
    sql_list = """
        SELECT f.id, f.timestamp, f.tipo, u.id as uid, u.nombre, u.apellido, u.legajo, u.horas_laborales,
        (SELECT COUNT(*) FROM fichajes_historial fh WHERE fh.fichaje_id = f.id) as num_modificaciones
        FROM fichajes f
        JOIN usuarios u ON f.usuario_id = u.id
        WHERE f.timestamp BETWEEN %s AND %s
        ORDER BY f.timestamp ASC
    """
    cursor.execute(sql_list, (start_dt, end_dt))
    fichajes_dia = cursor.fetchall()

    # --- CÁLCULO DE ALERTAS ---
    fichajes_por_usuario = {}
    for f in fichajes_dia:
        uid = f['uid']
        if uid not in fichajes_por_usuario:
            fichajes_por_usuario[uid] = {'entradas': 0, 'salidas': 0}
        
        if f['tipo'] == 'entrada':
            fichajes_por_usuario[uid]['entradas'] += 1
        elif f['tipo'] == 'salida':
            fichajes_por_usuario[uid]['salidas'] += 1

    usuarios_con_alerta_impar = []
    for uid, conteo in fichajes_por_usuario.items():
        if conteo['entradas'] != conteo['salidas']:
            usuarios_con_alerta_impar.append(uid)

    for f in fichajes_dia:
        if isinstance(f['timestamp'], datetime.datetime):
            f['hora_str'] = f['timestamp'].strftime('%H:%M:%S')
        
        if f['uid'] in usuarios_con_alerta_impar:
            f['alerta_impar'] = True
        else:
            f['alerta_impar'] = False

    cursor.close()
    conn.close()
    
    return render_template('admin_fichajes.html', 
                           fichajes=fichajes_dia, 
                           usuarios=lista_usuarios, 
                           fecha_seleccionada=fecha_filtro)


# 2. EDITAR FICHAJE (MANUAL)
# Esta función permite editar un fichaje existente.
# Antes de guardar los cambios, registra el estado anterior del fichaje en la tabla de historial
# para mantener una auditoría de las modificaciones realizadas.
@fichajes_bp.route("/admin/fichajes/editar/<int:fichaje_id>", methods=['GET', 'POST'])
@admin_required
def editar_fichaje(fichaje_id):
    conn = mysql.connector.connect(**Config.DB_CONFIG)
    cursor = conn.cursor(dictionary=True)
    
    # Obtenemos el fichaje ACTUAL antes de tocar nada (Para historial si lo implementaste)
    cursor.execute("SELECT * FROM fichajes WHERE id = %s", (fichaje_id,))
    fichaje_actual = cursor.fetchone()

    if not fichaje_actual:
        flash("Fichaje no encontrado", "error")
        return redirect(url_for('fichajes.admin_fichajes'))

    if request.method == 'POST':
        try:
            fecha_input = request.form['fecha']
            hora_input = request.form['hora']
            tipo = request.form['tipo']
            timestamp_str = f"{fecha_input} {hora_input}"
            
            # --- INSERTAR HISTORIAL ---
            sql_historial = """
                INSERT INTO fichajes_historial 
                (fichaje_id, fecha_modificacion, modificado_por, timestamp_original, tipo_original)
                VALUES (%s, NOW(), %s, %s, %s)
            """
            admin_username = session.get('username', 'Admin')
            cursor.execute(sql_historial, (
                fichaje_id, 
                admin_username, 
                fichaje_actual['timestamp'], 
                fichaje_actual['tipo']
            ))
            # --------------------------

            sql_update = "UPDATE fichajes SET timestamp=%s, tipo=%s WHERE id=%s"
            cursor.execute(sql_update, (timestamp_str, tipo, fichaje_id))
            conn.commit()
            flash("Fichaje actualizado.", "success")
            return redirect(url_for('fichajes.admin_fichajes', fecha=fecha_input))
            
        except mysql.connector.Error as err:
            flash(f"Error al actualizar: {err}", "error")
    
    # GET: Datos completos para el formulario
    cursor.execute("""
        SELECT f.*, u.nombre, u.apellido 
        FROM fichajes f 
        JOIN usuarios u ON f.usuario_id = u.id 
        WHERE f.id = %s
    """, (fichaje_id,))
    fichaje = cursor.fetchone()
    cursor.close()
    conn.close()

    if isinstance(fichaje['timestamp'], datetime.datetime):
        fichaje['fecha_val'] = fichaje['timestamp'].strftime('%Y-%m-%d')
        fichaje['hora_val'] = fichaje['timestamp'].strftime('%H:%M') 

    return render_template('editar_fichaje.html', fichaje=fichaje)


# 3. ELIMINAR FICHAJE (MANUAL)
# Esta función permite eliminar un fichaje de la base de datos.
# Recibe el ID del fichaje a eliminar y redirige a la lista de fichajes de la fecha correspondiente.
@fichajes_bp.route("/admin/fichajes/eliminar/<int:fichaje_id>", methods=['POST'])
@admin_required
@admin_required
def eliminar_fichaje(fichaje_id):
    fecha_retorno = request.form.get('fecha_retorno', date.today().isoformat())
    
    try:
        conn = mysql.connector.connect(**Config.DB_CONFIG)
        cursor = conn.cursor()
        cursor.execute("DELETE FROM fichajes WHERE id = %s", (fichaje_id,))
        conn.commit()
        cursor.close()
        conn.close()
        flash("Fichaje eliminado correctamente.", "success")
    except mysql.connector.Error as err:
        flash(f"Error al eliminar: {err}", "error")
        
    return redirect(url_for('fichajes.admin_fichajes', fecha=fecha_retorno))
# Esta función muestra el historial de modificaciones de un fichaje específico,
# detallando quién lo modificó, cuándo y cuáles eran los valores originales antes de la edición.

# 4. VER HISTORIAL 
@fichajes_bp.route("/admin/fichajes/historial/<int:fichaje_id>")
@admin_required
def ver_historial(fichaje_id):
    try:
        conn = mysql.connector.connect(**Config.DB_CONFIG)
        cursor = conn.cursor(dictionary=True)
        
        sql = """
            SELECT * FROM fichajes_historial 
            WHERE fichaje_id = %s 
            ORDER BY fecha_modificacion DESC
        """
        cursor.execute(sql, (fichaje_id,))
        historial = cursor.fetchall()
        
        for h in historial:
            if isinstance(h['timestamp_original'], datetime.datetime):
                h['fecha_orig'] = h['timestamp_original'].strftime('%d/%m/%Y %H:%M:%S')
            if isinstance(h['fecha_modificacion'], datetime.datetime):
                h['fecha_mod'] = h['fecha_modificacion'].strftime('%d/%m/%Y %H:%M')

        cursor.close()
        conn.close()
        
        return render_template('historial_fichaje.html', historial=historial, fichaje_id=fichaje_id)

    except mysql.connector.Error as err:
        return f"Error: {err}"