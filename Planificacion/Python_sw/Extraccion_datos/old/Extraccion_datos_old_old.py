import os
import sys
import numpy as np

def extraccion_datos(var, pos, ruta_datos):
    dato = []
    tiempo = []
    with open(ruta_datos, "r", encoding="utf-8") as file:
        for line in file:
            if var in line:
                campos = line.strip().split()
                if campos[2] == var:
                    dato.append(float(campos[pos]))
                    tiempo.append(float(campos[0]))
    return np.array(dato), np.array(tiempo)

from datetime import datetime, timedelta

def gps_to_datetime_local(week, tow_ms):
    gps_epoch = datetime(1980, 1, 6)
    # GPS time
    gps_time = gps_epoch + timedelta(weeks=int(week), milliseconds=int(tow_ms))
    # Ajuste a UTC (restando los 19 segundos de diferencia GPS–UTC)
    utc_time = gps_time - timedelta(seconds=19)
    # Ajuste a hora local española (CEST: UTC+2 en verano)
    local_time = utc_time + timedelta(hours=2)
    return local_time

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Error: falta el nombre del archivo como parámetro")
        sys.exit(1)

    nombre_archivo = sys.argv[1]  # nombre que recibes desde Qt
    home_dir = os.path.expanduser("~")
    ruta_datos = os.path.join(home_dir, "paparazzi", "var", "logs", nombre_archivo)

    # Validar extensión .data y preparar ruta salida .csv
    nombre_sin_ext, ext = os.path.splitext(nombre_archivo)
    if ext != ".data":
        print("Advertencia: el archivo no termina en .data")

    ruta_salida = os.path.join(home_dir, "PprzGCS", "Planificacion", "Extraccion_datos", nombre_sin_ext + ".csv")

    # Extraer datos
    x_raw, t_x = extraccion_datos("INS", 3, ruta_datos)
    y_raw, t_y = extraccion_datos("INS", 4, ruta_datos)
    lat_raw, t_lat = extraccion_datos("GPS_INT", 6, ruta_datos)
    lon_raw, t_lon = extraccion_datos("GPS_INT", 7, ruta_datos)
    x = x_raw * 0.0039063 #Factor de conversión
    y = y_raw * 0.0039063 #Factor de conversión
    lat = lat_raw * 0.0000001 #Factor de conversión
    lon = lon_raw * 0.0000001 #Factor de conversión
    u_raw, t_u = extraccion_datos("INS", 6, ruta_datos)
    v_raw, t_v = extraccion_datos("INS", 7, ruta_datos)
    u = u_raw * 0.0000019 #Factor de conversión
    v = v_raw * 0.0000019 #Factor de conversión
    theta = np.degrees(np.arctan2(u_raw, v_raw))
    # theta = np.arctan2(u_raw, v_raw)
    du_raw, t_du = extraccion_datos("INS", 9, ruta_datos)
    dv_raw, t_dv = extraccion_datos("INS", 10, ruta_datos)
    du = du_raw * 0.0009766 #Factor de conversión
    dv = dv_raw * 0.0009766 #Factor de conversión
    orientacion_raw, t_orientacion = extraccion_datos("ATTITUDE", 4, ruta_datos)
    throttle_L, t_T_L = extraccion_datos("BOAT_CTRL", 7, ruta_datos)
    throttle_R, t_T_R = extraccion_datos("BOAT_CTRL", 8, ruta_datos)
    Ah, t_Ah = extraccion_datos("ENERGY", 8, ruta_datos)
    week, t_week = extraccion_datos("GPS", 10, ruta_datos) #Semana desde 6 de Enero de 1980
    tow, t_tow = extraccion_datos("GPS", 11, ruta_datos) #tow = time on week
    utm_zone, t_utm_zone = extraccion_datos("GPS", 12, ruta_datos) #tow = time on week

    # Vectoriza para usar con arrays
    vectorized_gps_to_datetime_local = np.vectorize(gps_to_datetime_local)

    # Aplica a tus arrays
    result_local_time = vectorized_gps_to_datetime_local(week, tow)

    # Asegurar que todas las series tengan la misma longitud
    N = min(
        len(t_x), len(x),
        len(t_y), len(y),
        len(t_lat), len(lat),
        len(t_lon), len(lon),
        len(t_u), len(u_raw),
        len(t_v), len(v_raw),
        len(t_du), len(du_raw),
        len(t_dv), len(dv_raw),
        len(t_orientacion), len(orientacion_raw),
        len(t_T_L), len(throttle_L),
        len(t_T_R), len(throttle_R),
        len(t_Ah), len(Ah),
        len(t_week), len(week),
        len(t_tow), len(tow),
        len(t_utm_zone), len(utm_zone)
    )

    # Guardar en csv
    with open(ruta_salida, "w", encoding="utf-8") as f:
        f.write("fecha,t_x,x,t_y,y,t_lat,lat,l_lon,lon,t_utm_zone,utm_zone,t_u,u_raw,t_v,v_raw,t_du,du_raw,t_dv,dv_raw,t_orientacion,orientacion_raw,t_T_L,throttle_L,t_T_R,throttle_R,t_Ah,Ah\n")
        for i in range(N):
            fila = [
                result_local_time[i],
                t_x[i], x[i],
                t_y[i], y[i],
                t_lat[i], lat[i],
                t_lon[i], lon[i],
                t_utm_zone[i], utm_zone[i],
                t_u[i], u[i],
                t_v[i], v[i],
                t_du[i], du[i],
                t_dv[i], dv[i],
                t_orientacion[i], orientacion_raw[i],
                t_T_L[i], throttle_L[i],
                t_T_R[i], throttle_R[i],
                t_Ah[i], Ah[i]             
            ]

            f.write(",".join(str(valor) for valor in fila) + "\n")