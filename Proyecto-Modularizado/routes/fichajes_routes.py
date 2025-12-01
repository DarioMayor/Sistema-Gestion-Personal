from flask import Blueprint, render_template, request, flash, redirect, url_for
import mysql.connector
import datetime
from datetime import date
from config import Config
from utils.decorators import admin_required

# Creamos el Blueprint con nombre 'fichajes'
fichajes_bp = Blueprint('fichajes', __name__)

# 1. VER LISTA Y AGREGAR NUEVO (MANUAL)
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
            
            # IMPORTANTE: Aquí usamos el nombre del blueprint 'fichajes'
            return redirect(url_for('fichajes.admin_fichajes', fecha=fecha_input))
            
        except mysql.connector.Error as err:
            flash(f"Error al agregar: {err}", "error")
    
    cursor.execute("SELECT id, nombre, apellido, legajo FROM usuarios ORDER BY apellido, nombre")
    lista_usuarios = cursor.fetchall()
    
    start_dt = f"{fecha_filtro} 00:00:00"
    end_dt = f"{fecha_filtro} 23:59:59"
    
    sql_list = """
        SELECT f.id, f.timestamp, f.tipo, u.nombre, u.apellido, u.legajo
        FROM fichajes f
        JOIN usuarios u ON f.usuario_id = u.id
        WHERE f.timestamp BETWEEN %s AND %s
        ORDER BY f.timestamp ASC
    """
    cursor.execute(sql_list, (start_dt, end_dt))
    fichajes_dia = cursor.fetchall()
    
    for f in fichajes_dia:
        if isinstance(f['timestamp'], datetime.datetime):
            f['hora_str'] = f['timestamp'].strftime('%H:%M:%S')

    cursor.close()
    conn.close()
    
    return render_template('admin_fichajes.html', 
                           fichajes=fichajes_dia, 
                           usuarios=lista_usuarios, 
                           fecha_seleccionada=fecha_filtro)


# 2. EDITAR FICHAJE (MANUAL)
@fichajes_bp.route("/admin/fichajes/editar/<int:fichaje_id>", methods=['GET', 'POST'])
@admin_required
def editar_fichaje(fichaje_id):
    conn = mysql.connector.connect(**Config.DB_CONFIG)
    cursor = conn.cursor(dictionary=True)
    
    if request.method == 'POST':
        try:
            fecha_input = request.form['fecha']
            hora_input = request.form['hora']
            tipo = request.form['tipo']
            
            timestamp_str = f"{fecha_input} {hora_input}"
            
            sql_update = "UPDATE fichajes SET timestamp=%s, tipo=%s WHERE id=%s"
            cursor.execute(sql_update, (timestamp_str, tipo, fichaje_id))
            conn.commit()
            flash("Fichaje actualizado.", "success")
            return redirect(url_for('fichajes.admin_fichajes', fecha=fecha_input))
            
        except mysql.connector.Error as err:
            flash(f"Error al actualizar: {err}", "error")
    
    cursor.execute("""
        SELECT f.*, u.nombre, u.apellido 
        FROM fichajes f 
        JOIN usuarios u ON f.usuario_id = u.id 
        WHERE f.id = %s
    """, (fichaje_id,))
    fichaje = cursor.fetchone()
    cursor.close()
    conn.close()
    
    if not fichaje:
        flash("Fichaje no encontrado", "error")
        return redirect(url_for('fichajes.admin_fichajes'))

    if isinstance(fichaje['timestamp'], datetime.datetime):
        fichaje['fecha_val'] = fichaje['timestamp'].strftime('%Y-%m-%d')
        fichaje['hora_val'] = fichaje['timestamp'].strftime('%H:%M') 

    return render_template('editar_fichaje.html', fichaje=fichaje)


# 3. ELIMINAR FICHAJE (MANUAL)
@fichajes_bp.route("/admin/fichajes/eliminar/<int:fichaje_id>", methods=['POST'])
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