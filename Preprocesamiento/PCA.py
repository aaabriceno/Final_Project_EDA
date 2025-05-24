import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


#Cagar el archivo CSV columnas de 2 a 21 (atributos)
X = np.loadtxt('Preprocesamiento/puntos.csv', delimiter=',', skiprows=1, usecols=range(2, 22))
n_muestras, n_atributos = X.shape


# Mostrar la forma de la matriz
print(f"numero_muestras: {n_muestras}, dimensiones = {n_atributos}\n")


#Resumen estadístico por columna
for i in range(n_atributos):
    print(f"Atributo{i+1}: minimo={X[:, i].min()}, maximo={X[:, i].max()}, media={X[:, i].mean():.2f}")
   
    
#Calcular la media manualmente
media_vector = np.mean(X, axis=0)
std_vector = np.std(X, axis=0,ddof=1)

print(f"\nMedia: {media_vector}")
print(f"Desviación estándar: {std_vector}")


#Estandarizar los datos (media 0, desviación estándar 1)
X_estandarizado = (X - media_vector) / std_vector


#Verificar que la media de los datos centrados es cero
print(f"\nMedia de los datos estandarizados: (debe ser 0):")    
print(np.mean(X_estandarizado, axis=0))

print(f"\nDesviación estándar de los datos estandarizados (debe ser ~1):")
print(np.std(X_estandarizado, axis=0, ddof=1))


#Calcular cada entrada de la matrixz de covarianza de los datos estandarizados
matriz_de_covarianza = np.zeros((n_atributos, n_atributos))
for i in range(n_atributos):
    for j in range(n_atributos):
        matriz_de_covarianza[i, j] = np.sum(X_estandarizado[:, i] * X_estandarizado[:, j]) / (n_muestras - 1)


#Verificar que la matriz de covarianza es simétrica
print("Cov es simétrica:", np.allclose(matriz_de_covarianza, matriz_de_covarianza.T))


#Calcular los autovalores y autovectores
autovalores, autovectores = np.linalg.eigh(matriz_de_covarianza)


#Verificar que los autovalores son positivos
print("\nAutovlalores > 0:", np.all(autovalores > 0))


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
matriz_de_ortigonalidad = np.dot(autovectores_ordenados.T, autovectores_ordenados)
print("Matriz de productos punto (debe ser identidad):")
print(matriz_de_ortigonalidad)


#Elegir k automáticamente según la varianza acumulada deseada

#umbral = 0.95
#varianza_acumulada = np.cumsum(ratio_varianza_explicada)
#k = np.searchsorted(varianza_acumulada, umbral) + 1
#print(f"\nSe seleccionan {k} componentes para explicar al menos el {umbral*100:.1f}% de la varianza acumulada")

k = 15;
W = autovectores_ordenados[:, :k]


#Verificar forma de la matriz de proyección
print("Forma de W:",W.shape)  #debería ser (n_atributos, k)


#Proyectar los datos centrados al nuevo subespacio
X_pca = np.dot(X_estandarizado, W)


#Cargas lat y long a la matriz PCA
latlong = np.loadtxt('Preprocesamiento/puntos.csv', delimiter=',', skiprows=1, usecols=(0, 1))


#Formato de la matriz PCA
print("Forma de X_pca:", X_pca.shape) 
print("Matriz PCA", k, "dimensiones:\n")
print(X_pca[:10])  # Mostrar las primeras 10 filas de la matriz PCA


# Concatenar lat, long con componentes principales
X_final = np.hstack((latlong, X_pca))
print("Forma final con lat, long y PCA: ", X_final.shape)
np.set_printoptions(precision=4, suppress=True)  # Para mostrar menos decimales
print(X_final[:3])


#Convertir a DataFrame
df_final = pd.DataFrame(X_final, columns=['lat', 'long'] + [f'attr{i+1}' for i in range(k)])
df_copia = df_final.copy()

pca_columnas = df_final.columns[2:]  # Atributos PCA
df_copia[pca_columnas] = df_copia[pca_columnas].round(4)  # Redondear a 4 decimales


#Guardar en CSV, datos sin redondear decimales
#df_final.to_csv('Preprocesamiento/pca_puro.csv', index=False)
#print("Datos PCA guardados en 'Preprocesamiento/pca.csv'")


#Guardar en CSV 4 decimales
df_copia.to_csv('Preprocesamiento/pca_4_dec.csv', index=False)
print("Datos PCA guardados en 'Preprocesamiento/pca_4_dec.csv'")


# Graficar varianza explicada
plt.plot(ratio_varianza_explicada, marker='o')
plt.axvline(x=k-1, color='r', linestyle='--', label=f'k={k}')   
plt.xlabel('Número de componentes')
plt.ylabel('Varianza explicada acumulada')  # No olvides esta línea para el eje y
plt.title('Curva de varianza explicada')
plt.legend()
plt.grid(True)


#Guarda la figura en un archivo
plt.savefig('Preprocesamiento/varianza_explicada.png')
print("Gráfica guardada en 'varianza_explicada.png'")