import requests
import sys
import os
import json

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Uso: python script.py <nombre_archivo> <URL_barco>")
        sys.exit(1)

    home_dir = os.path.expanduser("~")

    nombre_archivo = sys.argv[1]
    URL = sys.argv[2]

    ruta_csv_barco = os.path.join(home_dir, "PprzGCS", "Planificacion", "JSON", "Barco", nombre_archivo + ".geojson")

    print("Ruta Barco:", ruta_csv_barco)
    headers = {"Content-Type": "application/json"}

    # Cargar y enviar JSON del barco
    try:
        with open(ruta_csv_barco, 'r') as f_barco:
            data_barco = json.load(f_barco)
        response_barco = requests.post(URL, headers=headers, json=data_barco)
        print("Código HTTP Barco:", response_barco.status_code)
        print("Contenido de la respuesta Barco:", response_barco.text)
    except Exception as e:
        print("Error al procesar el archivo del barco:", e)
