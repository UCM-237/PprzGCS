import requests
import sys

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Error: falta el nombre del archivo como parámetro")
        sys.exit(1)

    nombre_archivo = sys.argv[1]

    url = 'https://cyanoa.grupogimeno.com/testing'
    response = requests.post(url, json=nombre_archivo)

    print("Codigo HTTP:", response.status_code)
    print("Contenido de la respuesta:", response.text)

    