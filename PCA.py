import pandas as pd
import numpy as np

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
X_centered = X - mean_vector

#Verificar que la media de los datos centrados es cero
print(f"\nMedia de los datos centrados:")
print(np.mean(X_centered, axis=0))

#Calcular cada entrada de la matrixz de covarianza
covarianza_matriz = np.zeros((n_atributos, n_atributos))
for i in range(n_atributos):
    for j in range(n_atributos):
        covarianza_matriz[i, j] = np.sum(X_centered[:, i] * X_centered[:, j]) / (n_muestras - 1)
