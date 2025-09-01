import os
import sys
import numpy as np
import pandas as pd
from datetime import datetime, timedelta

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

def gps_to_datetime_local(week, tow_ms):
    gps_epoch = datetime(1980, 1, 6)
    # GPS time
    gps_time = gps_epoch + timedelta(weeks=int(week), milliseconds=int(tow_ms))
    # Ajuste a UTC (restando los 19 segundos de diferencia GPS–UTC)
    utc_time = gps_time - timedelta(seconds=19)
    # Ajuste a hora local española (CEST: UTC+2 en verano)
    local_time = utc_time
    return local_time

def llevar_a_t_comun(t_comun, t_var, var, check=False):
    var_interp = []
    t_var_interp = []

    i = 0  # índice en t_var
    last_value = var[0]
    last_time = t_var[0]

    for t in t_comun:
        while i < len(t_var) and t_var[i] <= t:
            last_value = var[i]
            last_time = t_var[i]
            i += 1
        var_interp.append(last_value)
        t_var_interp.append(last_time)

    return np.array(var_interp), np.array(t_var_interp)

def extraccion_datos_sonda(ruta):
    try:
        df_datos_sonda = pd.read_csv(ruta, encoding='utf-8', engine='python', on_bad_lines='skip')
        return df_datos_sonda
    except Exception as e:
        print(f"Error al cargar el archivo: {e}")
        return None

################################################################################################################
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Error: falta el nombre del archivo como parámetro")
        sys.exit(1)
    #Para que coja los datos que se mandan desde QT
    nombre_archivo = sys.argv[1]  #Nombre del archivo de la misión
    archivo_medidas = sys.argv[2]
    home_dir = os.path.expanduser("~")
    ruta_datos = os.path.join(home_dir, "paparazzi", "var", "logs", nombre_archivo)

    # Validar extensión .data y preparar ruta salida .csv
    nombre_sin_ext, ext = os.path.splitext(nombre_archivo)
    if ext != ".data":
        print("Advertencia: el archivo no termina en .data")

    ruta_salida = os.path.join(home_dir, "PprzGCS", "Planificacion", "Extraccion_datos", "Barco", nombre_sin_ext + ".csv")
    
    ruta_datos_sonda = archivo_medidas
    print("ruta datos sonda", ruta_datos_sonda)
    ########## EXTRACCIÓN DE DATOS DE LOS MENSAJES ##########
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
    mode, t_mode = extraccion_datos("SERIAL_COM", 3, ruta_datos) #Si está en 2 -> Manual en 4->Auto
    profile_id, t_profile_id = extraccion_datos("STATIC_CONTROL", 5, ruta_datos)
    static_control, t_static_control = extraccion_datos("STATIC_CONTROL", 3, ruta_datos) #Esto nos va a decir cuándo está activo el static control y por tanto cuándo está bajando la sonda
    intervalo_tiempo = 30
    N_tramos_5s = int(t_lat[-1]/intervalo_tiempo)
    #Vamos a crear un vector de tiempos de 5s en 5s
    tiempos_5s = np.linspace(0, t_lat[-1], N_tramos_5s)
    t_comun = tiempos_5s
    
    ########## LLEVAMOS TODOS LOS VECTORES A UN T COMÚN YA QUE CADA UNO TIENE DIFERENTES FRECUENCIA ##########
    x, t_x = llevar_a_t_comun(tiempos_5s, t_x, x)
    y_raw, t_y = llevar_a_t_comun(tiempos_5s, t_y, y)
    u, t_u = llevar_a_t_comun(tiempos_5s, t_u, u)
    v, t_v = llevar_a_t_comun(tiempos_5s, t_v, v)
    du, t_du = llevar_a_t_comun(tiempos_5s, t_du, du)
    dv, t_dv = llevar_a_t_comun(tiempos_5s, t_dv, dv)
    orientacion_raw, t_orientacion = llevar_a_t_comun(tiempos_5s, t_orientacion, orientacion_raw)
    throttle_L, t_T_L = llevar_a_t_comun(tiempos_5s, t_T_L, throttle_L)
    throttle_R, t_T_R = llevar_a_t_comun(tiempos_5s, t_T_R, throttle_R)
    #print(f"len Ah = {len(Ah) }\nlen t_comun = {len(t_lat)}")
    Ah, t_Ah = llevar_a_t_comun(tiempos_5s, t_Ah, Ah, 1)
    week, t_week = llevar_a_t_comun(tiempos_5s, t_week, week)
    tow, t_tow = llevar_a_t_comun(tiempos_5s, t_tow, tow)
    utm_zone, t_utm_zone = llevar_a_t_comun(tiempos_5s, t_utm_zone, utm_zone) #tow = time on week
    mode, t_mode  = llevar_a_t_comun(tiempos_5s, t_mode, mode)
    profile_id, t_profile_id  = llevar_a_t_comun(tiempos_5s, t_profile_id, profile_id)
    static_control, t_static_control  = llevar_a_t_comun(tiempos_5s, t_static_control, static_control)
    theta = np.degrees(np.arctan2(u_raw, v_raw))

    # Vectoriza para usar con arrays
    vectorized_gps_to_datetime_local = np.vectorize(gps_to_datetime_local)

    # Aplica a tus arrays
    result_local_time = vectorized_gps_to_datetime_local(week, tow)

