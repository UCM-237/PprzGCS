import os
import csv
import sys
import numpy as np
import pandas as pd
from datetime import datetime, timedelta


######## FUNCIONES #######

# Lee el log CSV de navegación (con cabecera)
# y devuelve un diccionario con listas por cada campo.
def extraccion_datos_nav(ruta_log):

    datos = {
        "time": [],
        "lat": [],
        "lon": [],
        "Ah": [],
        "profile": [],
        "time_UTC": [],
        "orient_raw": [],
        "theta": [],
        "static_control": [],
        "throttle_L": [],
        "throttle_R": [],
        "x": [],
        "y": [],
        "utm_zone": [],
        "u_raw": [],
        "v_raw": [],
        "du_raw": [],
        "dv_raw": []
    }

    with open(ruta_log, newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                datos["time"].append(float(row["time"]))
                datos["lat"].append(float(row["lat"]))
                datos["lon"].append(float(row["lon"]))
                datos["Ah"].append(float(row["Ah"]))
                datos["profile"].append(int(row["profile"]))
                datos["time_UTC"].append(row["time_UTC"])
                datos["orient_raw"].append(float(row["orient_raw"]))
                datos["theta"].append(float(row["theta"]))
                datos["static_control"].append(int(row["static_control"]))
                datos["throttle_L"].append(float(row["throttle_L"]))
                datos["throttle_R"].append(float(row["throttle_R"]))
                datos["x"].append(float(row["x"]))
                datos["y"].append(float(row["y"]))
                datos["utm_zone"].append(int(row["utm_zone"]))
                datos["u_raw"].append(float(row["u_raw"]))
                datos["v_raw"].append(float(row["v_raw"]))
                datos["du_raw"].append(float(row["du_raw"]))
                datos["dv_raw"].append(float(row["dv_raw"]))
            except Exception as e:
                print(f"Línea ignorada por error de formato: {e}")
                continue

    print(f"{len(datos['time'])} registros leídos del log.")
    return datos


# Carga el CSV de la sonda y normaliza las columnas
def extraccion_datos_sonda(ruta_csv_sonda: str):

    df = pd.read_csv(ruta_csv_sonda)

    df.columns = [
        "Tiempo",
        "Perfil",
        "Profundidad",
        "Temperatura",
        "pH",
        "DO_SAT",
        "DO",
        "Blue",
        "Chl", 
        "C"
    ]

    # Convertir tiempo a datetime por si se quiere usar después
    df["Tiempo"] = pd.to_datetime(df["Tiempo"], errors="coerce")

    return df


################################################################################################################
if __name__ == "__main__":
    if len(sys.argv) < 1:
        print("Error: falta el nombre del archivo como parámetro")
        sys.exit(1)

    #Para que coja los datos que se mandan desde QT
    archivo_nav = sys.argv[1]  #Nombre del archivo de la misión
    # archivo_sonda = sys.argv[2]
    # print("Archivo nav:", archivo_nav)
    
    # Validar extensión .csv y preparar ruta salida .csv
    archivo_nav_sin_ext, ext = os.path.splitext(archivo_nav)
    if ext != ".csv":
        print("Advertencia: el archivo no termina en .csv")
    if archivo_nav_sin_ext.startswith("log_"):
        fecha = archivo_nav_sin_ext.replace("log_", "", 1)
    else:
        fecha = archivo_nav_sin_ext

    archivo_sonda_sin_ext = "sonda_" + fecha
    archivo_sonda = archivo_sonda_sin_ext + ".csv"


    home_dir = os.path.expanduser("~")
    ruta_nav = os.path.join(home_dir, "PprzGCS", "Planificacion", "Resources","logs", "nav", archivo_nav)
    ruta_sonda = os.path.join(home_dir, "PprzGCS", "Planificacion", "Resources","logs", "sonda", archivo_sonda)
    salida_nav = os.path.join(home_dir, "PprzGCS", "Planificacion", "Extraccion_datos", "Barco", archivo_nav_sin_ext + ".csv")
    salida_sonda = os.path.join(home_dir, "PprzGCS", "Planificacion", "Extraccion_datos", "Sonda", archivo_sonda_sin_ext + ".csv")
    
    # print(f"ruta datos sonda {ruta_sonda} \n")
    # print(f"ruta datos nav {ruta_nav} \n")
    # print(f"salida datos nav {salida_nav} \n")
    # print(f"salida datos sonda {salida_sonda} \n")


    ########## EXTRACCIÓN DE DATOS DE LOS MENSAJES ##########
    nav = extraccion_datos_nav(ruta_nav)

    t_comun = nav["time"]
    x = nav["x"]
    y = nav["y"]

    lat = nav["lat"]
    lon = nav["lon"]

    u = nav["u_raw"]
    v = nav["v_raw"]

    du = nav["du_raw"]
    dv = nav["dv_raw"]

    orientacion_raw = nav["orient_raw"]
    throttle_L = nav["throttle_L"]
    throttle_R = nav["throttle_R"]

    Ah = nav["Ah"]

    profile_id = nav["profile"]
    static_control = nav["static_control"]

    utm_zone = nav["utm_zone"]
    theta = np.degrees(np.arctan2(u, v))

    utc_time = nav["time_UTC"]
    

    ########## EXTRACCIÓN DATOS CSV DE LA RASP ##########
    df = extraccion_datos_sonda(ruta_sonda)

    t_sonda = df["Tiempo"].tolist()
    profile_sonda = df["Perfil"].tolist()
    profundidad = df["Profundidad"].tolist()
    temperatura = df["Temperatura"].tolist()
    pH = df["pH"].tolist()
    DO_SAT = df["DO_SAT"].tolist()
    DO = df["DO"].tolist()  
    Blue = df["Blue"].tolist()
    Chl = df["Chl"].tolist()
    C = df["C"].tolist()

    # Una vez extraidos los datos, hacemos dos csv,
    # Uno con los datos de navegación y 
    # otro con los datos de la sonda + lo necesario.


    # CSV de datos de navegación
    with open(salida_nav, "w", encoding="utf-8") as f:    
        f.write("fecha_utc,t_comun,x,y,lat,lon,utm_zone,u,v,du,dv,orientacion,theta,throttle_L,throttle_R,Ah,profile_id,static_control\n")
        # Vamos de momento a hacer la prueba con la cabecera
        j = 0
        N = len(t_comun)
        for i in range(N):
            fila = [
                utc_time[i],
                t_comun[i],
                x[i],
                y[i],
                lat[i],
                lon[i],
                utm_zone[i],
                u[i],
                v[i],
                du[i],
                dv[i],
                orientacion_raw[i],
                theta[i],
                throttle_L[i],
                throttle_R[i],
                Ah[i],
                profile_id[i],
                static_control[i]
            ]
            j+=1
            f.write(",".join(str(valor) for valor in fila) + "\n")


    # CSV de datos de sonda + navegacion
    with open(salida_sonda, "w", encoding="utf-8") as f:  
        f.write("fecha_utc,x,y,lat,lon,utm_zone,profile_id, t_ini, t_fin,Blue, Chl, DO, DO_SAT, pH, C, Profundidad, Temperatura\n")
        
        perfiles_unicos = sorted(df["Perfil"].unique())
        print("Perfiles detectados:", perfiles_unicos)
        
        for perfil in perfiles_unicos:
            # Filtrar datos del perfil actual
            df_sonda_perfil = df[df["Perfil"] == perfil]

            # Convertir profile_id a numpy array para la máscara
            profile_nav = np.array(nav["profile"])
            mask_nav = profile_nav == perfil
            
            if not np.any(mask_nav):
                print(f"Aviso: perfil {perfil} no encontrado en navegación.")
                continue

            if max(df_sonda_perfil["Profundidad"]) <= 0:
                print(f"Aviso: perfil {perfil} tiene profundidad máxima <= 0.")
                continue

            # Tomar el primer registro del perfil como referencia
            # Encontrar el primer índice que cumple la condición
            first_index = np.where(mask_nav)[0][0]
            last_index = np.where(mask_nav)[0][-1]

            # Extraer datos base del barco usando el índice
            utc_time = nav["time_UTC"][first_index]
            x = nav["x"][first_index]
            y = nav["y"][first_index]
            lat = nav["lat"][first_index]
            lon = nav["lon"][first_index]
            utm_zone = nav["utm_zone"][first_index]
            t_ini = nav["time"][first_index]
            t_fin = nav["time"][last_index]

            # Escribir una línea por cada medición de la sonda
            for _, fila_sonda in df_sonda_perfil.iterrows():
                fila = [
                    utc_time,
                    x,
                    y,
                    lat,
                    lon,
                    utm_zone,
                    perfil,
                    t_ini,
                    t_fin,
                    fila_sonda["Blue"],
                    fila_sonda["Chl"],
                    fila_sonda["DO"],
                    fila_sonda["DO_SAT"],
                    fila_sonda["pH"],
                    fila_sonda["C"],
                    fila_sonda["Profundidad"],
                    fila_sonda["Temperatura"],
                ]
                f.write(",".join(str(valor) for valor in fila) + "\n")
        
    