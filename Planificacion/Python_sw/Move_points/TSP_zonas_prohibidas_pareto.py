#Basicas
import pandas as pd
import numpy as np
import matplotlib
matplotlib.use('Qt5Agg')  # Cambia el backend a Qt5Agg para mostrar ventanas gráficas
import matplotlib.pyplot as plt

#Scipy
from scipy.spatial.distance import cdist
from scipy.special import comb

#Pymoo
from pymoo.core.repair import Repair 
from pymoo.algorithms.moo.nsga2 import NSGA2
from pymoo.optimize import minimize
from pymoo.problems.single.traveling_salesman import create_random_tsp_problem
from pymoo.core.problem import ElementwiseProblem
from pymoo.operators.crossover.ox import OrderCrossover
from pymoo.operators.mutation.inversion import InversionMutation
from pymoo.config import Config

Config.warnings['not_compiled'] = False

#Varias
from shapely.geometry import Point, LineString, Polygon, MultiPoint, GeometryCollection
from lxml import etree
import xml.etree.ElementTree as ET
from xml.dom import minidom
from pyproj import CRS, Transformer
import os


home_dir = os.path.expanduser("~")

#Leemos los archivos sacados del boton de planificación
ruta_datos = os.path.join(home_dir, "PprzGCS", "Planificacion", "datos.txt")
Estrategia = pd.read_csv(ruta_datos, delimiter=':', header = None) #Datos.txt

Estrategia=pd.DataFrame(Estrategia)

#Carga de archivo XML
TipoTrayectoria = Estrategia.iloc[0,1]
Archivo = Estrategia.iloc[1,1]
Controlador = Estrategia.iloc[2,1]
Conf = Estrategia.iloc[3,1]
Aircraft = Estrategia.iloc[4,1]

ruta_archivo = os.path.join(home_dir, "paparazzi", "conf", "flight_plans", f"{Archivo}")
tree = ET.parse(ruta_archivo)
root = tree.getroot()

# Ruta a tu archivo XML y DTD
xml_file = ruta_archivo
ruta_dtd = os.path.join(home_dir, "paparazzi", "conf", "flight_plans", "flight_plan.dtd")
dtd_file = ruta_dtd

# Expande la ruta
dtd_file = os.path.expanduser(dtd_file)

# Cargar el DTD
with open(dtd_file, 'r') as dtd_f:
    dtd = etree.DTD(dtd_f)

# Expande la ruta
xml_file = os.path.expanduser(xml_file)

# Cargar el archivo XML
with open(xml_file, 'r') as xml_f:
    xml_content = xml_f.read()

# Validar el XML contra el DTD
try:
    xml_doc = etree.fromstring(xml_content)
    if not dtd.validate(xml_doc):
        print("El archivo XML no es válido.")
        print("Errores:")
        for error in dtd.error_log:
            print(f"Línea {error.line}: {error.message}")
except etree.XMLSyntaxError as e:
    print("Error de sintaxis XML:")
    print(e)


lat0 = root.attrib.get('lat0')
lon0 = root.attrib.get('lon0')

# Encontrar todos los waypoints
way_points = root.find('waypoints')
waypoint_list = []
unchanged_points = []
home = []

#HABRÍA QUE COMENTAR Y DESCOMENTAR DEPENDIENDO DE SI LOS PUNTOS DE PASO SE DAN EN X,Y O EN LON,LAT
# Almacenar los waypoints existentes en una lista
for waypoint in way_points.findall('waypoint'):
    name = waypoint.get('name')
    if name.startswith('BZ'):
        #lat = float(waypoint.get('lat'))
        #lon = float(waypoint.get('lon'))
        x = float(waypoint.get('x'))
        y = float(waypoint.get('y'))
        waypoint_list.append((name, x, y))
        #waypoint_list.append((name, lat, lon))
    else:
        x = float(waypoint.get('x'))
        y = float(waypoint.get('y'))
        unchanged_points.append((name, x, y))
        #waypoint_list.append((name, lon, lat))
    if name == "HOME":
        home.append((name, x, y))
        waypoint_list.append((name, x, y))


sectors = {}
zonas_prohibidas = {}
sectors_names = []
zonas_prohibidas_names = []
points_in_sectors = []
sectores_navegacion=0

sectors_node = root.find('sectors')
if sectors_node is not None:
    for sector in sectors_node:
        sector_name = sector.attrib['name'] 
        sectors_names.append(sector_name)
        if not sector_name.startswith("Net") and not sector_name.startswith("Zona_prohibida"):
            sectores_navegacion+=1
        points_in_sector = sector_name.split('_')[-1]  # Esto obtiene la parte después del _
        points_in_sectors.append(points_in_sector)
        corners = []
        for corner in sector.findall('corner'):
            corner_name = corner.attrib['name']
            
            # Comprobar si corner_name está en unchanged_points
            for name, x, y in unchanged_points:
                if corner_name == name:
                    corners.append((x, y))
                    break  # Salir del bucle una vez que se encuentra el punto
    
        sectors[sector_name] = corners
        if sector_name.startswith("Zona_prohibida"):
            zonas_prohibidas[sector_name] = corners
            zonas_prohibidas_names.append(sector_name)


#Limpiamos way_points
way_points.clear()

#Convertimos tanto la lista de waypoints como unchanged en una lista y las unimos
waypoints = np.array([[nombre, lat, lon] for nombre, lat, lon in waypoint_list])
unchangedpoints = np.array([[nombre, x, y] for nombre, x, y in unchanged_points])
sectors_points=[]
if sectores_navegacion > 0:
    for sector_name, coords in sectors.items():
        for coord in coords:
            sectors_points.append((sector_name, coord[0], coord[1]))
    sectors_array = np.array(sectors_points, dtype=object)
points = np.concatenate((waypoints, unchangedpoints), axis=0)
if sectores_navegacion > 0:
    unchanged_points = np.concatenate((unchanged_points, sectors_points), axis = 0)

num_zonas_prohibidas = sum(1 for key in sectors if "Zona_prohibida" in key)


#Ahora hay que calcular el centroide de cada sector y añadirlo a las paradas
def calcular_centroide(puntos):
    n = len(puntos)  # Número de vértices
    C_x = sum(x for x, y in puntos) / n  # Promedio de las coordenadas x
    C_y = sum(y for x, y in puntos) / n  # Promedio de las coordenadas y
    return C_x, C_y  # Devuelve el centroide como una tupla

#Definimos una matriz unicamente con las coordenadas de cada parada:
paradas = np.array(waypoints[:,-2:])

#Definimos parámetros del barco
velocidad_media = 40
if sectores_navegacion > 0:
    centroides = []
    for i in range(len(sectors_names)):
        if not sectors_names[i].startswith("Net") and not sectors_names[i].startswith("Zona_prohibida"):
            C_x, C_y = calcular_centroide(sectors[sectors_names[i]])
            C_i = [C_x, C_y]
            centroides.append(C_i)
    centroides = np.array(centroides)
    paradas=np.vstack((paradas, centroides))


#Si solo se quiere fijar el inicial
# class StartFromZeroRepair(Repair):

#     def _do(self, problem, X, **kwargs):
#         I = np.where(X == 0)[1]

#         for k in range(len(X)):
#             i = I[k]
#             X[k] = np.concatenate([X[k, i:], X[k, :i]])

#         return X

