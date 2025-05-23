import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


#Cagar el archivo CSV columnas de 2 a 21 (atributos)
X = np.loadtxt('puntos.csv', delimiter=',', skiprows=1, usecols=range(2, 22))

n_muestras, n_atributos = X.shape
# Mostrar la forma de la matriz
print(f"numero_muestras: {n_muestras}, dimensiones = {n_atributos}\n")

#Resumen estadístico por columna
for i in range(n_atributos):
    print(f"Atributo{i+1}: minimo={X[:, i].min()}, maximo={X[:, i].max()}, media={X[:, i].mean():.2f}")
    
#Calcular la media manualmente
mean_vector = np.sum(X, axis=0) / n_muestras
print(f"\nMedia: {mean_vector}")

#Restar la media a cada valor para centrar los datos
X_centro = X - mean_vector

#Verificar que la media de los datos centrados es cero
print(f"\nMedia de los datos centrados:")
print(np.mean(X_centro, axis=0))

#Calcular cada entrada de la matrixz de covarianza
covarianza_matriz = np.zeros((n_atributos, n_atributos))
for i in range(n_atributos):
    for j in range(n_atributos):
        covarianza_matriz[i, j] = np.sum(X_centro[:, i] * X_centro[:, j]) / (n_muestras - 1)

#Verificar que la matriz de covarianza es simétrica
print("Cov es simétrica:", np.allclose(covarianza_matriz, covarianza_matriz.T))

#Calcular los autovalores y autovectores
autovalores, autovectores = np.linalg.eig(covarianza_matriz)

#Verificar que los autovalores son positivos
print("Autovlalores > 0:", np.all(autovalores > 0))

#Ordenar los autovalores y autovectores manualmente de mayor a menor
idx_sorted = np.argsort(autovalores)[::-1]
autovalores_ordenados = autovalores[idx_sorted]
autovectores_ordenados = autovectores[:, idx_sorted]

#Mostrar la varianza explicada total y parcial
total_varianza = np.sum(autovalores_ordenados)
ratio_varianza_explicada = autovalores_ordenados / total_varianza
for i in range(n_atributos):
    print(f"COMPONENTE {i+1}: {ratio_varianza_explicada[i]*100:.2f}% de la varianza")

#Verificar ortogonalidad de los autovectores
dot_productos = np.dot(autovectores_ordenados.T, autovectores_ordenados)
print("Matriz de productos punto (debe ser identidad):")
print(dot_productos)

#Seleccionar las k componentes principales
k = 10
W = autovectores_ordenados[:, :k]

#Verificar forma de la matriz de proyección
print("Forma de W:",W.shape)  #debería ser (n_atributos, k)

#Proyectar los datos centrados al nuevo subespacio
X_pca = np.dot(X_centro, W)

#Cargas lat y long a la matriz PCA
latlong = np.loadtxt('puntos.csv', delimiter=',', skiprows=1, usecols=(0, 1))

#Formato de la matriz PCA
print("Forma de X_pca:", X_pca.shape) 
print("Matriz PCA", k, "dimensiones:\n")
print(X_pca[:10])  # Mostrar las primeras 10 filas de la matriz PCA


X_final = np.hstack((latlong, X_pca))
print("Forma final con lat, long y PCA:\n", X_final.shape)
print(X_final[:10])

plt.plot(ratio_varianza_explicada, marker ='o')
plt.axvline(x=k-1, color='r', linestyle='--', label=f'k={k}')
plt.xlabel('Numero de componentes') 


plt.title('Curva de varianza explicada')
plt.legend()
plt.grid(True)
plt.show()