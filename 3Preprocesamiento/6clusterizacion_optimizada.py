import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.cluster import MiniBatchKMeans
from sklearn.neighbors import NearestNeighbors
import seaborn as sns
import gc
import warnings
warnings.filterwarnings('ignore')

# Configurar pandas para mantener alta precisión
pd.set_option('display.float_format', '{:.14f}'.format)
pd.set_option('display.precision', 13)

print("=== CLUSTERING OPTIMIZADO PARA 10M PUNTOS ===")

# Cargar datos PCA
print("Cargando datos PCA...")
try:
    df = pd.read_csv('2Database/3pca_data_set_complete.csv')
    print(f"Dataset PCA shape: {df.shape}")
    print(f"Columnas: {list(df.columns)}")
except FileNotFoundError:
    print("Archivo PCA no encontrado, intentando con datos procesados...")
    df = pd.read_csv('2Database/2processed_data_complete_limpio.csv')
    print(f"Dataset procesado shape: {df.shape}")
    print(f"Columnas: {list(df.columns)}")

# ESTRATEGIA 1: ORDENAMIENTO + MINI-BATCH K-MEANS
print("\n=== ESTRATEGIA 1: ORDENAMIENTO + MINI-BATCH K-MEANS ===")

def ordenar_y_clustering_espacial(coords, n_clusters=100, batch_size=10000):
    """
    Ordenar puntos por lat/long y aplicar Mini-Batch K-Means
    """
    print(f"Aplicando ordenamiento + Mini-Batch K-Means...")
    print(f"  Número de clusters objetivo: {n_clusters}")
    print(f"  Batch size: {batch_size}")
    
    # Paso 1: Ordenar puntos por latitud y longitud
    print("  Ordenando puntos por coordenadas...")
    
    # Crear índices ordenados (más eficiente que ordenar todo el array)
    lat_sorted_idx = np.argsort(coords[:, 0])  # Ordenar por latitud
    lon_sorted_idx = np.argsort(coords[:, 1])  # Ordenar por longitud
    
    # Combinar ordenamiento (prioridad a latitud, luego longitud)
    combined_idx = np.lexsort((coords[:, 1], coords[:, 0]))  # (lat, lon)
    
    coords_ordenados = coords[combined_idx]
    
    print(f"  Puntos ordenados: {len(coords_ordenados):,}")
    
    # Paso 2: Aplicar Mini-Batch K-Means
    print("  Aplicando Mini-Batch K-Means...")
    
    kmeans = MiniBatchKMeans(
        n_clusters=n_clusters,
        batch_size=batch_size,
        random_state=42,
        n_init=3,
        max_iter=100
    )
    
    labels = kmeans.fit_predict(coords_ordenados)
    
    # Reordenar labels al orden original
    labels_original = np.zeros_like(labels)
    labels_original[combined_idx] = labels
    
    return labels_original, kmeans.cluster_centers_, n_clusters

# ESTRATEGIA 2: GRID-BASED ADAPTATIVO
print("\n=== ESTRATEGIA 2: GRID-BASED ADAPTATIVO ===")

