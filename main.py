import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from scipy.optimize import curve_fit

# Crear datos de prueba
x = np.linspace(0, 10, 100)
y = 2.5 * x + np.random.normal(size=100)

# Ajuste lineal
def line(x, a, b):
    return a * x + b

params, _ = curve_fit(line, x, y)

# Graficar
plt.scatter(x, y, label="Datos")
plt.plot(x, line(x, *params), color="red", label="Ajuste lineal")
plt.legend()
plt.show()
