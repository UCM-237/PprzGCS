import requests
import sys
import os
import json

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Uso: python script.py <nombre_archivo> <URL_sonda>")
        sys.exit(1)

    home_dir = os.path.expanduser("~")

    nombre_archivo = sys.argv[1]
    URL = sys.argv[2]

    ruta_csv_sonda = os.path.join(home_dir, "PprzGCS", "Planificacion", "JSON", "Sonda", nombre_archivo + "_sonda.geojson")

    print("Ruta Sonda:", ruta_csv_sonda)
    headers = {"Content-Type": "application/json"}

    # Cargar y enviar JSON de la sonda
    try:
        with open(ruta_csv_sonda, 'r') as f_sonda:
            data_sonda = json.load(f_sonda)
        response_sonda = requests.post(URL, headers=headers, json=data_sonda)
        print("Código HTTP Sonda:", response_sonda.status_code)
        print("Contenido de la respuesta Sonda:", response_sonda.text)
    except Exception as e:
        print("Error al procesar el archivo de la sonda:", e)
