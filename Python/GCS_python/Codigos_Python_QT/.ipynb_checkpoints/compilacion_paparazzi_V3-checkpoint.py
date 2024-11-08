import os
import subprocess
import time
import psutil

PAPARAZZI_DIR = "/home/dacya-iagesbloom/Documents/paparazzi"

# Cambiar al directorio de Paparazzi
os.chdir(PAPARAZZI_DIR)

# Función para realizar make clean y make
def make_clean_and_build():
    try:
        # Ejecutar make clean
        print("Ejecutando make clean...")
        subprocess.run(["make", "clean"], cwd=PAPARAZZI_DIR, check=True)
        print("make clean completado.")

        # Ejecutar make
        print("Ejecutando make...")
        subprocess.run(["make"], cwd=PAPARAZZI_DIR, check=True)
        print("Make ejecutado exitosamente.")
        
    except subprocess.CalledProcessError as e:
        print(f"Error en make: {e}")

# Función para cerrar los procesos relacionados con Paparazzi y GCS
def close_paparazzi_and_gcs():
    try:
        # Buscar procesos que están usando el puerto 2010
        result = subprocess.run(['sudo', 'lsof', '-t', '-i', ':2010'], stdout=subprocess.PIPE)
        pid_list = result.stdout.decode().splitlines()

        if pid_list:
            for pid in pid_list:
                # Matar cada proceso encontrado en el puerto 2010
                subprocess.run(['sudo', 'kill', '-9', pid])
                print(f"Proceso con PID {pid} terminado.")
        else:
            print("No se encontraron procesos usando el puerto 2010.")

        # Cerrar Paparazzi GCS y otros procesos gráficos
        for proc in psutil.process_iter(['pid', 'name']):
            if 'paparazzi' in proc.info['name'].lower():
                proc.kill()  # Matar el proceso de Paparazzi
                print(f"Proceso Paparazzi con PID {proc.info['pid']} terminado.")
            
            if 'groundstation' in proc.info['name'].lower() or 'gcs' in proc.info['name'].lower():
                proc.kill()  # Matar el proceso de la estación de tierra
                print(f"Proceso Estación de Tierra (GCS) con PID {proc.info['pid']} terminado.")

    except Exception as e:
        print(f"Error al cerrar procesos: {e}")

# Cerrar procesos antes de iniciar uno nuevo
close_paparazzi_and_gcs()

# Ejecutar make clean y make
make_clean_and_build()

# Especificar la ruta completa del ejecutable 'paparazzi' y abrirlo en segundo plano
PAPARAZZI_EXECUTABLE = os.path.join(PAPARAZZI_DIR, "paparazzi")
subprocess.Popen([PAPARAZZI_EXECUTABLE])
