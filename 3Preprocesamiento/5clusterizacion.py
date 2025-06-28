import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import gc
import warnings
warnings.filterwarnings('ignore')

# Configurar pandas para mantener alta precisión
pd.set_option('display.float_format', '{:.14f}'.format)
pd.set_option('display.precision', 13)

print("=== CLUSTERING GRID-BASED EFICIENTE PARA 10M PUNTOS ===")

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

# FUNCIÓN PRINCIPAL: GRID-BASED CLUSTERING
def grid_based_clustering(coords, grid_size=0.01, min_points=5):
    """
    Clustering basado en grid para datasets muy grandes
    """
    print(f"Aplicando Grid-Based Clustering...")
    print(f"  Grid size: {grid_size}")
    print(f"  Min points per cell: {min_points}")
    
    # Crear grid bins
    lat_bins = np.arange(coords[:, 0].min(), coords[:, 0].max() + grid_size, grid_size)
    lon_bins = np.arange(coords[:, 1].min(), coords[:, 1].max() + grid_size, grid_size)
    
    print(f"  Grid: {len(lat_bins)}x{len(lon_bins)} = {len(lat_bins)*len(lon_bins)} celdas")
    
    # Asignar cada punto a un bin del grid
    lat_indices = np.digitize(coords[:, 0], lat_bins) - 1
    lon_indices = np.digitize(coords[:, 1], lon_bins) - 1
    
    # Crear identificador único para cada celda del grid
    grid_ids = lat_indices * len(lon_bins) + lon_indices
    
    # Contar puntos por celda
    unique_grids, counts = np.unique(grid_ids, return_counts=True)
    
    # Filtrar celdas con suficientes puntos (clusters)
    valid_grids = unique_grids[counts >= min_points]
    
    print(f"  Celdas con suficientes puntos: {len(valid_grids)}")
    print(f"  Puntos en celdas válidas: {counts[counts >= min_points].sum():,}")
    
    # Asignar cluster labels
    cluster_labels = np.full(len(coords), -1)  # -1 = ruido
    
    for i, grid_id in enumerate(valid_grids):
        mask = grid_ids == grid_id
        cluster_labels[mask] = i
    
    return cluster_labels, len(valid_grids), grid_ids

# APLICAR CLUSTERING
print("\n=== APLICANDO GRID-BASED CLUSTERING ===")

coords = df[['pickup_latitude', 'pickup_longitude']].values
print(f"Coordenadas: {coords.shape}")

# Probar diferentes tamaños de grid
grid_sizes = [0.1, 0.05, 0.01, 0.005]
results = {}

for grid_size in grid_sizes:
    print(f"\n--- Probando grid_size = {grid_size} ---")
    
    # Ajustar min_points según el tamaño del grid
    min_points = max(5, int(1000 * grid_size * grid_size))  # Más puntos para grids más grandes
    
    labels, n_clusters, grid_ids = grid_based_clustering(coords, grid_size, min_points)
    
    n_noise = (labels == -1).sum()
    n_clustered = len(labels) - n_noise
    
    results[grid_size] = {
        'labels': labels,
        'n_clusters': n_clusters,
        'n_noise': n_noise,
        'n_clustered': n_clustered,
        'noise_percent': n_noise / len(labels) * 100,
        'grid_ids': grid_ids
    }
    
    print(f"  Resultados:")
    print(f"    Clusters: {n_clusters}")
    print(f"    Puntos en clusters: {n_clustered:,} ({n_clustered/len(labels)*100:.1f}%)")
    print(f"    Puntos de ruido: {n_noise:,} ({n_noise/len(labels)*100:.1f}%)")

# SELECCIONAR MEJOR RESULTADO
print("\n=== SELECCIONANDO MEJOR RESULTADO ===")

# Criterio: balance entre número de clusters y porcentaje de ruido
best_grid_size = min(results.keys(), key=lambda x: results[x]['noise_percent'])
best_result = results[best_grid_size]

print(f"Mejor grid_size: {best_grid_size}")
print(f"Clusters: {best_result['n_clusters']}")
print(f"Puntos en clusters: {best_result['n_clustered']:,} ({best_result['n_clustered']/len(coords)*100:.1f}%)")
print(f"Puntos de ruido: {best_result['n_noise']:,} ({best_result['noise_percent']:.1f}%)")

