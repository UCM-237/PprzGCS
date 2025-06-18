#!/usr/bin/env python
# coding: utf-8

# In[1]:


def extraccion_datos(var, pos):
    home_dir = os.path.expanduser("~")
    nombre_archivo = "25_05_16__11_42_15.data"
    ruta_datos = os.path.join(home_dir, "paparazzi", "var", "logs", nombre_archivo)
    #ruta_datos = os.path.join(home_dir, "paparazzi", "var", "logs", "25_05_16__11_30_21.data")
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


# In[2]:


import os
import numpy as np
# Asume que extraccion_datos devuelve dos listas o arrays de números: valores y tiempos
u_raw, t_u = extraccion_datos("INS", 6)
v_raw, t_v = extraccion_datos("INS", 7)
du_raw, t_du = extraccion_datos("INS", 9)
dv_raw, t_dv = extraccion_datos("INS", 10)
orientacion_raw, t_orientacion = extraccion_datos("ATTITUDE", 4)
throttle_L, t_T_L = extraccion_datos("BOAT_CTRL", 7)
throttle_R, t_T_R = extraccion_datos("BOAT_CTRL", 8)

# Encuentra el número mínimo de muestras comunes para sincronizar
N = min(
    len(t_u), len(u_raw),
    len(t_v), len(v_raw),
    len(t_du), len(du_raw),
    len(t_dv), len(dv_raw),
    len(t_orientacion), len(orientacion_raw),
    len(t_T_L), len(throttle_L),
    len(t_T_R), len(throttle_R)
)

# Abrimos el archivo para escribir
with open("datos_guardados.txt", "w") as f:
    # Escribimos encabezado
    f.write("t_u,u_raw,t_v,v_raw,t_du,du_raw,t_dv,dv_raw,t_orientacion,orientacion_raw,t_T_L,throttle_L,t_T_R,throttle_R\n")
    print("N =", N)
    print("len(t_u) =", len(t_u))
    print("len(u_raw) =", len(u_raw))
# Puedes imprimir más longitudes si quieres

    # Escribimos los datos
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
        # Convierte todos los valores a string antes de unirlos
        f.write(",".join(str(valor) for valor in fila) + "\n")


# In[ ]:




