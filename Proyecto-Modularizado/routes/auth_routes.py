from flask import Blueprint, render_template, request, redirect, url_for, session, flash
import mysql.connector
from werkzeug.security import check_password_hash, generate_password_hash
from config import Config
from utils.decorators import login_required

# Definimos el Blueprint
auth_bp = Blueprint('auth', __name__)

# --- RUTA DE LOGIN  ---
# Esta función gestiona el inicio de sesión de los usuarios validando sus credenciales.
@auth_bp.route("/login", methods=['GET', 'POST'])
def login():
    if 'logged_in' in session:
        return redirect(url_for('admin.dashboard')) # Redirige al blueprint 'admin'

    if request.method == 'POST':
        legajo = request.form['legajo']
        password = request.form['password']
        
        conn = mysql.connector.connect(**Config.DB_CONFIG)
        cursor = conn.cursor(dictionary=True)
        cursor.execute("SELECT * FROM usuarios WHERE legajo = %s", (legajo,))
        user = cursor.fetchone()
        cursor.close()
        conn.close()
        
        if user and user.get('password_hash') and check_password_hash(user['password_hash'], password):
            session['logged_in'] = True
            session['user_id'] = user['id']
            session['legajo'] = user['legajo']
            session['role'] = user['role']
            session['nombre'] = user['nombre']
            session['apellido'] = user['apellido']
            return redirect(url_for('admin.dashboard'))
        else:
            flash("Legajo o contraseña incorrectos", "error") 
            return redirect(url_for('auth.login'))
            
    return render_template('login.html')

# --- RUTA DE LOGOUT ---
# Esta función cierra la sesión actual del usuario y lo redirige al login.
@auth_bp.route("/logout")
def logout():
    session.clear()
    flash("Has cerrado sesión.", "success")
    return redirect(url_for('auth.login'))

# Esta función permite al usuario logueado cambiar su contraseña y ver sus datos básicos.
# --- RUTA DE PERFIL (MODIFICAR USUARIO) ---
@auth_bp.route("/perfil", methods=['GET', 'POST'])
@login_required
def perfil():
    user_id = session.get('user_id')
    
    conn = mysql.connector.connect(**Config.DB_CONFIG)
    cursor = conn.cursor(dictionary=True)
    
    if request.method == 'POST':
        nueva_password = request.form.get('nueva_password')
        confirmar_password = request.form.get('confirmar_password')
        
        if nueva_password:
            if nueva_password != confirmar_password:
                flash("Las contraseñas no coinciden.", "error")
            else:
                try:
                    password_hash = generate_password_hash(nueva_password)
                    cursor.execute("UPDATE usuarios SET password_hash = %s WHERE id = %s", (password_hash, user_id))
                    conn.commit()
                    flash("Contraseña actualizada exitosamente.", "success")
                except mysql.connector.Error as err:
                    flash(f"Error al actualizar contraseña: {err}", "error")
        else:
            flash("No se ingresó ninguna contraseña nueva.", "error")
            
        cursor.close()
        conn.close()
        return redirect(url_for('auth.perfil'))

    # GET: Mostrar datos
    try:
        cursor.execute("SELECT nombre, apellido, legajo, username as email, horas_laborales FROM usuarios WHERE id = %s", (user_id,))
        user = cursor.fetchone()
        
        # Formatear horas laborales si es necesario
        if user and user.get('horas_laborales'):
            hl = user['horas_laborales']
            # Si es timedelta, convertir a string HH:MM:SS o similar
            user['horas_laborales'] = str(hl)

    except mysql.connector.Error as err:
        flash(f"Error al cargar perfil: {err}", "error")
        user = {}
    finally:
        if conn.is_connected():
            cursor.close()
            conn.close()
            
    return render_template('perfil_usuario.html', usuario=user)