#Para fijar inicial y final: Básicamente pone el punto que queremos como de llegada al final y luego elimina 
#El 0 del vector (nuestro punto de partida) y lo pone al principio.
class StartFromZeroRepair(Repair):

    def _do(self, problem, X, **kwargs):
        if sectores_navegacion > 0:
            ult_punto = len(stops) - len(centroides)-1
        else:
            ult_punto = len(stops)-1
        # Encontramos el índice del último valor que debe ser el correspondiente al último punto en centroides
        I = np.where(X == ult_punto)[1] #Quiero mover el último punto antes de que empicen los centroides
        for k in range(len(X)):
            i = I[k]+1
            # Reorganizamos los puntos para que el último valor corresponda al punto que está en centroides[-1]
            X[k] = np.concatenate([X[k, i:], X[k, :i]])
            indice_0 = np.where (X[k] == 0)[0]

            X_del = np.delete(X[k], indice_0)
            X[k] = np.insert(X_del, 0, 0)
        return X

def calcular_intersecciones(p1, p2, poligono_vertices):
    """
    Calcula los puntos de intersección entre un segmento (p1, p2) y un polígono definido por sus vértices.
    """
    segmento = LineString([p1, p2])
    poligono = Polygon(poligono_vertices)  # Convertimos la lista de vértices en un objeto Polygon

    interseccion = poligono.boundary.intersection(segmento)

    if interseccion.is_empty:
        return []
    
    if interseccion.geom_type == "MultiPoint":
        return [list(p.coords)[0] for p in interseccion.geoms]
    elif interseccion.geom_type == "Point":
        return [list(interseccion.coords)[0]]
    
    return []


#Definimos una función para ver si el segmento pasa por una zona prohibida.
def segmento_atraviesa_poligono(p1, p2, vertices_poligono):
    segmento = LineString([p1, p2])
    poligono = Polygon(vertices_poligono)
    
    return segmento.intersects(poligono)
    
class RUTA (ElementwiseProblem):

    def __init__(self, **kwargs):

        n_stops, _ = stops.shape #Aquí coge las filas (Por tanto el número de paradas)
        self.stops = stops
        self.D = D
        super(RUTA, self).__init__(
            n_var=n_stops,
            n_obj=2,
            n_constr=0,
            xl=0,
            xu=n_stops,
            vtype=int,
            **kwargs
        )

    def _evaluate(self, x, out, *args, **kwargs):
        zonas_prohibidas_penalizacion = 0
        distancia = self.get_route_length(x)
        tiempo = distancia/velocidad_media
        for i in range(num_zonas_prohibidas):
            for j in range(len(x) - 1):
                intersecciones = calcular_intersecciones(waypoint_list[x[j]], waypoint_list[x[j+1]], zonas_prohibidas[zonas_prohibidas_names[i]])
                if len(intersecciones) >= 2:  # Se cruza la zona prohibida
                    dist_entrada_salida = np.linalg.norm(np.array(intersecciones[0]) - np.array(intersecciones[-1]))
                    zonas_prohibidas_penalizacion += dist_entrada_salida * 1000  # F
        out['F'] = [distancia, zonas_prohibidas_penalizacion]
        
    def get_route_length(self, x):
        n_stops = len(x)
        dist = 0
        for k in range(len(x)-1):
            i, j = x[k], x[k + 1]
            dist += self.D[i, j]
        return dist


def visualize_3(problem, x, sectors, j, fig=None, ax=None, show=True, label=True):

    x = x[j]
    if fig is None or ax is None:
        fig, ax = plt.subplots()
    
    # plot cities using scatter plot
    ax.scatter(problem.stops[:, 0], problem.stops[:, 1], color="black", s=150, label = "Waypoints")
    if label:
        for i, c in enumerate(problem.stops):
            ax.annotate(str(i), xy=c, fontsize=10, ha="center", va="center", color="white")
    ax.plot(problem.stops[:,0], problem.stops[:, 1], 'black', label="Final route")

    # Ploteamos los sectores
    for sector_name, points in sectors.items():
        sector_points = np.array(points)
        color = "red" if sector_name.startswith("Zona_prohibida") else "blue"
        if not sector_name.startswith("Net"):
            ax.fill(sector_points[:, 0], sector_points[:, 1], alpha=0.2, color=color, label=f'Sector {sector_name}')
        
    #fig.suptitle(f"Route length: {problem.get_route_length(x)}km \nRoute time: {problem.get_route_length(x)*60/velocidad_media}min")
    fig.suptitle("Final route PtP")
    
    ax.legend()
    plt.legend(loc="upper right")
    if show:
        ax.grid(True)
        plt.show()  # Muestra el gráfico en una ventana emergente


def closest_point_and_reorder(home, points):

    # Convertir las coordenadas de los puntos a un numpy array y trabajar solo con las coordenadas (últimas 2 columnas)
    coords = np.array(points[:, -2:], dtype=float)
    
    # Convertir el punto fijo a un array numpy
    home = np.array(home, dtype=float)
    
    # Calcular las distancias euclidianas entre el punto fijo y cada punto de 'coords'
    distances = np.linalg.norm(coords - home, axis=1)
    
    # Encontrar el índice del punto con la menor distancia
    closest_index = np.argmin(distances)
    
    # Reordenar los puntos colocando el más cercano en la primera posición
    reordered_points = np.vstack([points[closest_index], np.delete(points, closest_index, axis=0)])
    
    return reordered_points, closest_index
        
home_coord = home[0][1:]
waypoint_list = [(x, y) for (_,x, y) in waypoint_list]
waypoint_list, closest_index = closest_point_and_reorder(home_coord, paradas)
paradas = waypoint_list

#Creamos un diccionario para almacenar cada ruta
rutas={}
waypoint_list = np.array(waypoint_list, dtype=object)
stops = waypoint_list.astype(float)
fitness=[]

grupos_finales=np.array([stops[:, 0], stops[:, 1], stops[:,1]])
grupos_finales=grupos_finales.T
rutas["ruta_1"]=grupos_finales[grupos_finales[:,2]==1][:,:2]

#Calculamos matriz de distancias
D = cdist(stops, stops)


from pymoo.core.sampling import Sampling

n_stops=len(stops)
problem = RUTA()

    
class CustomSampling(Sampling):
    def _do(self, problem, n_samples, **kwargs):
        # Generar n_samples rutas aleatorias
        samples = []
        for _ in range(n_samples):
            route = np.random.permutation(n_stops)
            samples.append(route)
        return np.array(samples)

sampling=CustomSampling()

algorithm = NSGA2(
    pop_size=20,
    sampling=sampling,
    mutation=InversionMutation(),
    crossover=OrderCrossover(),
    repair=StartFromZeroRepair(),
    eliminate_duplicates=True,
    save_history=True,
)


res = minimize(
    problem,
    algorithm,
    termination=('n_gen',100),
    seed=1,
    verbose=False,
)

###############################PARA VISUALIZAR TODAS LAS SOLUCIONES DEL FRENTE############################################

# if sectores_navegacion == 0 and num_zonas_prohibidas == 0 and TipoTrayectoria == "Point to point": 
#     print("Ploteando con visualize")
#     visualize_3(problem, res.X, sectors, j=0)
#########################################################################################################################

resultado_rutas = []
for j in range(len(res.X)):
    fila = []  # Crear una nueva lista para cada fila de resultados
    for i in range(len(res.X[j])):
        idx = res.X[j][i]
        fila.append(paradas[idx])  # Agregar cada parada a la fila
    resultado_rutas.append(np.array(fila))  # Almacenar la fila como un array independiente

