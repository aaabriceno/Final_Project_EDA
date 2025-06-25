import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.neighbors import NearestNeighbors
from sklearn.cluster import DBSCAN
from scipy.spatial.distance import pdist, squareform
import seaborn as sns

print("=== CÁLCULO AUTOMÁTICO DE EPS PARA DBSCAN (DATOS PCA) ===")

# Cargar datos PCA
print("Cargando datos PCA...")
df = pd.read_csv('Database/3pca_data_set_complete.csv')

print(f"Dataset PCA shape: {df.shape}")
print(f"Columnas: {list(df.columns)}")

# Usar muestra para cálculos (más rápido)
if len(df) > 100000:
    print(f"Dataset muy grande ({len(df):,} registros). Usando muestra de 50,000 para cálculos...")
    df_sample = df.sample(n=50000, random_state=42)
else:
    df_sample = df

print(f"Usando {len(df_sample):,} puntos para análisis")

# Preparar coordenadas (lat, long)
coords = df_sample[['lat', 'long']].values

print(f"\nRango de coordenadas:")
print(f"Latitud: {coords[:, 0].min():.6f} a {coords[:, 0].max():.6f}")
print(f"Longitud: {coords[:, 1].min():.6f} a {coords[:, 1].max():.6f}")

# MÉTODO 1: Análisis de k-Nearest Neighbors (KNN)
print("\n=== MÉTODO 1: Análisis K-Nearest Neighbors ===")

# Calcular distancias a los k vecinos más cercanos
k_values = [5, 10, 15, 20]
eps_candidates = []

for k in k_values:
    print(f"\nCalculando distancias para k={k}...")
    
    # Usar NearestNeighbors para calcular distancias
    nbrs = NearestNeighbors(n_neighbors=k+1, algorithm='ball_tree').fit(coords)
    distances, indices = nbrs.kneighbors(coords)
    
    # Tomar la distancia al k-ésimo vecino (excluyendo el punto mismo)
    k_distances = distances[:, k]
    
    # Calcular estadísticas
    mean_dist = np.mean(k_distances)
    median_dist = np.median(k_distances)
    percentile_90 = np.percentile(k_distances, 90)
    percentile_95 = np.percentile(k_distances, 95)
    
    print(f"  k={k}: Media={mean_dist:.6f}, Mediana={median_dist:.6f}")
    print(f"  k={k}: Percentil 90={percentile_90:.6f}, Percentil 95={percentile_95:.6f}")
    
    eps_candidates.append({
        'k': k,
        'mean': mean_dist,
        'median': median_dist,
        'p90': percentile_90,
        'p95': percentile_95
    })

# MÉTODO 2: Análisis de distribución de distancias
print("\n=== MÉTODO 2: Análisis de Distribución de Distancias ===")

# Calcular distancias entre todos los puntos (muestra más pequeña para velocidad)
if len(coords) > 10000:
    coords_dist = coords[:10000]
    print(f"Usando {len(coords_dist)} puntos para cálculo de distancias completas...")
else:
    coords_dist = coords

# Calcular matriz de distancias
print("Calculando matriz de distancias...")
distances = pdist(coords_dist)
print(f"Distancia mínima: {distances.min():.6f}")
print(f"Distancia máxima: {distances.max():.6f}")
print(f"Distancia media: {distances.mean():.6f}")
print(f"Distancia mediana: {np.median(distances):.6f}")

# Calcular percentiles
percentiles = [50, 75, 80, 85, 90, 95, 99]
print("\nPercentiles de distancias:")
for p in percentiles:
    value = np.percentile(distances, p)
    print(f"  Percentil {p}: {value:.6f}")

# MÉTODO 3: Análisis por densidad de población
print("\n=== MÉTODO 3: Análisis por Densidad de Población ===")

# Calcular densidad aproximada (puntos por área)
lat_range = coords[:, 0].max() - coords[:, 0].min()
lon_range = coords[:, 1].max() - coords[:, 1].min()
area_approx = lat_range * lon_range
density = len(coords) / area_approx

print(f"Área aproximada: {area_approx:.6f} grados²")
print(f"Densidad: {density:.0f} puntos/grado²")

# Calcular eps basado en densidad
eps_density = np.sqrt(1 / density) * 0.1  # Factor de ajuste
print(f"EPS sugerido por densidad: {eps_density:.6f}")

# VISUALIZACIÓN: Curva del codo (Elbow curve)
print("\n=== GENERANDO GRÁFICAS DE ANÁLISIS ===")

# Gráfica 1: Distancias KNN
plt.figure(figsize=(15, 10))

plt.subplot(2, 3, 1)
for candidate in eps_candidates:
    plt.plot([candidate['k']], [candidate['mean']], 'o', label=f'k={candidate["k"]}')
    plt.plot([candidate['k']], [candidate['median']], 's', label=f'k={candidate["k"]} (mediana)')
plt.xlabel('k (número de vecinos)')
plt.ylabel('Distancia')
plt.title('Distancias KNN por k')
plt.legend()
plt.grid(True, alpha=0.3)

# Gráfica 2: Distribución de distancias
plt.subplot(2, 3, 2)
plt.hist(distances, bins=100, alpha=0.7, color='blue', edgecolor='black')
plt.xlabel('Distancia')
plt.ylabel('Frecuencia')
plt.title('Distribución de Distancias')
plt.grid(True, alpha=0.3)

