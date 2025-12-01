from flask import Blueprint, render_template, request, redirect, url_for, session, flash
import mysql.connector
from werkzeug.security import check_password_hash
from config import Config

# Definimos el Blueprint
auth_bp = Blueprint('auth', __name__)

# --- RUTA DE LOGIN  ---
@auth_bp.route("/login", methods=['GET', 'POST'])
def login():
    if 'logged_in' in session:
        return redirect(url_for('admin.dashboard')) # Redirige al blueprint 'admin'

    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        
        conn = mysql.connector.connect(**Config.DB_CONFIG)
        cursor = conn.cursor(dictionary=True)
        cursor.execute("SELECT * FROM usuarios WHERE username = %s", (username,))
        user = cursor.fetchone()
        cursor.close()
        conn.close()
        
        if user and user.get('password_hash') and check_password_hash(user['password_hash'], password):
            session['logged_in'] = True
            session['user_id'] = user['id']
            session['username'] = user['username']
            session['role'] = user['role']
            session['nombre'] = user['nombre']
            session['apellido'] = user['apellido']
            return redirect(url_for('admin.dashboard'))
        else:
            flash("Email o contraseña incorrectos", "error") 
            return redirect(url_for('auth.login'))
            
    return render_template('login.html')

# --- RUTA DE LOGOUT ---
@auth_bp.route("/logout")
def logout():
    session.clear()
    flash("Has cerrado sesión.", "success")
    return redirect(url_for('auth.login'))