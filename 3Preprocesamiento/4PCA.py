import pandas as pd
import numpy as np

print("=== SCRIPT DE PCA ===")

# Cargar archivo CSV limpio
print("Cargando archivo limpio...")
df = pd.read_csv('Database/2processed_data_complete_limpio.csv')
print(f"Forma del dataset limpio: {df.shape}")
print(f"Registros en archivo limpio: {df.shape[0]:,}")

# Lista de atributos para PCA (todos los numéricos excepto total_amount)
atributos_para_pca = [
    'passenger_count', 'trip_distance', 'payment_type',
    'fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge'
]

print(f"Atributos que se usarán para PCA: {atributos_para_pca}")

# Verificar que no hay valores NaN
print(f"Valores NaN en atributos PCA: {df[atributos_para_pca].isnull().sum().sum()}")

# Seleccionar solo los atributos para PCA
X = df[atributos_para_pca].values
n_muestras, n_atributos = X.shape

print(f"\nDatos para PCA:")
print(f"Número de muestras: {n_muestras:,}")
print(f"Número de atributos: {n_atributos}")

# Calcular media y desviación estándar
media_vector = np.mean(X, axis=0)
std_vector = np.std(X, axis=0, ddof=1)

# Estandarizar los datos (media 0, desviación estándar 1)
X_estandarizado = (X - media_vector) / std_vector

# Calcular matriz de covarianza
matriz_de_covarianza = np.cov(X_estandarizado.T, ddof=1)

# Calcular autovalores y autovectores
autovalores, autovectores = np.linalg.eigh(matriz_de_covarianza)

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

# Elegir k automáticamente según la varianza acumulada deseada
umbral = 0.95
k_auto = np.searchsorted(varianza_acumulada, umbral) + 1
print(f"\nSe seleccionan {k_auto} componentes para explicar al menos el {umbral*100:.1f}% de la varianza")

# Usar k automático, pero con límite razonable
#k = 6
k = min(k_auto, n_atributos)  # Máximo igual al número de atributos originales
print(f"Usando k = {k} componentes (de {n_atributos} atributos originales)")

W = autovectores_ordenados[:, :k]

# Proyectar los datos al nuevo subespacio
X_pca = np.dot(X_estandarizado, W)

# Cargar coordenadas lat/long y tripID
latlong = df[['pickup_latitude', 'pickup_longitude']].values
trip_ids = df['tripID'].values.reshape(-1, 1)  # Reshape para concatenar

# Formato de la matriz PCA
print(f"\nForma de X_pca: {X_pca.shape}")

# Concatenar tripID, lat, long con componentes principales
X_final = np.hstack((trip_ids, latlong, X_pca))
print(f"Forma final con tripID, lat, long y PCA: {X_final.shape}")

# Convertir a DataFrame
columnas_pca = [f'PC{i+1}' for i in range(k)]
df_final = pd.DataFrame(X_final, columns=['tripID', 'pickup_latitude', 'pickup_longitude'] + columnas_pca)

# Verificar que no se perdió ningún tripID
print(f"\nVerificación de tripIDs:")
print(f"TripIDs únicos en original: {len(df['tripID'].unique()):,}")
print(f"TripIDs únicos en final: {len(df_final['tripID'].unique()):,}")
print(f"¿Se mantuvieron todos los tripIDs? {len(df['tripID'].unique()) == len(df_final['tripID'].unique())}")

# Guardar en CSV sin redondear (mantener precisión completa)
df_final.to_csv('Database/3pca_data_set_complete.csv', index=False)
print(f"\nDatos PCA guardados en 'Database/3pca_data_set_complete.csv' (sin redondear)")
print(f"Columnas del archivo final: {list(df_final.columns)}")

print(f"\n=== RESUMEN FINAL ===")
print(f"- Dataset limpio: {df.shape[0]:,} muestras, {len(atributos_para_pca)} atributos")
print(f"- Componentes PCA seleccionados: {k}")
print(f"- Varianza explicada: {varianza_acumulada[k-1]*100:.2f}%")
print(f"- Reducción de dimensionalidad: {len(atributos_para_pca)} → {k} dimensiones")
print(f"- Registros en archivo final: {df_final.shape[0]:,}")
print(f"- Archivo de salida: Database/3pca_data_set_complete.csv")

print("\n¡PCA completado!")