# APLICAR RESULTADO FINAL
df_final = df.copy()
df_final['cluster_label'] = best_result['labels']
df_final['grid_id'] = best_result['grid_ids']

# ANÁLISIS DE CLUSTERS
print("\n=== ANÁLISIS DE CLUSTERS ===")

# Estadísticas por cluster
print("\nEstadísticas por cluster:")
cluster_stats = df_final.groupby('cluster_label').agg({
    'pickup_latitude': ['count', 'min', 'max', 'mean'],
    'pickup_longitude': ['min', 'max', 'mean']
}).round(6)

print(cluster_stats.head(10))  # Mostrar solo primeros 10 clusters

# Tamaño de cada cluster
print(f"\nTamaño de cada cluster:")
cluster_sizes = df_final['cluster_label'].value_counts().sort_index()
for cluster_id, size in cluster_sizes.head(20).items():  # Mostrar solo primeros 20
    if cluster_id == -1:
        print(f"  Ruido (cluster -1): {size:,} puntos")
    else:
        print(f"  Cluster {cluster_id}: {size:,} puntos")

# VISUALIZACIÓN
print("\n=== GENERANDO VISUALIZACIONES ===")

# Usar muestra para visualización si el dataset es muy grande
if len(df_final) > 100000:
    print("Dataset grande, usando muestra para visualización...")
    df_viz = df_final.sample(n=100000, random_state=42)
else:
    df_viz = df_final

# Gráfica 1: Mapa de clusters
print("Generando mapa de clusters...")
plt.figure(figsize=(15, 10))

# Filtrar solo clusters (sin ruido) para mejor visualización
df_clusters_viz = df_viz[df_viz['cluster_label'] != -1]

if len(df_clusters_viz) > 0:
    scatter = plt.scatter(df_clusters_viz['pickup_longitude'], df_clusters_viz['pickup_latitude'], 
                         c=df_clusters_viz['cluster_label'], cmap='tab20', alpha=0.6, s=0.5)
    plt.colorbar(scatter, label='Cluster ID')
else:
    plt.scatter(df_viz['pickup_longitude'], df_viz['pickup_latitude'], 
               alpha=0.1, s=0.1, color='blue', label='Todos los puntos')

plt.title(f'Grid-Based Clustering - {best_result["n_clusters"]} clusters, {best_result["n_noise"]:,} puntos de ruido', 
          fontsize=16, fontweight='bold')
plt.xlabel('Longitud', fontsize=13)
plt.ylabel('Latitud', fontsize=13)
plt.grid(True, alpha=0.3)

# Agregar líneas de referencia para NYC
plt.axhline(y=40.7128, color='red', linestyle='--', alpha=0.7, label='NYC Lat')
plt.axvline(x=-74.0060, color='red', linestyle='--', alpha=0.7, label='NYC Long')

plt.legend()
plt.tight_layout()
plt.savefig('3Preprocesamiento/AnalisisCoord/clusters_grid_based.png', dpi=300, bbox_inches='tight')
plt.close()
print("Mapa de clusters guardado: clusters_grid_based.png")

# Gráfica 2: Comparación de diferentes tamaños de grid
print("Generando comparación de métodos...")
plt.figure(figsize=(15, 10))

# Subplot 1: Número de clusters vs grid_size
plt.subplot(2, 3, 1)
grid_sizes_list = list(results.keys())
n_clusters_list = [results[gs]['n_clusters'] for gs in grid_sizes_list]
plt.plot(grid_sizes_list, n_clusters_list, 'o-', color='blue', linewidth=2, markersize=8)
plt.xlabel('Grid Size')
plt.ylabel('Número de Clusters')
plt.title('Clusters vs Grid Size')
plt.grid(True, alpha=0.3)
plt.xscale('log')

# Subplot 2: Porcentaje de ruido vs grid_size
plt.subplot(2, 3, 2)
noise_percent_list = [results[gs]['noise_percent'] for gs in grid_sizes_list]
plt.plot(grid_sizes_list, noise_percent_list, 'o-', color='red', linewidth=2, markersize=8)
plt.xlabel('Grid Size')
plt.ylabel('% de Puntos de Ruido')
plt.title('Ruido vs Grid Size')
plt.grid(True, alpha=0.3)
plt.xscale('log')

