#Modulo para las funciones que se utilizan en la optimización

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
    
    return reordered_points


class CustomSampling(Sampling):
    def _do(self, problem, n_samples, **kwargs):
        # Generar n_samples rutas aleatorias
        samples = []
        for _ in range(n_samples):
            # Crear una ruta aleatoria (permutación de las paradas)
            route = np.random.permutation(n_stops)
            #print(f"n_paradas {n_stops}")
            samples.append(route)
            #print(f"samples {samples}")
        return np.array(samples)

 #Visualización de los resultados
def visualize_3(problem, x, n, fig=None, ax=None, show=True, label=True):
    with plt.style.context('ggplot'):
        # Asegúrate de que x sea un array unidimensional
        x = np.asarray(x)  # Convierte x a un array de numpy

        if x.ndim == 1:  # Asegúrate de que es un vector
            x = x.flatten()  # Aplana el array para asegurarte de que sea 1D

        if fig is None or ax is None:
            fig, ax = plt.subplots()

        # Plot de las ciudades usando scatter plot
        ax.scatter(problem.stops[:, 0], problem.stops[:, 1], s=150)
        if label:
            # Anota las ciudades
            for i, c in enumerate(problem.stops):
                ax.annotate(str(i), xy=c, fontsize=10, ha="center", va="center", color="white")

        # Plotea la línea en el camino
        for i in range(len(x) - 1):
            current = int(x[i])  # Asegúrate de convertir a entero
            next_ = int(x[i + 1])  # Asegúrate de convertir a entero
            ax.plot(problem.stops[[current, next_], 0], problem.stops[[current, next_], 1], 'r--')

        fig.suptitle(f"Route length: {problem.get_route_length(x)} km \nRoute time: {problem.get_route_length(x) * 60 / velocidad_media} min")

        if show:
            plt.show()

#Funciones para el cálculo de curvas de Bezier

from scipy.special import comb

def get_bezier_parameters(X, Y, smooth_factor, degree=3):
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

    #if len(X) < degree + 1:
    #    raise ValueError(f'There must be at least {degree + 1} points to '
    #                     f'determine the parameters of a degree {degree} curve. '
    #                     f'Got only {len(X)} points.')

    def bpoly(n, t, k):
        """ Bernstein polynomial when a = 0 and b = 1. """
        return t ** k * (1 - t) ** (n - k) * comb(n, k)
        #return comb(n, i) * ( t**(n-i) ) * (1 - t)**i

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


def bezier_curve(points, nTimes=50):
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

# Función para formatear el XML
def prettify(element):
    """ Devuelve una versión 'bonita' del XML """
    rough_string = ET.tostring(element, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="  ")