# Crear una cuadrícula dentro del área
def generar_waypoints_area(polygon, centroide, vertices, Pnts_total, x_inicio, y_inicio, x_fin, y_fin, estrategia):

    min_x, min_y, max_x, max_y = polygon.bounds
    waypoints = []
    punto_final=(x_fin, y_fin)
    if estrategia.startswith("ZigZag"):  
        x_paso = np.abs(min_x-max_x)/(np.sqrt(Pnts_total)+1)
        y_paso = np.abs(min_y - max_y)/(np.sqrt(Pnts_total)+1)
        Pnts_total=(Pnts_total)
        y_vals = np.arange(min_y, max_y, y_paso)
        x_vals = np.arange(min_x, max_x, x_paso)
        area=polygon.area

        centroide_i = Point(centroide[0], centroide[1])
    
        # Calcular las distancias a los vértices
        dists = []
        for i in range(len(vertices)):
            vertice = np.array(vertices[i], dtype=float)
            dist = np.linalg.norm(vertice - np.array((float(x_inicio), float(y_inicio))))
            dists.append(dist)
        
        # Obtener el punto más cercano
        min_index = np.argmin(dists)
        closest_point = tuple(map(float, vertices[min_index]))  # Asegúrate de que es una tupla de flotantes
        
        punto_final = (float(x_fin), float(y_fin))
        punto_inicial=(float(x_ini),float(y_ini))
        # Controlar el orden de los puntos
        #Vamos a hacer que la entrada controle el eje x y la salida el eje y
        if punto_final[1] < (min_y):
            y_vals = np.flip(y_vals)
        if punto_final[0] < (max_x):
            x_vals = np.flip(x_vals)

        
        # Generar waypoints dentro del polígono
        cont_aux=0
        for y in y_vals:
            for x in x_vals:
                point = Point(x, y)
                if polygon.contains(point):
                    if cont_aux < Pnts_total:
                        waypoints.append((float(x), float(y)))  # Asegurarte de que son flotantes   
                    cont_aux+=1

    if estrategia.startswith("Espiral"):
        
        centroide_i = Point(centroide[0], centroide[1])
        
        mediatrices = [Point((vertices[i][0] + vertices[i+1][0]) / 2, (vertices[i][1] + vertices[i+1][1]) / 2)
        for i in range(len(vertices) - 1)]

        distancia_mediatrices = [centroide_i.distance(punto_medio) for punto_medio in mediatrices]

        radio_maximo = max(distancia_mediatrices)
        radio_minimo = min(distancia_mediatrices)

        radio_inicial = radio_maximo*0.2
        
        #Añadir un warning de que si no caben los puntos introducidos por el usuario el algoritmo va a recorrer la region con menos puntos

        #######################################OPCIÓN CON SEPARACIÓN FIJA DMIN POR VUELTA#####################################################
        #D_min = 0.05 #Tamaño para que el barco pueda moverse correctamente/error de la sonda
        #vueltas_totales = (radio_minimo-radio_inicial)/D_min 
        #print("Total de puntos", Pnts_total)
        #Pnts_total = Pnts_total
        #Pnts_1vuelta = Pnts_total/vueltas_totales
        #incremento_radio = D_min/(Pnts_1vuelta)
        ######################################################################################################################################

        ######################################OPCIÓN CON SEPARACIÓN DMIN EN LA PRIMERA VUELTA#################################################
        D_min = radio_minimo/Pnts_total #Tamaño para que el barco pueda moverse correctamente/error de la sonda
        Pnts_1vuelta = 2*np.pi*radio_inicial*0.35/D_min #Ponemos como ajuste de error el 50% del radio inicial
        vueltas_totales = Pnts_total/Pnts_1vuelta
        incremento_radio = (radio_minimo/vueltas_totales)/(Pnts_1vuelta)
        ######################################################################################################################################

        # Asegurarse de que los puntos de inicio son flotantes
        punto_final = (float(x_fin), float(y_fin))
        punto_inicial=(float(x_ini),float(y_ini))
        
        pasos = Pnts_total
        pasos = int(pasos)
        
        theta = np.linspace(0, 2 * vueltas_totales * np.pi, pasos)  # Genera ángulos para la espiral
        radio = np.linspace(radio_inicial, radio_inicial + incremento_radio * pasos, pasos)
        signo = 1
        
        if float(x_fin) > float(max_x/2):
            signo = -1
            
        for r, t in zip(radio, theta):
            x = centroide[0] + signo*r * np.cos(t)
            y = centroide[1] + signo*r * np.sin(t)
            punto = Point(x, y)
            if polygon.contains(punto):  # Solo añade el punto si está dentro del polígono
                waypoints.append((float(x), float(y)))

    return waypoints


def encontrar_centroide_idx(centroides, centroide_objetivo):
    for i, c in enumerate(centroides):
        if np.allclose(c, centroide_objetivo, atol=1e-6):
            return i
    return -1  # No encontrado


resultados_sectores_rutas = []

for j in range(len(resultado_rutas)):
    resultado = [list(map(float, punto)) for punto in resultado_rutas[j]]
    Matriz_pnts_añadidos = np.empty((0, 2))
    centroides_undefined=0
    resultado = [list(map(float, punto)) for punto in resultado]
    # Suponiendo que tienes un bucle que llama a esta función con los parámetros adecuados
    for i in range(len(sectors_names)):
        if sectors_names[i].startswith("Net") or sectors_names[i].startswith("Zona_prohibida"):
            centroides_undefined+=1
        elif not sectors_names[i].startswith("Net") and not sectors_names[i].startswith("Zona_prohibida"):
            poligono = Polygon(sectors[sectors_names[i]])
            vertices = sectors[sectors_names[i]]
            area = poligono.area
            
            Estrategia_recorrido=sectors_names[i]
            n_puntos=int(points_in_sectors[i])
            vertices = sectors[sectors_names[i]]
            # Obtener coordenadas iniciales y finales
            pos_centroide = np.where((centroides == centroides[i-centroides_undefined]).all(axis=1))[0]
            sector_indice = (np.where(res.X[j] == len(paradas) - len(centroides) + pos_centroide))[0]
            punto_inicial = res.X[j][sector_indice[0] - 1]
            punto_final = res.X[j][sector_indice[0] + 1]
            x_ini = paradas[punto_inicial, 0]
            y_ini = paradas[punto_inicial, 1]
            x_fin = paradas[punto_final, 0]
            y_fin = paradas[punto_final, 1]
            
            waypoints = generar_waypoints_area(poligono, centroides[i-centroides_undefined], vertices, n_puntos, x_ini, y_ini, x_fin, y_fin, Estrategia_recorrido)
            
            #La estrategia para ver cuantos puntos hemos añadido en el proceso es la siguiente:
                #Guardo en una matriz la posicion del centroide en el vector resultado junto con los pnts añadidos
                #Miro cuantos centroides he modificado con un ínidce menor que el centroide que estoy sustituyendo en esta iteración
                #La suma será el número de puntos que he añadido antes de este centroide
            Matriz_filtrada = Matriz_pnts_añadidos[Matriz_pnts_añadidos[:,0] < sector_indice[0]]
            puntos_añadidos = int(np.sum(Matriz_filtrada[:,1]))
            if Estrategia_recorrido.startswith("ZigZag"):
                Punto_borrado=resultado[sector_indice[0] + puntos_añadidos]
                resultado = np.delete(resultado, sector_indice[0] + puntos_añadidos, axis = 0)
                resultado = np.insert(resultado, sector_indice[0] + puntos_añadidos, waypoints, axis = 0)
            elif Estrategia_recorrido.startswith("Espiral"):
                Punto_borrado=resultado[sector_indice[0] + puntos_añadidos]
                resultado = np.delete(resultado, sector_indice[0] + puntos_añadidos, axis = 0)
                resultado = np.insert(resultado, sector_indice[0] + puntos_añadidos, waypoints, axis = 0)
            long_añadidos = len(waypoints)-1          
            Matriz_pnts_añadidos = np.append(Matriz_pnts_añadidos, [[sector_indice[0], long_añadidos]], axis = 0)
            
            #Dibujar el área y los waypoints
            x_coords, y_coords = zip(*waypoints)
    resultados_sectores_rutas.append(resultado)
    resultado = np.vstack(resultado)


