from fpdf import FPDF

class PDF(FPDF):
    # Método para definir el encabezado de cada página del PDF.
    def header(self):
        # Logo (opcional, asegúrate de que la ruta sea correcta si lo descomentas)
        # self.image('static/logo-inti.png', 10, 8, 33)
        self.set_font('Arial', 'B', 15)
        self.cell(0, 10, 'Registro de Fichajes', 0, 1, 'C')
        self.ln(5)

    # Método para definir el pie de página de cada hoja, mostrando el número de página.
    def footer(self):
        self.set_y(-15)
        self.set_font('Arial', 'I', 8)
        self.cell(0, 10, f'Pagina {self.page_no()}/{{nb}}', 0, 0, 'C')