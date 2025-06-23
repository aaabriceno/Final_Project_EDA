import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Cargar archivo CSV limpio
df = pd.read_csv('Database/processed_data_complete.csv')

# Lista de atributos para PCA (todos los numéricos excepto total_amount)
atributos_para_pca = [
    'passenger_count', 'trip_distance', 'payment_type',
    'fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge'
]

print(f"Atributos que se usarán para PCA: {atributos_para_pca}")
print(f"Forma del dataset original: {df.shape}")

# Corregir valores negativos en todos los atributos de tarifa
atributos_tarifa = ['fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge']
for columna in atributos_tarifa:
    min_valor = df[columna].min()
    if min_valor < 0:
        print(f"Corrigiendo valores negativos en {columna}: min={min_valor}")
        df[columna] = df[columna] + abs(min_valor)

# Recalcular total_amount
df['total_amount'] = (
    df['fare_amount'] + df['extra'] + df['mta_tax'] +
    df['tip_amount'] + df['tolls_amount'] + df['improvement_surcharge']
)

# Guardar archivo limpio
df.to_csv('Database/processed_data_complete_limpio.csv', index=False)
print("Archivo limpio guardado en 'Database/processed_data_complete_limpio.csv'")

# Seleccionar solo los atributos para PCA
X = df[atributos_para_pca].values
n_muestras, n_atributos = X.shape

print(f"\nDatos para PCA:")
print(f"Número de muestras: {n_muestras}")
print(f"Número de atributos: {n_atributos}")

# Mostrar estadísticas descriptivas
print("\nResumen estadístico por atributo:")
for i, atributo in enumerate(atributos_para_pca):
    print(f"{atributo}: min={X[:, i].min():.2f}, max={X[:, i].max():.2f}, media={X[:, i].mean():.2f}")

# Calcular media y desviación estándar
media_vector = np.mean(X, axis=0)
std_vector = np.std(X, axis=0, ddof=1)

print(f"\nMedia: {media_vector}")
print(f"Desviación estándar: {std_vector}")

# Estandarizar los datos (media 0, desviación estándar 1)
X_estandarizado = (X - media_vector) / std_vector

# Verificar estandarización
print(f"\nMedia de los datos estandarizados (debe ser ~0):")
print(np.mean(X_estandarizado, axis=0))

print(f"\nDesviación estándar de los datos estandarizados (debe ser ~1):")
print(np.std(X_estandarizado, axis=0, ddof=1))

# Calcular matriz de covarianza
matriz_de_covarianza = np.zeros((n_atributos, n_atributos))
for i in range(n_atributos):
    for j in range(n_atributos):
        matriz_de_covarianza[i, j] = np.sum(X_estandarizado[:, i] * X_estandarizado[:, j]) / (n_muestras - 1)

# Verificar que la matriz de covarianza es simétrica
print("\nCov es simétrica:", np.allclose(matriz_de_covarianza, matriz_de_covarianza.T))

# Calcular autovalores y autovectores
autovalores, autovectores = np.linalg.eigh(matriz_de_covarianza)

# Verificar que los autovalores son positivos
print("\nAutovalores > 0:", np.all(autovalores > 0))

# Ordenar autovalores y autovectores de mayor a menor
idx_sorted = np.argsort(autovalores)[::-1]
autovalores_ordenados = autovalores[idx_sorted]
autovectores_ordenados = autovectores[:, idx_sorted]

# Mostrar varianza explicada
total_varianza = np.sum(autovalores_ordenados)
ratio_varianza_explicada = autovalores_ordenados / total_varianza
varianza_acumulada = np.cumsum(ratio_varianza_explicada)

print("\nVarianza explicada por componente:")
for i in range(n_atributos):
    print(f"COMPONENTE {i+1}: {ratio_varianza_explicada[i]*100:.2f}% (acumulada: {varianza_acumulada[i]*100:.2f}%)")

# Verificar ortogonalidad de los autovectores
matriz_de_ortogonalidad = np.dot(autovectores_ordenados.T, autovectores_ordenados)
print("\nMatriz de productos punto (debe ser identidad):")
print(matriz_de_ortogonalidad)

# Elegir k automáticamente según la varianza acumulada deseada
umbral = 0.95
k_auto = np.searchsorted(varianza_acumulada, umbral) + 1
print(f"\nSe seleccionan {k_auto} componentes para explicar al menos el {umbral*100:.1f}% de la varianza")

# Usar k automático, pero con límite razonable
k = min(k_auto, n_atributos)  # Máximo igual al número de atributos originales
print(f"Usando k = {k} componentes (de {n_atributos} atributos originales)")

W = autovectores_ordenados[:, :k]

# Verificar forma de la matriz de proyección
print(f"Forma de W: {W.shape}")

# Proyectar los datos al nuevo subespacio
X_pca = np.dot(X_estandarizado, W)

# Cargar coordenadas lat/long
latlong = df[['pickup_latitude', 'pickup_longitude']].values

# Formato de la matriz PCA
print(f"\nForma de X_pca: {X_pca.shape}")
print(f"Primeras 5 filas de la matriz PCA:")
print(X_pca[:5])

# Concatenar lat, long con componentes principales
X_final = np.hstack((latlong, X_pca))
print(f"\nForma final con lat, long y PCA: {X_final.shape}")
np.set_printoptions(precision=4, suppress=True)
print("Primeras 3 filas del resultado final:")
print(X_final[:3])

# Convertir a DataFrame
columnas_pca = [f'PC{i+1}' for i in range(k)]
df_final = pd.DataFrame(X_final, columns=['lat', 'long'] + columnas_pca)

# Guardar en CSV con 4 decimales
df_final_redondeado = df_final.copy()
df_final_redondeado[columnas_pca] = df_final_redondeado[columnas_pca].round(4)

df_final_redondeado.to_csv('Preprocesamiento/pca_4_dec.csv', index=False)
print(f"\nDatos PCA guardados en 'Preprocesamiento/pca_4_dec.csv'")

# Mostrar información sobre las componentes principales
print(f"\nInformación de las componentes principales:")
for i in range(k):
    print(f"PC{i+1}: {ratio_varianza_explicada[i]*100:.2f}% de varianza")

# Graficar varianza explicada
plt.figure(figsize=(10, 6))
plt.plot(range(1, n_atributos + 1), ratio_varianza_explicada * 100, marker='o', label='Varianza individual')
plt.plot(range(1, n_atributos + 1), varianza_acumulada * 100, marker='s', label='Varianza acumulada')
plt.axvline(x=k, color='r', linestyle='--', label=f'k={k} componentes')
plt.axhline(y=95, color='g', linestyle=':', label='95% varianza')
plt.xlabel('Número de componentes')
plt.ylabel('Varianza explicada (%)')
plt.title('Curva de varianza explicada')
plt.legend()
plt.grid(True)

# Guardar la figura
plt.savefig('Preprocesamiento/varianza_explicada.png', dpi=300, bbox_inches='tight')
print("Gráfica guardada en 'Preprocesamiento/varianza_explicada.png'")
plt.show()

print(f"\nResumen:")
print(f"- Dataset original: {df.shape[0]} muestras, {len(atributos_para_pca)} atributos")
print(f"- Componentes PCA seleccionados: {k}")
print(f"- Varianza explicada: {varianza_acumulada[k-1]*100:.2f}%")
print(f"- Reducción de dimensionalidad: {len(atributos_para_pca)} → {k} dimensiones")