def get_bezier_parameters(X, Y, smooth_factor, degree=12):
    """ Least square qbezier fit using penrose pseudoinverse.

    Parameters:

    X: array of x data.
    Y: array of y data. Y[0] is the y point for X[0].
    degree: degree of the Bézier curve. 2 for quadratic, 3 for cubic.

    Based on https://stackoverflow.com/questions/12643079/b%C3%A9zier-curve-fitting-with-scipy
    and probably on the 1998 thesis by Tim Andrew Pastva, "Bézier Curve Fitting".
    """
    if degree < 1:
        raise ValueError('degree must be 1 or greater.')

    if len(X) != len(Y):
        raise ValueError('X and Y must be of the same length.')

    def bpoly(n, t, k):
        """ Bernstein polynomial when a = 0 and b = 1. """
        return t ** k * (1 - t) ** (n - k) * comb(n, k)

    def bmatrix(T):
        """ Bernstein matrix for Bézier curves. """
        return np.matrix([[bpoly(degree, t, k) for k in range(degree + 1)] for t in T])

    def least_square_fit(points, M, smooth_factor):
        M_ = np.linalg.pinv(M)
        
        smooth_matrix = np.eye(M.shape[1])
        for i in range(1, M.shape[1]):
            smooth_matrix[i, i - 1] = -1  # Penaliza diferencias grandes entre puntos de control adyacentes

        # Combine the original fitting problem with the smoothness constraint
        augmented_matrix = np.vstack([M, smooth_factor * smooth_matrix])
        augmented_points = np.vstack([points, np.zeros((smooth_matrix.shape[0], points.shape[1]))])
        
        return np.linalg.pinv(augmented_matrix) @ augmented_points

    T = np.linspace(0, 1, len(X))
    M = bmatrix(T)
    points = np.array(list(zip(X, Y)))
    
    final = least_square_fit(points, M, smooth_factor).tolist()
    final[0] = [X[0], Y[0]]
    final[len(final)-1] = [X[len(X)-1], Y[len(Y)-1]]
    return final

def bernstein_poly(i, n, t):
    """
     The Bernstein polynomial of n, i as a function of t
    """
    return comb(n, i) * ( t**(n-i) ) * (1 - t)**i


def bezier_curve(points, nTimes=1000):
    """
       Given a set of control points, return the
       bezier curve defined by the control points.

       points should be a list of lists, or list of tuples
       such as [ [1,1], 
                 [2,3], 
                 [4,5], ..[Xn, Yn] ]
        nTimes is the number of time steps, defaults to 1000

        See http://processingjs.nihongoresources.com/bezierinfo/
    """

    nPoints = len(points)
    xPoints = np.array([p[0] for p in points])
    yPoints = np.array([p[1] for p in points])

    t = np.linspace(0.0, 1.0, nTimes)

    polynomial_array = np.array([ bernstein_poly(i, nPoints-1, t) for i in range(0, nPoints)   ])

    xvals = np.dot(xPoints, polynomial_array)
    yvals = np.dot(yPoints, polynomial_array)

    return xvals, yvals

def detecta_cruce_ruta(ruta, zonas_prohibidas, zonas_prohibidas_names, coordenadas, estrategia):
    """
    Detecta todos los cruces de la ruta con zonas prohibidas.
    Devuelve una lista de tuplas (índice de cruce, índice de zona).
    """
    if estrategia == "Point to point":
        cruces = []
        for i in range(len(ruta) - 1):
            p1, p2 = coordenadas[i], coordenadas[i + 1]
            for idx, zona in enumerate(zonas_prohibidas_names):
                if segmento_atraviesa_poligono(p1, p2, zonas_prohibidas[zona]):
                    cruces.append((i, idx))  # Añadir todos los cruces encontrados
    elif estrategia == "Continuous":
        cruces = []
        xpoints = coordenadas[:,0]
        ypoints = coordenadas[:,1]
        Puntos_paso = list(zip(xpoints, ypoints))
        data = get_bezier_parameters(xpoints, ypoints, 0.005, degree=len(xpoints)*2-2)
        xvals, yvals = bezier_curve(data, nTimes=1000)
        curve = LineString(np.column_stack((xvals, yvals)))
        coords = list(curve.coords)[::-1]
        cruces_bz = []
        for idx, name in enumerate(zonas_prohibidas_names):
            zona_prohibida_poligono = Polygon(zonas_prohibidas[name])
            for i in range(len(coords) - 1):
                segmento = LineString([coords[i], coords[i + 1]])
                if segmento.intersects(zona_prohibida_poligono):
                     cruces_bz.append((i, name))
            if cruces_bz:
                i_start = cruces_bz[0][0]
                i_end = cruces_bz[-1][0] + 1
                # Map Puntos_paso to closest coords index
                map_p_to_c = []
                for p_idx, p_waypoint in enumerate(Puntos_paso):
                    min_dist = float('inf')
                    closest_c_idx = -1
                    for c_idx, c_point in enumerate(coords):
                        dist = np.linalg.norm(np.array(p_waypoint) - np.array(c_point))
                        if dist < min_dist:
                            min_dist = dist
                            closest_c_idx = c_idx
                    map_p_to_c.append(closest_c_idx)

                # Find idx_p_start: the last waypoint whose corresponding Bezier point is before the entry point
                idx_p_start = 0
                for j in range(len(map_p_to_c)):
                    if map_p_to_c[j] < i_start:
                        idx_p_start = j
                    else:
                        break

                # Find idx_p_end: the first waypoint whose corresponding Bezier point is after the exit point
                idx_p_end = len(Puntos_paso) - 1
                for k in range(len(map_p_to_c) - 1, -1, -1):
                    if map_p_to_c[k] > i_end:
                        idx_p_end = k
                    else:
                        break

                # Ensure idx_p_end is at least idx_p_start + 1 to form a valid segment
                if idx_p_end <= idx_p_start:
                    idx_p_end = idx_p_start + 1
                    # If idx_p_end goes beyond the last waypoint, adjust idx_p_start back
                    if idx_p_end >= len(Puntos_paso):
                        idx_p_end = len(Puntos_paso) - 1
                        if idx_p_end > 0: # Ensure idx_p_start is not negative
                            idx_p_start = idx_p_end - 1
                        else: # Only one waypoint, cannot form a segment
                            idx_p_start = 0
                            idx_p_end = 0 # This might need further refinement based on desired behavior for single point crossings


                if idx_p_start is None or idx_p_end is None:
                    print("No se encontraron puntos de paso en la curva antes o después del punto de cruce.")
                

                cruces.append((idx_p_start, idx))
                            
    return cruces

