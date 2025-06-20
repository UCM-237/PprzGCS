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
    u_raw, t_u = extraccion_datos("INS", 6, ruta_datos)
    v_raw, t_v = extraccion_datos("INS", 7, ruta_datos)
    du_raw, t_du = extraccion_datos("INS", 9, ruta_datos)
    dv_raw, t_dv = extraccion_datos("INS", 10, ruta_datos)
    orientacion_raw, t_orientacion = extraccion_datos("ATTITUDE", 4, ruta_datos)
    throttle_L, t_T_L = extraccion_datos("BOAT_CTRL", 7, ruta_datos)
    throttle_R, t_T_R = extraccion_datos("BOAT_CTRL", 8, ruta_datos)

    # Asegurar que todas las series tengan la misma longitud
    N = min(
        len(t_u), len(u_raw),
        len(t_v), len(v_raw),
        len(t_du), len(du_raw),
        len(t_dv), len(dv_raw),
        len(t_orientacion), len(orientacion_raw),
        len(t_T_L), len(throttle_L),
        len(t_T_R), len(throttle_R)
    )

    # Guardar en csv
    with open(ruta_salida, "w", encoding="utf-8") as f:
        f.write("t_u,u_raw,t_v,v_raw,t_du,du_raw,t_dv,dv_raw,t_orientacion,orientacion_raw,t_T_L,throttle_L,t_T_R,throttle_R\n")
        for i in range(N):
            fila = [
                t_u[i], u_raw[i],
                t_v[i], v_raw[i],
                t_du[i], du_raw[i],
                t_dv[i], dv_raw[i],
                t_orientacion[i], orientacion_raw[i],
                t_T_L[i], throttle_L[i],
                t_T_R[i], throttle_R[i]
            ]

            f.write(",".join(str(valor) for valor in fila) + "\n")