# Gráfica 3: Percentiles de distancias
plt.subplot(2, 3, 3)
percentile_values = [np.percentile(distances, p) for p in percentiles]
plt.plot(percentiles, percentile_values, 'o-', color='red')
plt.xlabel('Percentil')
plt.ylabel('Distancia')
plt.title('Percentiles de Distancias')
plt.grid(True, alpha=0.3)

# Gráfica 4: Comparación de métodos
plt.subplot(2, 3, 4)
methods = ['KNN-Mean', 'KNN-Median', 'KNN-P90', 'KNN-P95', 'Density']
values = [
    eps_candidates[1]['mean'],  # k=10
    eps_candidates[1]['median'],  # k=10
    eps_candidates[1]['p90'],  # k=10
    eps_candidates[1]['p95'],  # k=10
    eps_density
]
plt.bar(methods, values, color=['blue', 'green', 'orange', 'red', 'purple'])
plt.ylabel('Valor de EPS')
plt.title('Comparación de Métodos')
plt.xticks(rotation=45)
plt.grid(True, alpha=0.3)

# Gráfica 5: Análisis de sensibilidad
plt.subplot(2, 3, 5)
k_range = [candidate['k'] for candidate in eps_candidates]
mean_range = [candidate['mean'] for candidate in eps_candidates]
median_range = [candidate['median'] for candidate in eps_candidates]

plt.plot(k_range, mean_range, 'o-', label='Media', color='blue')
plt.plot(k_range, median_range, 's-', label='Mediana', color='red')
plt.xlabel('k (número de vecinos)')
plt.ylabel('Distancia')
plt.title('Sensibilidad a k')
plt.legend()
plt.grid(True, alpha=0.3)

# Gráfica 6: Distribución logarítmica
plt.subplot(2, 3, 6)
plt.hist(np.log10(distances + 1e-10), bins=100, alpha=0.7, color='green', edgecolor='black')
plt.xlabel('log10(Distancia)')
plt.ylabel('Frecuencia')
plt.title('Distribución Logarítmica')
plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('Preprocesamiento/AnalisisCluster/analisis_eps_dbscan.png', dpi=300, bbox_inches='tight')
plt.close()
print("Gráfica guardada: analisis_eps_dbscan.png")

# RECOMENDACIONES FINALES
print("\n=== RECOMENDACIONES PARA EPS ===")

# Calcular recomendaciones
recommended_eps = {
    'conservador': eps_candidates[1]['median'],  # k=10, mediana
    'moderado': eps_candidates[1]['p90'],        # k=10, percentil 90
    'agresivo': eps_candidates[1]['p95'],        # k=10, percentil 95
    'densidad': eps_density
}

print("Valores recomendados de EPS:")
for approach, value in recommended_eps.items():
    print(f"  {approach.capitalize()}: {value:.6f}")

print(f"\nRECOMENDACIÓN PRINCIPAL:")
print(f"  Usar EPS = {recommended_eps['moderado']:.6f} (percentil 90, k=10)")
print(f"  Este valor captura el 90% de las distancias más pequeñas")

print(f"\nPARÁMETROS SUGERIDOS PARA DBSCAN:")
print(f"  eps = {recommended_eps['moderado']:.6f}")
print(f"  min_samples = 5 (mínimo para formar cluster)")
print(f"  metric = 'euclidean'")

# Prueba rápida con el valor recomendado
print(f"\n=== PRUEBA RÁPIDA CON EPS RECOMENDADO ===")
eps_test = recommended_eps['moderado']

# Usar muestra más pequeña para prueba rápida
if len(coords) > 10000:
    coords_test = coords[:10000]
else:
    coords_test = coords

print(f"Probando DBSCAN con eps={eps_test:.6f} en {len(coords_test)} puntos...")
dbscan = DBSCAN(eps=eps_test, min_samples=5, metric='euclidean')
labels = dbscan.fit_predict(coords_test)

n_clusters = len(set(labels)) - (1 if -1 in labels else 0)
n_noise = list(labels).count(-1)

print(f"Resultados:")
print(f"  Número de clusters: {n_clusters}")
print(f"  Puntos de ruido: {n_noise} ({n_noise/len(coords_test)*100:.1f}%)")
print(f"  Puntos en clusters: {len(coords_test) - n_noise} ({(len(coords_test) - n_noise)/len(coords_test)*100:.1f}%)")

# ANÁLISIS ADICIONAL: Componentes PCA
print(f"\n=== ANÁLISIS DE COMPONENTES PCA ===")
pca_columns = [col for col in df.columns if col.startswith('PC')]
print(f"Componentes PCA disponibles: {pca_columns}")

if len(pca_columns) > 0:
    print(f"\nEstadísticas de componentes PCA:")
    print(df_sample[pca_columns].describe())
    
    # Correlación entre componentes
    print(f"\nMatriz de correlación entre componentes PCA:")
    corr_matrix = df_sample[pca_columns].corr()
    print(corr_matrix)
    
    # Visualizar correlaciones
    plt.figure(figsize=(8, 6))
    sns.heatmap(corr_matrix, annot=True, cmap='coolwarm', center=0)
    plt.title('Correlación entre Componentes PCA')
    plt.tight_layout()
    plt.savefig('Preprocesamiento/AnalisisCluster/correlacion_pca_dbscan.png', dpi=300, bbox_inches='tight')
    plt.close()
    print("Gráfica de correlación PCA guardada: correlacion_pca_dbscan.png")

print(f"\n¡Análisis completado! Revisa 'analisis_eps_dbscan.png' para visualizar los resultados.")
print(f"Ahora puedes usar estos valores de eps para aplicar DBSCAN a tus datos PCA.") 