def rodear_zona_prohibida(ruta, zonas_prohibidas, zonas_prohibidas_names, coordenadas):
    """
    Modifica la ruta para evitar cruzar zonas prohibidas.
    Si la ruta cruza una zona prohibida, se inserta un punto de desvío.
    """
    n_zonas_prohibidas_cruzadas = 0
    cruces = detecta_cruce_ruta(ruta, zonas_prohibidas, zonas_prohibidas_names, coordenadas, TipoTrayectoria)
    if cruces != []:
        puntos_extra = 0
        puntos_añadidos = 0
        flag = 0
        for i in range(len(cruces)):

            if flag == 0:
                cruce_idx = cruces[i][0]
                zona_idx = cruces[i][1]
                nueva_ruta = ruta
                punto_de_rodeo = calcular_punto_rodeo(coordenadas[cruce_idx+puntos_añadidos], coordenadas[cruce_idx+1+puntos_añadidos], zonas_prohibidas[zonas_prohibidas_names[zona_idx]], zonas_prohibidas, zonas_prohibidas_names, coordenadas = coordenadas)
                if punto_de_rodeo != []:
                    # Si punto_de_rodeo es un solo punto, asegúrate de que sea un array 2D de forma (1, 2)
                    if isinstance(punto_de_rodeo, (np.ndarray, tuple)) and punto_de_rodeo.ndim == 1:
                        punto_de_rodeo_np_array = np.array([float(punto_de_rodeo[0]), float(punto_de_rodeo[1])]).reshape(1, 2)
                        puntos_extra = 1
                    elif isinstance(punto_de_rodeo, list):  # Si es una lista de puntos
                        punto_de_rodeo_np_array = np.array([list(punto) for punto in punto_de_rodeo], dtype=float)
                        puntos_extra = len(punto_de_rodeo_np_array)
                    else:
                        raise ValueError("El formato de 'punto_de_rodeo' no es reconocido.")
                    for i in range(puntos_extra):
                        nueva_ruta = np.append(nueva_ruta, len(ruta)+i)
                        puntos_añadidos += 1
                    coordenadas = np.insert(coordenadas, cruce_idx+1, punto_de_rodeo_np_array, axis = 0)
                    # Convertimos `problem_stops` a lista de listas
                    problem_stops_list = problem.stops.tolist()
                
                    problem_stops_list = [[float(coord[0]), float(coord[1])] if isinstance(coord, np.ndarray) else coord for coord in problem_stops_list]
                    # Añadimos el punto de rodeo como una nueva fila de coordenadas en la lista
                    problem_stops_list.extend(punto_de_rodeo_np_array)
                    # Convertimos de nuevo la lista a `numpy.ndarray` para mantener el formato
                    problem_stops = np.array(problem_stops_list)
                    problem.stops = problem_stops
                    
                    ruta=nueva_ruta
                    n_zonas_prohibidas_cruzadas += 1
                    cruces_i = detecta_cruce_ruta(nueva_ruta, zonas_prohibidas, zonas_prohibidas_names, coordenadas, TipoTrayectoria)
                    if cruces_i == []:    
                        flag = 1
            else:
                nueva_ruta = ruta
                
            cruces_final = detecta_cruce_ruta(nueva_ruta, zonas_prohibidas, zonas_prohibidas_names, coordenadas, TipoTrayectoria)
        
            if cruces_final != []:   
                return rodear_zona_prohibida(nueva_ruta, zonas_prohibidas, zonas_prohibidas_names, coordenadas)  
            return nueva_ruta, coordenadas
    else:
        return ruta, coordenadas
    
from shapely.geometry import LineString, Polygon, Point
import numpy as np

def desplazar_punto(punto, direccion, distancia):
    """Desplaza un punto en la dirección dada por una distancia especificada."""
    return (punto[0] + direccion[0] * distancia, punto[1] + direccion[1] * distancia)
    
def calcular_distancia_ruta(coordenadas):
    distancia_total = 0
    
    # Iterar sobre los puntos consecutivos
    for i in range(len(coordenadas) - 1):
        # Convertir puntos en arrays de NumPy
        punto1 = np.array(coordenadas[i])
        punto2 = np.array(coordenadas[i + 1])
        
        # Calcular la distancia euclidiana entre los puntos
        distancia_total += np.linalg.norm(punto2 - punto1)
    
    return distancia_total



def punto_valido(punto, zonas_prohibidas, zonas_prohibidas_names):
    """
    Verifica que el punto no esté dentro ni toque ninguna zona prohibida (objetos Polygon).
    """
    punto_geom = Point(punto)
    for idx, zona in enumerate(zonas_prohibidas_names):
        poligono = Polygon(zonas_prohibidas[zona])
        if poligono.contains(punto_geom) or poligono.touches(punto_geom):
            return False         
    return True


def calcular_punto_rodeo(punto_inicial, punto_final, vertices_zona_prohibida, zonas_prohibidas, zonas_prohibidas_names, coordenadas, margen=50):
    """
    Encuentra el primer punto con visibilidad para rodear la zona prohibida.

    :param punto_inicial: Coordenadas (x, y) del punto inicial.
    :param punto_final: Coordenadas (x, y) del punto final.
    :param vertices_zona_prohibida: Lista de vértices [(x1, y1), (x2, y2), ...] de la zona prohibida.
    :param margen: Distancia de separación del polígono para el punto de rodeo.
    :return: Punto de rodeo (x, y).
    """
    puntos_ajustados_horarios=[]
    puntos_ajustados_antihorarios=[]
    ruta_horario=[]
    ruta_antihorario=[]
    ruta_ajustada_horario=[]
    ruta_ajustada_antihorario=[]
    poligono = Polygon(vertices_zona_prohibida)
    linea = LineString([punto_inicial, punto_final])
    # 1. Encontrar el punto de intersección con la zona prohibida
    if TipoTrayectoria == "Point to point":
        interseccion = poligono.boundary.intersection(linea)
    if TipoTrayectoria == "Continuous":
        xpoints = coordenadas [:,0]
        ypoints = coordenadas [:,1]
        data = get_bezier_parameters(xpoints, ypoints, 0.005, degree=len(xpoints)*2)
        xvals, yvals = bezier_curve(data, nTimes=1000)
        curve = LineString(np.column_stack((xvals, yvals)))
        coords = list(curve.coords)[::-1]
        interseccion_bz = []
        interseccion = []
 
        for i in range(len(coords) - 1):
            segmento = LineString([coords[i], coords[i + 1]])
            if segmento.intersects(poligono):
                interseccion_i = segmento.intersection(poligono)
                interseccion_bz.append(interseccion_i)
        # Extraemos todos los puntos individuales de las geometrías de intersección
        puntos_interseccion = []
        for geom in interseccion_bz:
            if geom.geom_type == "Point":
                puntos_interseccion.append(geom)
            elif geom.geom_type == "MultiPoint":
                puntos_interseccion.extend(list(geom.geoms))
            elif geom.geom_type == "LineString":
                puntos_interseccion.append(Point(geom.coords[0]))
                puntos_interseccion.append(Point(geom.coords[-1]))
            elif geom.geom_type == "GeometryCollection":
                for g in geom.geoms:
                    if g.geom_type == "Point":
                        puntos_interseccion.append(g)
                    elif g.geom_type == "LineString":
                        puntos_interseccion.append(Point(g.coords[0]))
                        puntos_interseccion.append(Point(g.coords[-1]))

        # Ordenar los puntos según su aparición en la curva
        # Asumimos que `curve` es la trayectoria original, y comparamos la distancia al inicio
        puntos_interseccion.sort(key=lambda p: curve.project(p))

        # Tomamos primer y último punto de intersección
        if len(puntos_interseccion) >= 2:
            interseccion = MultiPoint([puntos_interseccion[0], puntos_interseccion[-1]])
        elif len(puntos_interseccion) == 1:
            interseccion = puntos_interseccion[0]
        else:
            interseccion = GeometryCollection()
    if interseccion.is_empty:
        return []  # No hay intersección, camino directo

    if interseccion.geom_type == "MultiPoint":
        interseccion = min(interseccion.geoms, key=lambda p: Point(punto_inicial).distance(p))

    # 2. Calcular el centroide del polígono
    centroide = poligono.centroid
    centroide_coords = np.array([centroide.x, centroide.y])

    # 3. Buscar el primer vértice con visibilidad desde punto_final recorriendo ambos sentidos del polígono
    vertices = list(poligono.exterior.coords[:-1])
    idx_interseccion = min(range(len(vertices)), key=lambda i: Point(vertices[i]).distance(Point(punto_inicial)))
    i = 0
    while i < len(vertices):
        idx_horario = (idx_interseccion - i) % len(vertices)
        idx_antihorario = (idx_interseccion + i) % len(vertices)
        
        punto_candidato_horario = np.array(vertices[idx_horario])
        punto_candidato_antihorario = np.array(vertices[idx_antihorario])
        
        # 4. Determinar la dirección desde el vértice hacia el centroide
        direccion_normal_horario = punto_candidato_horario - centroide_coords
        direccion_normal_antihorario = punto_candidato_antihorario - centroide_coords
        
        direccion_normal_horario /= np.linalg.norm(direccion_normal_horario)  # Normalizar
        direccion_normal_antihorario /= np.linalg.norm(direccion_normal_antihorario)
        
        # 5. Desplazar el punto en la dirección contraria al centroide
        punto_ajustado_horario = desplazar_punto(punto_candidato_horario, direccion_normal_horario, margen)
        punto_ajustado_antihorario = desplazar_punto(punto_candidato_antihorario, direccion_normal_antihorario, margen)

        # Si no se encuentra un punto válido, continuar al siguiente vértice
        i += 1
        if punto_valido(punto_ajustado_horario, zonas_prohibidas, zonas_prohibidas_names):
            puntos_ajustados_horarios.append(punto_ajustado_horario)
        if punto_valido(punto_ajustado_antihorario, zonas_prohibidas, zonas_prohibidas_names):
            puntos_ajustados_antihorarios.append(punto_ajustado_antihorario)

    for i in range(len(puntos_ajustados_horarios)):
        ruta_horario.append(puntos_ajustados_horarios[i])
        if not poligono.intersects(LineString([punto_inicial, *ruta_horario, punto_final])):
            break   
            
    for i in range(len(puntos_ajustados_antihorarios)):
        ruta_antihorario.append(puntos_ajustados_antihorarios[i])
        if not poligono.intersects(LineString([punto_inicial, *ruta_antihorario, punto_final])):
            break
    for i in range(len(ruta_horario)):
        ruta_ajustada_horario.insert(0, ruta_horario[-i-1])
        if not poligono.intersects(LineString([punto_inicial, *ruta_ajustada_horario, punto_final])):
            break
    for i in range(len(ruta_antihorario)):
        ruta_ajustada_antihorario.insert(0, ruta_antihorario[-i-1])
        if not poligono.intersects(LineString([punto_inicial, *ruta_ajustada_antihorario, punto_final])):
            break
            
    diferencias_horarios = np.diff(ruta_ajustada_horario, axis = 0)
    diferencias_antihorarios = np.diff(ruta_ajustada_antihorario, axis = 0)

    distancia_horarios = np.linalg.norm(diferencias_horarios, axis = 1).sum() + Point(ruta_ajustada_horario[0]).distance(Point(punto_inicial)) + Point(ruta_ajustada_horario[-1]).distance(Point(punto_final))
    distancia_antihorarios = np.linalg.norm(diferencias_antihorarios, axis = 1).sum() + Point(ruta_ajustada_antihorario[0]).distance(Point(punto_inicial)) + Point(ruta_ajustada_antihorario[-1]).distance(Point(punto_final))
    
    if distancia_horarios <= distancia_antihorarios:
        return ruta_ajustada_horario
    else:
        return ruta_ajustada_antihorario

