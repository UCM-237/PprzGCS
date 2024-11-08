import subprocess
import os

# Definir la ruta del ejecutable
PAPARAZZI_DIR = "/home/dacya-iagesbloom/Documents/paparazzi"  # Asegúrate de tener la ruta correcta
GCS_EXECUTABLE = os.path.join(PAPARAZZI_DIR, "sw", "ground_segment", "cockpit", "gcs")

# Intentar ejecutar el comando
try:
    subprocess.run([GCS_EXECUTABLE], check=True)
    print("GCS ejecutado exitosamente.")
except subprocess.CalledProcessError as e:
    print(f"Error al ejecutar GCS: {e}")
except FileNotFoundError:
    print(f"El ejecutable no se encuentra en la ruta: {GCS_EXECUTABLE}")
