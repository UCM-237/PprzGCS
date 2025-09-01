import requests
import sys
import os
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Error: falta el nombre del archivo como parámetro")
        sys.exit(1)

    home_dir = os.path.expanduser("~")

    nombre_archivo = sys.argv[1]
    URL_barco = sys.argv[2]
    URL_sonda = sys.argv[3]

    ruta_csv_barco = os.path.join(home_dir, "PprzGCS", "Planificacion", "JSON", "Barco", nombre_archivo + ".geojson")
    ruta_csv_sonda = os.path.join(home_dir, "PprzGCS", "Planificacion", "JSON", "Sonda", nombre_archivo + "_sonda.geojson")

    print("ruta barco", ruta_csv_barco)
    print("ruta sonda", ruta_csv_sonda)
    response_barco = requests.post(URL_barco, json=ruta_csv_barco)
    response_sonda = requests.post(URL_sonda, json=ruta_csv_sonda)

    print("Codigo HTTP Barco:", response_barco.status_code)
    print("Contenido de la respuesta Barco:", response_barco.text)

    print("Codigo HTTP Sonda:", response_sonda.status_code)
    print("Contenido de la respuesta Sonda:", response_sonda.text)

    