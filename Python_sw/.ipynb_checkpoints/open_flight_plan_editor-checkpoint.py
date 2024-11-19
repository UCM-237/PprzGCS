import subprocess
import os
home_dir = os.path.expanduser("~")

# Definir la ruta del ejecutable
GCS_EXECUTABLE = os.path.join(home_dir,"paparazzi", "sw", "ground_segment", "cockpit", "gcs")  # Asegúrate de tener la ruta correcta

# Ruta del archivo de vuelo que deseas cargar
flight_plan_file = os.path.join(home_dir, "paparazzi", "conf", "flight_plans", "UCM", "flight_plan_empty.xml")  # Cambia esta ruta al archivo de vuelo que deseas cargar

# Verificar si el archivo de vuelo existe
if not os.path.exists(flight_plan_file):
    print(f"El archivo de vuelo no se encuentra en la ruta: {flight_plan_file}")
else:
    # Intentar ejecutar el comando con el archivo de vuelo
    try:
        subprocess.run([GCS_EXECUTABLE, "-edit", flight_plan_file], check=True)
        print("GCS ejecutado exitosamente con el archivo de vuelo.")
    except subprocess.CalledProcessError as e:
        print(f"Error al ejecutar GCS: {e}")
    except FileNotFoundError:
        print(f"El ejecutable no se encuentra en la ruta: {GCS_EXECUTABLE}")
