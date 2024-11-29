import subprocess
import psutil
import time
import os
home_dir = os.path.expanduser("~")
# Función para cerrar los procesos relacionados con Paparazzi y GCS
def close_paparazzi_and_gcs():
    try:
        # Buscar procesos que están usando el puerto 2010
        result = subprocess.run(['lsof', '-t', '-i', ':2010'], stdout=subprocess.PIPE)
        pid_list = result.stdout.decode().splitlines()

        if pid_list:
            for pid in pid_list:
                # Matar cada proceso encontrado en el puerto 2010
                subprocess.run(['kill', '-9', pid])
                print(f"Proceso con PID {pid} terminado.")
        else:
            print("No se encontraron procesos usando el puerto 2010.")

        # Cerrar Paparazzi GCS y otros procesos gráficos
        for proc in psutil.process_iter(['pid', 'name']):
            if 'paparazzi' in proc.info['name'].lower():
                proc.kill()  # Matar el proceso de Paparazzi
                print(f"Proceso Paparazzi con PID {proc.info['pid']} terminado.")
            
            # Cerrar el GCS si está en ejecución (por si acaso)
            if 'groundstation' in proc.info['name'].lower() or 'gcs' in proc.info['name'].lower():
                proc.kill()  # Matar el proceso de la estación de tierra
                print(f"Proceso Estación de Tierra (GCS) con PID {proc.info['pid']} terminado.")

    except Exception as e:
        print(f"Error al cerrar procesos: {e}")

# Llamar a la función para cerrar los procesos antes de iniciar uno nuevo
close_paparazzi_and_gcs()

# Función para abrir Paparazzi GCS
def open_paparazzi():
    try:
        # Ruta al directorio de Paparazzi
        PAPARAZZI_EXECUTABLE = os.path.join(home_dir, "paparazzi", "paparazzi")

        # Ejecutar Paparazzi en segundo plano
        subprocess.Popen([PAPARAZZI_EXECUTABLE])
        print("Paparazzi abierto con éxito.")
    except Exception as e:
        print(f"Error al abrir Paparazzi: {e}")

# Cerrar Paparazzi y GCS
close_paparazzi_and_gcs()

# Esperar unos segundos para asegurar que los procesos se hayan cerrado completamente
time.sleep(2)

# Abrir Paparazzi
open_paparazzi()

