from functools import wraps
from flask import session, redirect, url_for, flash

# Este decorador asegura que el usuario esté logueado antes de permitir el acceso a la vista.
def login_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'logged_in' not in session:
            flash("Por favor, inicia sesión para ver esta página.", "error")
            return redirect(url_for('auth.login')) # Nota: 'auth.login' es el nuevo nombre
        return f(*args, **kwargs)
    return decorated_function


# Este decorador asegura que el usuario tenga el rol de 'admin' para acceder a la vista.
# Si está logueado pero no es admin, lo redirige al dashboard.
def admin_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        # Si no está logueado
        if 'logged_in' not in session:
            flash("Por favor, inicia sesión para ver esta página.", "error")
            return redirect(url_for('auth.login'))
        # Si está logueado PERO NO es admin
        if session.get('role') != 'admin':
            flash("No tienes permiso de administrador para ver esta página.", "error")
            return redirect(url_for('admin.dashboard')) 
        return f(*args, **kwargs)
    return decorated_function