dist_mas_corta = 1000000000
if len(resultados_sectores_rutas) >= 1:
    for w in range(len(resultados_sectores_rutas)):
        resultados_sectores = resultados_sectores_rutas[w]
        resultados_sectores_antes = np.copy(resultados_sectores)
        resultados_sectores = np.array(resultados_sectores)
        longitud_inicial = len(resultados_sectores[:,0])
        ruta_aplanada = np.linspace(0, longitud_inicial-1, longitud_inicial)
        ruta_final, resultados_sectores = rodear_zona_prohibida(ruta_aplanada, zonas_prohibidas, zonas_prohibidas_names, resultados_sectores)
        if ruta_final.size == 0:
            ruta_final = ruta_aplanada
            
        dist_ruta_actual = calcular_distancia_ruta(resultados_sectores)
        
        if dist_ruta_actual < dist_mas_corta:
            ruta_mas_corta = ruta_final
            dist_mas_corta = dist_ruta_actual
            resultados_sectores_ruta_mas_corta = resultados_sectores
            print("Número de puntos finales (PtP):", len(resultados_sectores_ruta_mas_corta))
            resultados_sectores_antes_ruta_mas_corta = resultados_sectores_antes
        longitud_final = len(ruta_final)
        n_puntos_rodeo = longitud_final - longitud_inicial
else:
    resultados_sectores = resultados_sectores_rutas[0]
    ruta_mas_corta = resultados_sectores
    resultados_sectores_ruta_mas_corta = resultados_sectores

if TipoTrayectoria == "Point to point":
    if len(resultados_sectores_rutas) >= 1:
        resultados_sectores = resultados_sectores_ruta_mas_corta
        
        #Dibujar el camino
        plt.plot(resultados_sectores_antes_ruta_mas_corta[:, 0], resultados_sectores_antes_ruta_mas_corta[:, 1], color='b', linestyle='--', label='Camino inicial')
        plt.plot(resultados_sectores_ruta_mas_corta[:, 0], resultados_sectores_ruta_mas_corta[:, 1], color='black', linestyle='-', label='Camino final')
        
        #Dibujar los puntos de rodeo (superponiendo sobre el camino)
        plt.scatter(resultados_sectores_ruta_mas_corta[:, 0], resultados_sectores_ruta_mas_corta[:, 1], color='black', label='Waypoints', s=150)
        for i, c in enumerate(resultados_sectores_ruta_mas_corta):
                plt.annotate(str(i), xy=c, fontsize=10, ha="center", va="center", color="white")
        #Dibujar las regiones (sectores)
        for sector_name in sectors_names:
            if sector_name in sectors and sector_name != "Net":  # Verificar si el sector existe en el diccionario
                if sector_name.startswith("Zona_prohibida"):
                    color = "red"
                else:
                    color = "blue"
                poligono = Polygon(sectors[sector_name])  # Convertir a polígono
                x_poly, y_poly = poligono.exterior.xy  # Obtener coordenadas del borde
                plt.fill(x_poly, y_poly, alpha=0.3, color=color, label=sector_name)  # Dibujar la región con transparencia
    
        #Agregar etiquetas y título
        plt.title('Final Route PtP ')
        plt.xlabel('X')
        plt.ylabel('Y')
    
        #Evitar duplicados en la leyenda
        handles, labels = plt.gca().get_legend_handles_labels()
        by_label = dict(zip(labels, handles))  # Eliminar duplicados en la leyenda
        plt.legend(by_label.values(), by_label.keys(), loc='center left', bbox_to_anchor=(1.0, 0.5))
    
        plt.grid(True)
        plt.tight_layout() 
        #Mostrar el gráfico
        plt.show()
        
    else:
            visualize_3(problem, res.X, sectors, j=0)
# Ahora hay que meter estos puntos en el xml

# Añadimos una columna de ceros donde meteremos los nombres
if not isinstance(resultados_sectores, np.ndarray):
    # Convertir a numpy array si no lo es
    resultados_sectores = np.array(resultados_sectores)

Columna_nombres = np.zeros(resultados_sectores.shape[0], dtype=object)  # Cambiado a un vector de tamaño adecuado

dtype = [('nombre', 'U10'), ('x', 'f4'), ('y', 'f4')]  # U10 para string, f4 para float

# Concatenar la columna de nombres y Puntos
ruta = np.empty(len(Columna_nombres)-1, dtype=dtype)  # Crear un array vacío del tipo correcto
ruta['nombre'] = Columna_nombres[1:]  # Asignar la columna de nombres
ruta['x'] = resultados_sectores[1:, 0]  # Asignar la columna x
ruta['y'] = resultados_sectores[1:, 1]  # Asignar la columna y

unchanged_array = np.zeros(len(unchangedpoints), dtype=dtype)

