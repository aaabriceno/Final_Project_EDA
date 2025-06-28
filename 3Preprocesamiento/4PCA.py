import pandas as pd
import numpy as np
from sklearn.decomposition import PCA
from sklearn.preprocessing import StandardScaler
import matplotlib.pyplot as plt

# Configurar pandas para mantener alta precisión
pd.set_option('display.float_format', '{:.13f}'.format)
pd.set_option('display.precision', 13)

print("=== SCRIPT DE PCA ===")

# Cargar archivo CSV limpio
print("Cargando archivo limpio...")
df = pd.read_csv('2Database/2processed_data_complete_limpio.csv')
print(f"Forma del dataset limpio: {df.shape}")
print(f"Registros en archivo limpio: {df.shape[0]:,}")
print(f"Columnas disponibles: {list(df.columns)}")

# Lista de atributos para PCA (todos los numéricos excepto total_amount)
atributos_para_pca = [
    'passenger_count', 'trip_distance', 'payment_type',
    'fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge'
]

# Verificar que todas las columnas existen
columnas_faltantes = [col for col in atributos_para_pca if col not in df.columns]
if columnas_faltantes:
    print(f"❌ ERROR: Columnas faltantes: {columnas_faltantes}")
    print(f"Columnas disponibles: {list(df.columns)}")
    exit(1)

print(f"✅ Atributos que se usarán para PCA: {atributos_para_pca}")

# Verificar que no hay valores NaN
nan_count = df[atributos_para_pca].isnull().sum().sum()
print(f"Valores NaN en atributos PCA: {nan_count}")

if nan_count > 0:
    print("⚠️ ADVERTENCIA: Hay valores NaN. Se eliminarán...")
    df = df.dropna(subset=atributos_para_pca)
    print(f"Registros después de eliminar NaN: {len(df):,}")

# Seleccionar solo los atributos para PCA
X = df[atributos_para_pca].values
n_muestras, n_atributos = X.shape

print(f"\nDatos para PCA:")
print(f"Número de muestras: {n_muestras:,}")
print(f"Número de atributos: {n_atributos}")

# Verificar que hay suficientes datos
if n_muestras < n_atributos:
    print("❌ ERROR: No hay suficientes muestras para PCA")
    exit(1)

# Calcular media y desviación estándar
media_vector = np.mean(X, axis=0)
std_vector = np.std(X, axis=0, ddof=1)

# Verificar que no hay desviación estándar cero
if np.any(std_vector == 0):
    print("❌ ERROR: Hay columnas con desviación estándar cero")
    print(f"Columnas problemáticas: {[atributos_para_pca[i] for i, std in enumerate(std_vector) if std == 0]}")
    exit(1)

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
output_path = '2Database/3pca_data_set_complete.csv'
df_final.to_csv(output_path, index=False)
print(f"\nDatos PCA guardados en '{output_path}' (sin redondear)")
print(f"Columnas del archivo final: {list(df_final.columns)}")

# Mostrar estadísticas de las componentes PCA
print(f"\nEstadísticas de las componentes PCA:")
print(df_final[columnas_pca].describe())

print(f"\n=== RESUMEN FINAL ===")
print(f"- Dataset limpio: {df.shape[0]:,} muestras, {len(atributos_para_pca)} atributos")
print(f"- Componentes PCA seleccionados: {k}")
print(f"- Varianza explicada: {varianza_acumulada[k-1]*100:.2f}%")
print(f"- Reducción de dimensionalidad: {len(atributos_para_pca)} → {k} dimensiones")
print(f"- Registros en archivo final: {df_final.shape[0]:,}")
print(f"- Archivo de salida: {output_path}")

print("\n¡PCA completado!")