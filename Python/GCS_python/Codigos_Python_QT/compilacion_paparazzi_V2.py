import os
import subprocess
import psutil
import time

PAPARAZZI_DIR = "/home/dacya-iagesbloom/Documents/paparazzi"

# Cambiar al directorio de Paparazzi
os.chdir(PAPARAZZI_DIR)

# Hacer un 'make clean' para limpiar archivos anteriores
try:
    subprocess.run(["make", "clean"], cwd=PAPARAZZI_DIR, check=True)
    print("Limpieza exitosa (make clean)")
except subprocess.CalledProcessError as e:
    print(f"Error en make clean: {e}")

# Hacer el 'make' con el target 'nps'
try:
    subprocess.run(["make", "TARGET=nps"], cwd=PAPARAZZI_DIR, check=True)
    print("Compilación con TARGET=nps ejecutada exitosamente")
except subprocess.CalledProcessError as e:
    print(f"Error en la compilación con TARGET=nps: {e}")


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
            
            # Cerrar el GCS si está en ejecución (por si acaso)
            if 'groundstation' in proc.info['name'].lower() or 'gcs' in proc.info['name'].lower():
                proc.kill()  # Matar el proceso de la estación de tierra
                print(f"Proceso Estación de Tierra (GCS) con PID {proc.info['pid']} terminado.")

    except Exception as e:
        print(f"Error al cerrar procesos: {e}")

# Llamar a la función para cerrar los procesos antes de iniciar uno nuevo
close_paparazzi_and_gcs()

# Especificar la ruta completa del ejecutable 'paparazzi'
PAPARAZZI_EXECUTABLE = os.path.join(PAPARAZZI_DIR, "paparazzi")

# Reiniciar la GCS después de la compilación exitosa
try:
    subprocess.Popen([PAPARAZZI_EXECUTABLE])
    print("Paparazzi abierto con éxito.")
except Exception as e:
    print(f"Error al abrir Paparazzi: {e}")