# Ponemos el nombre en la columna correspondiente
resultados_sectores = resultados_sectores[1:] #Como se ha añadido el home en el TSP como el primer punto, ahora hay que quitarlo
for i in range(len(resultados_sectores)):
        ruta[i]['nombre']=f'L{i}'
# Rellenamos este array
for i, (nombre, x_str, y_str) in enumerate(unchangedpoints):
    unchanged_array[i] = (nombre, float(x_str), float(y_str)) 

#Si se añade para que vaya más de un barco esto habrá que modificarlo para que cree diferentes archivos
# Reescribir todos los waypoints organizados desde 'resultados'

for name, x, y in ruta:
    new_waypoint = ET.Element('waypoint', x=str(x), y=str(y), name=name)
    way_points.append(new_waypoint)
for name, x, y in unchangedpoints:
    new_unchanged = ET.Element('waypoint', name=name, x=str(x), y=str(y))
    way_points.append(new_unchanged)
#Ahora hay que borrar la sección type de sectors ya que paparazzi no la interpreta pero nos ha valido para la optimizacion
for sector in root.findall(".//sector"):
    if 'type' in sector.attrib:
        del sector.attrib['type']
        
# Función para formatear el XML
def prettify(element):
    """ Devuelve una versión 'bonita' del XML """
    rough_string = ET.tostring(element, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="  ", newl="\n", encoding=None)

Archivo_sin_extension = os.path.splitext(Archivo)[0]

# Guardar los cambios en el archivo XML
ruta_archivo_opt = os.path.join(home_dir, "paparazzi", "conf", "flight_plans", f"{Archivo_sin_extension}_opt.xml")
with open(ruta_archivo_opt, 'w', encoding='utf-8') as f:
    f.write('<!DOCTYPE flight_plan SYSTEM "../flight_plan.dtd">\n')
    xml_pretty = prettify(root)
    xml_sin_version = '\n'.join(xml_pretty.splitlines()[1:])  
    # Escribir el XML limpio en el archivo
    f.write(xml_sin_version)
    
import xml.etree.ElementTree as ET

# Abrimos el archivo XML
ruta_controlador = os.path.join(home_dir, "paparazzi", "conf", "airframes", f"{Controlador}")
tree = ET.parse(ruta_controlador)
root = tree.getroot()

n_segmentos = str(len(paradas) - 1)  # Asegúrate de que `paradas` esté definida en el código

# Bandera para verificar si el cambio se realizó
cambio_realizado = False

# Buscar y modificar el valor de `GVF_PARAMETRIC_BARE_2D_BEZIER_N_SEG`
for module in root.iter('module'):
    if module.get('name') == 'gvf_parametric_bare':
        for define in module.findall('define'):
            if define.get('name') == 'GVF_PARAMETRIC_BARE_2D_BEZIER_N_SEG':
                define.set('value', n_segmentos)
                cambio_realizado = True  # Marcar el cambio como realizado


# Definir el CRS de tu sistema de coordenadas personalizado (ejemplo ficticio)
crs_personal = CRS(proj='tmerc', lat_0=lat0, lon_0=lon0, k=1.0, x_0=0, y_0=0, datum='WGS84')  # Ejemplo de un sistema transversal mercator
crs_wgs84 = CRS.from_epsg(4326)  # WGS84

# Crear un transformador entre tu sistema y WGS84
transformer = Transformer.from_crs(crs_personal, crs_wgs84, always_xy=True)

# Función para guardar los puntos en un archivo .txt
def guardar_puntos_en_txt(puntos, archivo_salida):
    try:
        with open(archivo_salida, 'w') as f:
            # Escribir los encabezados
            f.write("Nombre\tlat\tlon\n")
            
            # Escribir los puntos
            for punto in puntos:
                # Aquí no se usa UTM, solo transformamos de lat/lon a otro CRS si es necesario
                lon, lat = transformer.transform(punto['x'], punto['y'])
                lat_str = str(lat)
                lon_str = str(lon)
                lat_coma = lat_str.replace(".", ",")
                lon_coma = lon_str.replace(".", ",")

                f.write(f"{punto['nombre']}\t{lat_coma}\t{lon_coma}\n")
    except Exception as e:
        print(f"Error al guardar los puntos: {e}")

Archivo_basename = os.path.splitext(os.path.basename(Archivo))[0]

if TipoTrayectoria == "Point to point":
    #Creamos el vector para identificar que puntos son de rodeo y cuales de medida
    flag_stop = [0 if point in resultados_sectores_antes_ruta_mas_corta else 1 for point in resultados_sectores_ruta_mas_corta]
    #Necesitamos que flag_stop tenga 50 elementos, asique rellenamos con ceros hasta que esto se cumpla
    flag_stop += [0] * (50 - len(flag_stop))
    ruta_waypoints_finales = os.path.join(home_dir, "PprzGCS", "Planificacion", "Resources", "waypoints_opt", f"{Archivo_basename}_waypoints.txt")
    guardar_puntos_en_txt(ruta, ruta_waypoints_finales)

#Añadimos los puntos
points = []
xpoints = resultados_sectores_ruta_mas_corta[:, 0]
ypoints = resultados_sectores_ruta_mas_corta[:, 1]

for i in range(len(xpoints)):
    points.append([xpoints[i],ypoints[i]])

#Interpolar los puntos del TSP para sacar los puntos de la recta

def calcular_pendiente(punto1, punto2):
    x1, y1 = punto1
    x2, y2 = punto2

    if x2 == x1:
        raise ValueError("La pendiente es indefinida (división por cero).")
    
    pendiente = (y2 - y1) / (x2 - x1)
    return pendiente

def calcular_interseccion(punto, pendiente):
    x1, y1 = punto
    b = y1 - pendiente * x1
    return b

def calcular_puntos_en_recta(punto1, punto2, x_values):
    m = calcular_pendiente(punto1, punto2)
    b = calcular_interseccion(punto1, m)
    
    # Calcular los puntos en la recta
    puntos = [(x, m * x + b) for x in x_values]
    return puntos


import matplotlib.pyplot as plt
# Plot the original points
plt.scatter(xpoints, ypoints, s=150, c="black", edgecolors="white", label="Waypoints")

# Get the Bezier parameters based on a degree.
data = get_bezier_parameters(xpoints, ypoints, 0.005, degree=len(xpoints)*2-2) #BZ0 BZ5 BZ8 BZ11 son los de paso, por tanto habrá 4*2 + 1 puntos de contol ya que el algoritmo te pone 1 pnt cntrl en el 1 punto y en el último
xvals, yvals = bezier_curve(data, nTimes=1000)
xpoints_antes = resultados_sectores_antes_ruta_mas_corta[:,0]
ypoints_antes = resultados_sectores_antes_ruta_mas_corta[:,1]
data_antes = get_bezier_parameters(xpoints_antes, ypoints_antes, 0.005, degree=len(xpoints)*2-2)
xvals_antes, yvals_antes = bezier_curve(data_antes, nTimes=1000)
x_val = [x[0] for x in data]
y_val = [x[1] for x in data]

x_val = np.delete(x_val, 0)
x_val = np.delete(x_val, -1)
y_val = np.delete(y_val, 0)
y_val = np.delete(y_val, -1)

punto1 = (x_val[1], y_val[1])
punto2 = (x_val[0], y_val[0])

valores_x =(x_val[1]+(abs(x_val[1]-x_val[0]))/2, 0)

puntos_en_recta = calcular_puntos_en_recta(punto1, punto2, valores_x)