# Subplot 3: Distribución de tamaños de clusters
plt.subplot(2, 3, 3)
cluster_sizes_no_noise = cluster_sizes[cluster_sizes.index != -1]
plt.hist(cluster_sizes_no_noise.values, bins=50, alpha=0.7, color='green', edgecolor='black')
plt.xlabel('Tamaño del Cluster')
plt.ylabel('Frecuencia')
plt.title('Distribución de Tamaños de Clusters')
plt.grid(True, alpha=0.3)

# Subplot 4: Top 20 clusters por tamaño
plt.subplot(2, 3, 4)
top_clusters = cluster_sizes_no_noise.head(20)
plt.bar(range(len(top_clusters)), top_clusters.values, color='orange', alpha=0.7)
plt.xlabel('Cluster ID')
plt.ylabel('Número de Puntos')
plt.title('Top 20 Clusters por Tamaño')
plt.xticks(range(len(top_clusters)), [str(i) for i in top_clusters.index], rotation=45)
plt.grid(True, alpha=0.3)

# Subplot 5: Análisis de componentes PCA por cluster (si existen)
pca_columns = [col for col in df_final.columns if col.startswith('PC')]
if len(pca_columns) > 0:
    plt.subplot(2, 3, 5)
    
    # Calcular media de PC1 por cluster
    pc1_means = df_final[df_final['cluster_label'] != -1].groupby('cluster_label')['PC1'].mean()
    plt.scatter(range(len(pc1_means)), pc1_means.values, alpha=0.6, color='purple')
    plt.xlabel('Cluster ID')
    plt.ylabel('Media PC1')
    plt.title('PC1 por Cluster')
    plt.grid(True, alpha=0.3)

# Subplot 6: Resumen de resultados
plt.subplot(2, 3, 6)
plt.axis('off')
summary_text = f"""
RESUMEN FINAL:
Grid Size: {best_grid_size}
Clusters: {best_result['n_clusters']}
Puntos en clusters: {best_result['n_clustered']:,}
Puntos de ruido: {best_result['n_noise']:,}
% Ruido: {best_result['noise_percent']:.1f}%
"""
plt.text(0.1, 0.5, summary_text, fontsize=12, verticalalignment='center',
         bbox=dict(boxstyle="round,pad=0.3", facecolor="lightblue", alpha=0.8))

plt.tight_layout()
plt.savefig('3Preprocesamiento/AnalisisCoord/analisis_grid_based.png', dpi=300, bbox_inches='tight')
plt.close()
print("Análisis completo guardado: analisis_grid_based.png")

# GUARDAR RESULTADOS
print("\n=== GUARDANDO RESULTADOS ===")

output_file = '2Database/4clusters_grid_based_completo.csv'
df_final.to_csv(output_file, index=False)
print(f"Dataset con clusters guardado: {output_file}")

# Guardar también un resumen de clusters
cluster_summary = df_final.groupby('cluster_label').agg({
    'pickup_latitude': ['count', 'min', 'max', 'mean'],
    'pickup_longitude': ['min', 'max', 'mean']
}).round(6)

cluster_summary_file = '2Database/4clusters_grid_based_resumen.csv'
cluster_summary.to_csv(cluster_summary_file)
print(f"Resumen de clusters guardado: {cluster_summary_file}")

print(f"\n=== RESUMEN FINAL ===")
print(f"Dataset original: {len(df):,} puntos")
print(f"Grid size seleccionado: {best_grid_size}")
print(f"Clusters encontrados: {best_result['n_clusters']}")
print(f"Puntos en clusters: {best_result['n_clustered']:,} ({best_result['n_clustered']/len(coords)*100:.1f}%)")
print(f"Puntos de ruido: {best_result['n_noise']:,} ({best_result['noise_percent']:.1f}%)")

print(f"\nArchivos generados:")
print(f"  - clusters_grid_based.png (mapa de clusters)")
print(f"  - analisis_grid_based.png (análisis completo)")
print(f"  - 4clusters_grid_based_completo.csv (datos con clusters)")
print(f"  - 4clusters_grid_based_resumen.csv (resumen de clusters)")

print(f"\n¡Grid-Based Clustering completado!") 