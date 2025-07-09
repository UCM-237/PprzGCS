import subprocess
import os
import pandas as pd
from PyQt5.QtWidgets import QApplication, QMessageBox

def run_gcs(ruta_datos):
        
    # Definir la ruta del ejecutable
    GCS_EXECUTABLE = os.path.join(home_dir, "paparazzi", "sw", "ground_segment", "cockpit", "gcs")

    # Ruta del archivo de vuelo que deseas cargar
    flight_plan_file = ruta_datos

    # Crear una instancia de la aplicación Qt
    app = QApplication([])

    # Verificar si el archivo de vuelo existe
    if not os.path.exists(flight_plan_file):
        QMessageBox.critical(None, "Error", f"El archivo de vuelo no se encuentra en la ruta:\n{flight_plan_file}, abriendo un archivo de vuelo en blanco")
        flight_plan_file = os.path.join(home_dir, "paparazzi", "conf", "flight_plans", "UCM", "flight_plan_empty.xml")
        

    # Intentar ejecutar el comando con el archivo de vuelo
    try:
        result = subprocess.run(
            [GCS_EXECUTABLE, "-edit", flight_plan_file],
            check=True,
            capture_output=True,
            text=True
        )
        #QMessageBox.information(None, "Éxito", "GCS ejecutado exitosamente con el archivo de vuelo.")
    except subprocess.CalledProcessError as e:
        error_message = (
            f"Error al ejecutar GCS:\n{e}\n"
            f"Salida estándar:\n{e.stdout or 'No hay salida estándar.'}\n"
            f"Salida de error:\n{e.stderr or 'No hay salida de error.'}"
        )
        QMessageBox.critical(None, "Error", error_message)
    except FileNotFoundError:
        QMessageBox.critical(None, "Error", f"El ejecutable no se encuentra en la ruta:\n{GCS_EXECUTABLE}")
    except Exception as e:
        QMessageBox.critical(None, "Error", f"Error inesperado: {e}")

import sys
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Error: falta el nombre del archivo como parámetro")
        sys.exit(1)

    nombre_archivo = sys.argv[1]  # nombre que recibes desde Qt
    home_dir = os.path.expanduser("~")
    ruta_datos = os.path.join(home_dir, "paparazzi", "conf", "flight_plans", nombre_archivo)
    run_gcs(ruta_datos)