curve = LineString(np.column_stack((xvals, yvals)))
coords = list(curve.coords)[::-1]
cruces = []
for name in zonas_prohibidas_names:
    zona_prohibida_poligono = Polygon(zonas_prohibidas[name])
    for i in range(len(coords) - 1):
        segmento = LineString([coords[i], coords[i + 1]])
        if segmento.intersects(zona_prohibida_poligono):
            longitud_inicial = len(resultados_sectores[:,0])
            ruta_aplanada = np.linspace(0, longitud_inicial-1, longitud_inicial)
            ruta_final_bz, resultados_sectores_bz = rodear_zona_prohibida(ruta_aplanada, zonas_prohibidas, zonas_prohibidas_names, resultados_sectores)

x_val = np.insert(x_val, 1, puntos_en_recta[0][0])
#x_val = np.insert(x_val, 2, puntos_en_recta[1][0])
y_val = np.insert(y_val, 1, puntos_en_recta[0][1])
#y_val = np.insert(y_val, 2, puntos_en_recta[1][1])

if TipoTrayectoria == "Continuous":
    # Plot the control points
    plt.scatter(x_val, y_val, s=150, c="blue", edgecolors="white", linewidths=1, label='Control Points')

    for i, c in enumerate(zip(xpoints, ypoints)):
        plt.annotate(str(i), xy=c, fontsize=10, ha="center", va="center", color="white")

    # Ploteamos los sectores
    if sectores_navegacion > 0 :
        for sector_name, points in sectors.items():
            sector_points = np.array(points)
            color = "blue"
            if not sector_name.startswith("Net") and not sector_name.startswith("Zona_prohibida"):
                plt.fill(sector_points[:, 0], sector_points[:, 1], alpha=0.2, color=color, label=f'Sector {sector_name}')
    if num_zonas_prohibidas > 0:
        for sector_name, points in sectors.items():
            sector_points = np.array(points)
            color = "red"
            if sector_name.startswith("Zona_prohibida"):
                plt.fill(sector_points[:, 0], sector_points[:, 1], alpha=0.2, color=color, label=f'Sector {sector_name}')
                
    # Plot the resulting Bezier curve
    plt.plot(xvals_antes, yvals_antes, 'blue', label = 'Camino inicial', linestyle = '--')
    plt.plot(xvals, yvals, 'black', label='Camino final')
    
    plt.title("Final route continuous sin cruce")
    plt.legend(loc='center left', bbox_to_anchor=(1, 0.5))
    plt.grid(True)
    plt.xlabel("x")
    plt.ylabel("y")
    plt.tight_layout()  # Ajusta para que no se corte nada dentro de la figura
    plt.show()


#Para añadir los puntos de paso y de control en el txt para mandarselo a paparazzi desde la GCS. Guarda 1 paso, 2 control, 1 paso, 2 control...
#Añadimos en primer lugar BZ0 y los 4 puntos de control (si es con continuidad C2)
#Añadimos en primer lugar BZ0 y los 2 puntos de contro (si es con continuidad C0)
Puntos_paso = [xpoints, ypoints]
Puntos_paso = np.vstack((xpoints, ypoints)).T
Puntos_control = np.array([x_val, y_val]).T
Total_puntos = Puntos_control.shape[0] + Puntos_paso.shape[0]
Puntos_Bezier = np.zeros((Total_puntos, Puntos_control.shape[1]))
Puntos_Bezier = []



idx_paso, idx_control = 0, 0
while idx_paso < len(Puntos_paso):# and idx_control + 1 <= len(Puntos_control):
    # Agregar punto de paso (desempaquetado si es una lista)
    Puntos_Bezier.extend(Puntos_paso[idx_paso] if isinstance(Puntos_paso[idx_paso], list) else [Puntos_paso[idx_paso]])
    idx_paso += 1
    # Agregar dos puntos de control (desempaquetados si son listas)
    if idx_control + 1 < len(Puntos_control):
        Puntos_Bezier.extend(Puntos_control[idx_control] if isinstance(Puntos_control[idx_control], list) else [Puntos_control[idx_control]])
        Puntos_Bezier.extend(Puntos_control[idx_control + 1] if isinstance(Puntos_control[idx_control + 1], list) else [Puntos_control[idx_control + 1]])
        idx_control += 2  # Saltamos dos controles
# Convertir a un array NumPy si es necesario
Puntos_Bezier = np.array(Puntos_Bezier)


#En caso de que se quiera guardar la ruta con curvas de Bézier en vez de point-to-point
print("TipoTrayectoria: ", TipoTrayectoria)
if TipoTrayectoria == "Continuous":
    
    puntos_control=[x_val, y_val]
    # Concatenar la columna de nombres y Puntos
    columna_nombres_bz = np.zeros(len(Puntos_Bezier))
    ruta_bz = np.empty(len(Puntos_Bezier), dtype=dtype)  # Crear un array vacío del tipo correcto
    ruta_bz['nombre'] = columna_nombres_bz[:]  # Asignar la columna de nombres
    ruta_bz['x'] = Puntos_Bezier[:, 0]  # Asignar la columna x
    ruta_bz['y'] = Puntos_Bezier[:, 1]  # Asignar la columna y
    for i in range(len(Puntos_Bezier)):
            ruta_bz[i]['nombre']=f'BZ{i}'
    print("Número de puntos finales (Continuous):", len(ruta_bz))

    #Vamos a crear flag stop para ver que puntos son de rodeo y cuales de medida
    ruta_bz_arr = np.stack((ruta_bz["x"], ruta_bz["y"]), axis = 1)
    ruta_bz_arr_rounded = np.round(ruta_bz_arr.astype(np.float64), 2)
    resultados_sectores_antes_ruta_mas_corta_rounded = np.round(resultados_sectores_antes_ruta_mas_corta.astype(np.float64),2)
    #Para poder hacer bien la comparación, ya que estaba teniendo errores de redondeo, vuelvo a sacar los puntos de control de ruta_bz
    puntos_control_rounded = []
    for i in range(0, len(ruta_bz_arr), 3):
        puntos_control_rounded.append(ruta_bz_arr_rounded[i])
        puntos_control_rounded.append(ruta_bz_arr_rounded[i])
    flag_stop = []
    for i in range(len(ruta_bz_arr_rounded)):
        point = ruta_bz_arr_rounded[i]
        is_in = np.any(np.all(resultados_sectores_antes_ruta_mas_corta_rounded == point, axis=1))
        is_in_control = np.any(np.all(puntos_control_rounded == point, axis=1))
        if is_in:
            flag_stop.append(0)
        elif is_in_control:
            flag_stop.append(2)
        else:
            #print("Punto = ", point)
            flag_stop.append(1)

    #Necesitamos que flag_stop tenga 50 elementos, asique rellenamos con ceros hasta que tenga esta longitud
    flag_stop += [0] * (50 - len(flag_stop))
    ruta_waypoints_finales = os.path.join(home_dir, "PprzGCS", "Planificacion", "Resources", "waypoints_opt", f"{Archivo_basename}_waypoints.txt")
    guardar_puntos_en_txt(ruta_bz, ruta_waypoints_finales)

#Guardamos el flag_stop en un txt
# salida_flag = os.path.join(home_dir, "PprzGCS", "Planificacion", "Resources", "waypoints_opt", f"{Archivo_basename}_flag_stop.txt")
# with open(salida_flag, 'w') as f:
#     f.write(flag_stop)

import json

home_dir = os.path.expanduser("~")
salida_flag = os.path.join(
    home_dir, "PprzGCS", "Planificacion", "Resources", "flag_stop",
    f"{Archivo_basename}_flag_stop.txt"
)

with open(salida_flag, "w") as f:
    json.dump(flag_stop, f)          # <‑‑ convierte la lista en texto válido


# import json
# resultado = {"status": "success", "flag_stop": flag_stop, "Message": "Optimización exitosa"}
# print(json.dumps(resultado), flush=True)   # <-- flush=True garantiza que se vacíe el búfer

print("Optimización exitosa.")