def grid_based_adaptativo(coords, target_clusters=100):
    """
    Grid-based que se adapta para obtener el número de clusters deseado
    """
    print(f"Aplicando Grid-Based Adaptativo...")
    print(f"  Clusters objetivo: {target_clusters}")
    
    # Calcular grid_size basado en el área y número de clusters deseado
    lat_range = coords[:, 0].max() - coords[:, 0].min()
    lon_range = coords[:, 1].max() - coords[:, 1].min()
    area = lat_range * lon_range
    
    # Estimar grid_size para obtener aproximadamente target_clusters
    grid_size = np.sqrt(area / target_clusters)
    
    print(f"  Grid size calculado: {grid_size:.6f}")
    
    # Crear grid bins
    lat_bins = np.arange(coords[:, 0].min(), coords[:, 0].max() + grid_size, grid_size)
    lon_bins = np.arange(coords[:, 1].min(), coords[:, 1].max() + grid_size, grid_size)
    
    # Asignar puntos a celdas
    lat_indices = np.digitize(coords[:, 0], lat_bins) - 1
    lon_indices = np.digitize(coords[:, 1], lon_bins) - 1
    grid_ids = lat_indices * len(lon_bins) + lon_indices
    
    # Contar puntos por celda
    unique_grids, counts = np.unique(grid_ids, return_counts=True)
    
    # Filtrar celdas con suficientes puntos
    min_points = max(5, len(coords) // (target_clusters * 10))
    valid_grids = unique_grids[counts >= min_points]
    
    # Asignar cluster labels
    cluster_labels = np.full(len(coords), -1)
    for i, grid_id in enumerate(valid_grids):
        mask = grid_ids == grid_id
        cluster_labels[mask] = i
    
    # Calcular centroides aproximados
    centroids = []
    for i, grid_id in enumerate(valid_grids):
        mask = grid_ids == grid_id
        centroid = coords[mask].mean(axis=0)
        centroids.append(centroid)
    
    centroids = np.array(centroids)
    
    return cluster_labels, centroids, len(valid_grids)

# ESTRATEGIA 3: HÍBRIDA (RECOMENDADA)
print("\n=== ESTRATEGIA 3: HÍBRIDA (RECOMENDADA) ===")

def clustering_hibrido(coords, n_clusters_objetivo=100):
    """
    Estrategia híbrida: ordenamiento + muestreo + clustering
    """
    print(f"Aplicando Clustering Híbrido...")
    print(f"  Clusters objetivo: {n_clusters_objetivo}")
    
    # Paso 1: Ordenar puntos
    print("  Paso 1: Ordenando puntos...")
    combined_idx = np.lexsort((coords[:, 1], coords[:, 0]))
    coords_ordenados = coords[combined_idx]
    
    # Paso 2: Muestreo estratificado
    print("  Paso 2: Muestreo estratificado...")
    
    # Dividir en chunks y muestrear de cada uno
    chunk_size = len(coords) // 10  # 10 chunks
    sample_size = min(100000, len(coords) // 10)  # 10% del dataset
    
    coords_muestra = []
    for i in range(0, len(coords_ordenados), chunk_size):
        chunk = coords_ordenados[i:i+chunk_size]
        if len(chunk) > sample_size // 10:
            # Muestrear del chunk
            chunk_sample = chunk[np.random.choice(len(chunk), sample_size // 10, replace=False)]
            coords_muestra.append(chunk_sample)
        else:
            coords_muestra.append(chunk)
    
    coords_muestra = np.vstack(coords_muestra)
    print(f"  Muestra estratificada: {len(coords_muestra):,} puntos")
    
    # Paso 3: Mini-Batch K-Means en la muestra
    print("  Paso 3: Aplicando Mini-Batch K-Means...")
    
    kmeans = MiniBatchKMeans(
        n_clusters=n_clusters_objetivo,
        batch_size=10000,
        random_state=42,
        n_init=3,
        max_iter=100
    )
    
    labels_muestra = kmeans.fit_predict(coords_muestra)
    centroids = kmeans.cluster_centers_
    
    # Paso 4: Asignar clusters al dataset completo
    print("  Paso 4: Asignando clusters al dataset completo...")
    
    # Usar NearestNeighbors para asignar clusters
    nn = NearestNeighbors(n_neighbors=1, algorithm='ball_tree')
    nn.fit(centroids)
    
    # Procesar en chunks para evitar problemas de memoria
    chunk_size = 100000
    all_labels = []
    
    for i in range(0, len(coords), chunk_size):
        chunk = coords[i:i+chunk_size]
        distances, indices = nn.kneighbors(chunk)
        all_labels.extend(indices.flatten())
        
        if (i // chunk_size + 1) % 10 == 0:
            print(f"    Procesados {i + len(chunk):,} de {len(coords):,} puntos...")
    
    labels = np.array(all_labels)
    
    # Reordenar al orden original
    labels_original = np.zeros_like(labels)
    labels_original[combined_idx] = labels
    
    return labels_original, centroids, n_clusters_objetivo

# APLICAR ESTRATEGIAS
print("\n=== APLICANDO ESTRATEGIAS ===")

coords = df[['pickup_latitude', 'pickup_longitude']].values
print(f"Coordenadas: {coords.shape}")

# Calcular número de clusters basado en el tamaño del dataset
n_clusters_objetivo = min(100, max(10, len(coords) // 10000))  # 1 cluster por 10k puntos
print(f"Número de clusters objetivo: {n_clusters_objetivo}")

# Aplicar las tres estrategias
print("\n--- Estrategia 1: Ordenamiento + Mini-Batch K-Means ---")
labels_kmeans, centroids_kmeans, n_clusters_kmeans = ordenar_y_clustering_espacial(
    coords, n_clusters_objetivo, batch_size=10000
)

print("\n--- Estrategia 2: Grid-Based Adaptativo ---")
labels_grid, centroids_grid, n_clusters_grid = grid_based_adaptativo(
    coords, n_clusters_objetivo
)

print("\n--- Estrategia 3: Híbrida (Recomendada) ---")
labels_hibrido, centroids_hibrido, n_clusters_hibrido = clustering_hibrido(
    coords, n_clusters_objetivo
)

# COMPARAR RESULTADOS
print("\n=== COMPARACIÓN DE ESTRATEGIAS ===")

estrategias = {
    'Mini-Batch K-Means': (labels_kmeans, n_clusters_kmeans),
    'Grid-Based Adaptativo': (labels_grid, n_clusters_grid),
    'Híbrida': (labels_hibrido, n_clusters_hibrido)
}

for nombre, (labels, n_clusters) in estrategias.items():
    n_noise = (labels == -1).sum() if -1 in labels else 0
    n_clustered = len(labels) - n_noise
    
    print(f"\n{nombre}:")
    print(f"  Clusters: {n_clusters}")
    print(f"  Puntos en clusters: {n_clustered:,} ({n_clustered/len(labels)*100:.1f}%)")
    print(f"  Puntos de ruido: {n_noise:,} ({n_noise/len(labels)*100:.1f}%)")

# SELECCIONAR MEJOR ESTRATEGIA
print("\n=== SELECCIONANDO MEJOR ESTRATEGIA ===")

# Criterio: menos ruido y número de clusters cercano al objetivo
def evaluar_estrategia(labels, n_clusters, objetivo):
    n_noise = (labels == -1).sum() if -1 in labels else 0
    n_clustered = len(labels) - n_noise
    ruido_percent = n_noise / len(labels) * 100
    diferencia_clusters = abs(n_clusters - objetivo)
    
    # Score: menos ruido es mejor, clusters cercanos al objetivo es mejor
    score = ruido_percent + diferencia_clusters * 0.1
    return score

scores = {}
for nombre, (labels, n_clusters) in estrategias.items():
    score = evaluar_estrategia(labels, n_clusters, n_clusters_objetivo)
    scores[nombre] = score
    print(f"  {nombre}: Score = {score:.2f}")

mejor_estrategia = min(scores.keys(), key=lambda x: scores[x])
print(f"\nMejor estrategia: {mejor_estrategia}")

# APLICAR RESULTADO FINAL
if mejor_estrategia == 'Mini-Batch K-Means':
    labels_final = labels_kmeans
    centroids_final = centroids_kmeans
    n_clusters_final = n_clusters_kmeans
elif mejor_estrategia == 'Grid-Based Adaptativo':
    labels_final = labels_grid
    centroids_final = centroids_grid
    n_clusters_final = n_clusters_grid
else:  # Híbrida
    labels_final = labels_hibrido
    centroids_final = centroids_hibrido
    n_clusters_final = n_clusters_hibrido

df_final = df.copy()
df_final['cluster_label'] = labels_final

# VISUALIZACIÓN
print("\n=== GENERANDO VISUALIZACIONES ===")

# Usar muestra para visualización
if len(df_final) > 100000:
    df_viz = df_final.sample(n=100000, random_state=42)
else:
    df_viz = df_final

# Gráfica comparativa
fig, axes = plt.subplots(2, 2, figsize=(20, 16))

# Estrategia 1
ax1 = axes[0, 0]
labels_viz = labels_kmeans[:len(df_viz)] if len(df_final) > 100000 else labels_kmeans
scatter1 = ax1.scatter(df_viz['pickup_longitude'], df_viz['pickup_latitude'], 
                      c=labels_viz, cmap='tab20', alpha=0.6, s=0.5)
ax1.set_title(f'Mini-Batch K-Means\n{n_clusters_kmeans} clusters', fontsize=12)
ax1.set_xlabel('Longitud')
ax1.set_ylabel('Latitud')
ax1.grid(True, alpha=0.3)

# Estrategia 2
ax2 = axes[0, 1]
labels_viz = labels_grid[:len(df_viz)] if len(df_final) > 100000 else labels_grid
scatter2 = ax2.scatter(df_viz['pickup_longitude'], df_viz['pickup_latitude'], 
                      c=labels_viz, cmap='tab20', alpha=0.6, s=0.5)
ax2.set_title(f'Grid-Based Adaptativo\n{n_clusters_grid} clusters', fontsize=12)
ax2.set_xlabel('Longitud')
ax2.set_ylabel('Latitud')
ax2.grid(True, alpha=0.3)

# Estrategia 3 (Mejor)
ax3 = axes[1, 0]
labels_viz = labels_hibrido[:len(df_viz)] if len(df_final) > 100000 else labels_hibrido
scatter3 = ax3.scatter(df_viz['pickup_longitude'], df_viz['pickup_latitude'], 
                      c=labels_viz, cmap='tab20', alpha=0.6, s=0.5)
ax3.set_title(f'Híbrida (Recomendada)\n{n_clusters_hibrido} clusters', fontsize=12)
ax3.set_xlabel('Longitud')
ax3.set_ylabel('Latitud')
ax3.grid(True, alpha=0.3)

# Comparación de estadísticas
ax4 = axes[1, 1]
estrategias_nombres = list(estrategias.keys())
clusters_counts = [estrategias[estrategia][1] for estrategia in estrategias_nombres]
noise_percentages = [(estrategias[estrategia][0] == -1).sum() / len(estrategias[estrategia][0]) * 100 
                    for estrategia in estrategias_nombres]

x = np.arange(len(estrategias_nombres))
width = 0.35

ax4.bar(x - width/2, clusters_counts, width, label='Número de Clusters', alpha=0.8)
ax4_twin = ax4.twinx()
ax4_twin.bar(x + width/2, noise_percentages, width, label='% Ruido', alpha=0.8, color='orange')

ax4.set_xlabel('Estrategias')
ax4.set_ylabel('Número de Clusters')
ax4_twin.set_ylabel('% de Puntos de Ruido')
ax4.set_title('Comparación de Estrategias')
ax4.set_xticks(x)
ax4.set_xticklabels(estrategias_nombres, rotation=45)
ax4.legend(loc='upper left')
ax4_twin.legend(loc='upper right')
ax4.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('3Preprocesamiento/AnalisisCoord/comparacion_clustering_estrategias.png', dpi=300, bbox_inches='tight')
plt.close()
print("Visualización guardada: comparacion_clustering_estrategias.png")

# GUARDAR RESULTADOS
print("\n=== GUARDANDO RESULTADOS ===")

output_file = '2Database/4clusters_optimizado_completo.csv'
df_final.to_csv(output_file, index=False)
print(f"Dataset con clusters guardado: {output_file}")

# Guardar centroides
centroids_df = pd.DataFrame(centroids_final, columns=['centroid_lat', 'centroid_lon'])
centroids_df['cluster_id'] = range(len(centroids_final))
centroids_file = '2Database/4clusters_optimizado_centroides.csv'
centroids_df.to_csv(centroids_file, index=False)
print(f"Centroides guardados: {centroids_file}")

print(f"\n=== RESUMEN FINAL ===")
print(f"Dataset: {len(df):,} puntos")
print(f"Estrategia seleccionada: {mejor_estrategia}")
print(f"Clusters encontrados: {n_clusters_final}")
print(f"Puntos en clusters: {(labels_final != -1).sum():,}")
print(f"Puntos de ruido: {(labels_final == -1).sum():,}")

print(f"\n¡Clustering optimizado completado!") 