########## EXTRACCIÓN DATOS CSV DE LA RASP. AHORAMISMO ES UN PROXI, CUANDO ESTÉ BIEN EL EXCEL MIRAR COMO HACERLO CON EL TIEMPO DE LA MEDIDA ##########
    df = extraccion_datos_sonda(ruta_datos_sonda)
    # Asegurar que todas las series tengan la misma longitud
    N = min(
        len(t_comun),
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
        len(theta),
        len(t_week), len(week),
        len(t_tow), len(tow),
        len(t_utm_zone), len(utm_zone),
        len(t_mode), len(mode),
        len(t_profile_id), len(profile_id),
        len(t_static_control), len(static_control)
    )

    ########## GUARDAR CSV DE LOS DATOS DE NAVEGACIÓN ##########  
    #Aqui tambien se añaden datos de la sonda que se cogeran y utilizarán después. Estos datos de la sonda no se ponen luego en el geojson. En el código de QT está capado el número de columnas que se cogen del csv para el geojson, en este caso son 19 columnas las que se cogen. Se puede cambiar en la función "guardarVentanaYCsvEnJson" de código ~PprzGCS/Planificacion/python_sw/muestreo_window.cpp
    #### Aquí es donde falta adaptar y se tienen que relacionar los datos de paparazzi con los de la rasp de la sonda.
    with open(ruta_salida, "w", encoding="utf-8") as f:    f.write("fecha_utc,t_comun,x,y,lat,lon,utm_zone,u_raw,v_raw,du_raw,dv_raw,orientacion_raw,theta,throttle_L,throttle_R,Ah,mode,profile_id,static_control,Profundidad,Temperatura,pH,DO_SAT,DO,Blue,Chl\n")
        j = 0
        for i in range(N):
            if static_control[i] == 0:
                fila = [
                    result_local_time[i],
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
                    mode[i],
                    profile_id[i],
                    static_control[i],
                    "Null","Null","Null","Null","Null","Null","Null"
                ]
            else:
                fila = [
                    result_local_time[i],
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
                    mode[i],
                    profile_id[i],
                    static_control[i],
                    df.loc[j, "Profundidad"],
                    df.loc[j, "Temperatura"],
                    df.loc[j, "pH"],
                    df.loc[j, "DO_SAT"],
                    df.loc[j, "DO"],
                    df.loc[j, "Blue"],
                    df.loc[j, "Chl"]
                ]
                j+=1
            f.write(",".join(str(valor) for valor in fila) + "\n")

    ######## SE COGEN LOS DATOS DEL CSV CREANDO ANTES DE DATOS DE NAVEGACION ##########
    df_sonda = pd.read_csv(ruta_salida)
    #Ponemos cada columna como un vector
    lat_df_sonda = df_sonda["lat"].tolist()
    lon_df_sonda = df_sonda["lon"].tolist()
    x_df_sonda = df_sonda["x"].tolist()
    y_df_sonda = df_sonda["y"].tolist()
    t_df_sonda = t_x.tolist()
    static_control_df_sonda = df_sonda["static_control"].tolist()
    zona_utm_df_sonda = df_sonda["utm_zone"].tolist()
    profundidad_df_sonda = df_sonda["Profundidad"].tolist()
    temperatura_df_sonda = df_sonda["Temperatura"].tolist()
    pH_df_sonda = df_sonda["pH"].tolist()
    DO_SAT_df_sonda = df_sonda["DO_SAT"].tolist()
    DO_df_sonda = df_sonda["DO"].tolist()
    Blue_df_sonda = df_sonda["Blue"].tolist()
    Chl_df_sonda = df_sonda["Chl"].tolist()
    lat_sonda = []
    lon_sonda = []
    x_sonda = []
    y_sonda = []
    t_x_sonda_ini = []
    t_x_sonda_fin = []
    zona_utm_sonda = []
    # profundidades_i = []
    # tempteraturas_i = []
    # pH_i = []
    # DO_SAT_i = []
    # DO_i = []
    # Blue_i = []
    # Chl_i = []
    Profundidad_sonda = []
    Temperatura_sonda = []
    pH_sonda = []
    DO_SAT_sonda = []
    DO_sonda = []
    Blue_sonda = []
    Chl_sonda = []
    perfiles = []
    fecha_sonda = []
    static_control_ant = 0
    perfil = 0
    contador = 0
    for i in range(len(static_control_df_sonda)):
        if static_control_df_sonda[i] == 1 and static_control_ant == 0:
            perfil += 1
            contador += 1
            indice_inicial = i
            fecha_sonda.append(result_local_time[indice_inicial])
            lat_sonda.append(lat_df_sonda[indice_inicial])
            lon_sonda.append(lon_df_sonda[indice_inicial])
            x_sonda.append(x_df_sonda[indice_inicial])
            y_sonda.append(y_df_sonda[indice_inicial])
            t_x_sonda_ini.append(t_df_sonda[indice_inicial])
            zona_utm_sonda.append(zona_utm_df_sonda[i])
            Profundidad_sonda.append(profundidad_df_sonda[i])
            Temperatura_sonda.append(temperatura_df_sonda[i])
            pH_sonda.append(pH_df_sonda[i])
            DO_SAT_sonda.append(DO_SAT_df_sonda[i])
            DO_sonda.append(DO_df_sonda[i])
            Blue_sonda.append(Blue_df_sonda[i])
            Chl_sonda.append(Chl_df_sonda[i])
            perfiles.append(perfil)
            
        elif static_control[i] == 1 and static_control_ant == 1:
            contador += 1
            fecha_sonda.append(result_local_time[indice_inicial])
            lat_sonda.append(lat_df_sonda[indice_inicial])
            lon_sonda.append(lon_df_sonda[indice_inicial])
            x_sonda.append(x_df_sonda[indice_inicial])
            y_sonda.append(y_df_sonda[indice_inicial])
            t_x_sonda_ini.append(t_df_sonda[indice_inicial])
            zona_utm_sonda.append(zona_utm_df_sonda[i])
            Profundidad_sonda.append(profundidad_df_sonda[i])
            Temperatura_sonda.append(temperatura_df_sonda[i])
            pH_sonda.append(pH_df_sonda[i])
            DO_SAT_sonda.append(DO_SAT_df_sonda[i])
            DO_sonda.append(DO_df_sonda[i])
            Blue_sonda.append(Blue_df_sonda[i])
            Chl_sonda.append(Chl_df_sonda[i])
            perfiles.append(perfil)

        elif static_control[i] == 0 and static_control_ant == 1:
            t_fin = np.full(contador, t_df_sonda[i])
            t_x_sonda_fin.extend(t_fin)  # para añadir los valores uno a uno, igual que los otros vectores

            
            contador = 0
        static_control_ant = static_control[i]
        
    ruta_csv_sonda = os.path.join(home_dir, "PprzGCS", "Planificacion", "Extraccion_datos", "Sonda", nombre_sin_ext + "_sonda.csv")   
    
    data = {
        "fecha_utc": fecha_sonda,
        "perfil": perfiles,
        "lat": lat_sonda,
        "lon": lon_sonda,
        "x": x_sonda,
        "y": y_sonda,
        "t_ini": t_x_sonda_ini,  # Si quieres unir en una sola celda
        "t_fin": t_x_sonda_fin,
        "zona_utm": zona_utm_sonda,
        "profundidad": Profundidad_sonda,
        "temperatura": Temperatura_sonda,
        "pH": pH_sonda,
        "DO_SAT": DO_SAT_sonda,
        "DO": DO_sonda,
        "Blue": Blue_sonda,
        "Chl": Chl_sonda
    }
    
    df = pd.DataFrame(data)
    
    # Guardar a CSV
    df.to_csv(ruta_csv_sonda, index=False, encoding="utf-8", na_rep="NaN")