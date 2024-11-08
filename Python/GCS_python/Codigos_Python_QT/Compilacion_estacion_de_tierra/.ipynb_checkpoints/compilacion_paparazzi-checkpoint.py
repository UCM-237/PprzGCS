import os
import subprocess
import time

# Definir el nombre del airframe y el flight plan
AIRFRAME = "rover2f405WSE"  # Cambia esto según tu archivo de airframe
FLIGHT_PLAN = "flight_plan_pruebaQT.xml"  # Cambia esto por el nombre de tu archivo de misión

# Definir la ruta del directorio de Paparazzi
PAPARAZZI_DIR = "/home/dacya-iagesbloom/Documents/paparazzi"

# Definir la ruta completa del archivo de flight plan en la carpeta UCM
FLIGHT_PLAN_PATH = os.path.join(PAPARAZZI_DIR, "conf", "flight_plans", "UCM", FLIGHT_PLAN)

# Verificar si el archivo de flight plan existe
if not os.path.exists(FLIGHT_PLAN_PATH):
    print(f"Error: No se encuentra el archivo de flight plan: {FLIGHT_PLAN_PATH}")
    exit(1)

# Cambiar al directorio de Paparazzi
os.chdir(PAPARAZZI_DIR)

# Copiar el archivo de flight plan a su ubicación correspondiente en el directorio de flight_plans
DEST_PATH = os.path.join(PAPARAZZI_DIR, "conf", "flight_plans", "UCM")
subprocess.run(["cp", FLIGHT_PLAN_PATH, DEST_PATH])

# Compilar el airframe y el flight plan con target=np
compilation = subprocess.run(["make", f"AIRCRAFT={AIRFRAME}", "TARGET=np"])

# Verificar si la compilación fue exitosa
if compilation.returncode == 0:
    print("Compilación exitosa. Reiniciando GCS...")
    
    # Especificar la ruta completa del ejecutable 'paparazzi'
    PAPARAZZI_EXECUTABLE = os.path.join(PAPARAZZI_DIR, "paparazzi")
    
    # Reiniciar la GCS
    subprocess.run([PAPARAZZI_EXECUTABLE, "&"])
else:
    print("Error en la compilación.")
