import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.neighbors import NearestNeighbors
from sklearn.cluster import DBSCAN
from scipy.spatial.distance import pdist
import seaborn as sns

print("=== APLICACIÓN COMPLETA DE DBSCAN ===")

# Cargar datos PCA
print("Cargando datos PCA...")
df = pd.read_csv('Database/3pca_data_set_complete.csv')

print(f"Dataset PCA shape: {df.shape}")
print(f"Columnas: {list(df.columns)}")

# PASO 1: Calcular eps usando muestra
print("\n=== PASO 1: CALCULANDO EPS ===")

# Usar muestra para cálculos
if len(df) > 100000:
    print(f"Dataset muy grande ({len(df):,} registros). Usando muestra de 50,000 para análisis...")
    df_sample = df.sample(n=50000, random_state=42)
else:
    df_sample = df

coords_sample = df_sample[['lat', 'long']].values

# Calcular eps usando KNN
print("Calculando eps usando K-Nearest Neighbors...")
k = 10
nbrs = NearestNeighbors(n_neighbors=k+1, algorithm='ball_tree').fit(coords_sample)
distances, indices = nbrs.kneighbors(coords_sample)
k_distances = distances[:, k]

# Calcular eps recomendado
eps_recommended = np.percentile(k_distances, 90)
print(f"EPS recomendado (percentil 90, k=10): {eps_recommended:.6f}")

# PASO 2: Aplicar DBSCAN a TODO el dataset
print(f"\n=== PASO 2: APLICANDO DBSCAN A TODO EL DATASET ===")

# Preparar coordenadas completas
coords_full = df[['lat', 'long']].values
print(f"Aplicando DBSCAN a {len(coords_full):,} puntos...")

# Aplicar DBSCAN
dbscan = DBSCAN(eps=eps_recommended, min_samples=5, metric='euclidean')
labels = dbscan.fit_predict(coords_full)

# Análisis de resultados
n_clusters = len(set(labels)) - (1 if -1 in labels else 0)
n_noise = list(labels).count(-1)

print(f"\nResultados DBSCAN:")
print(f"  Número de clusters: {n_clusters}")
print(f"  Puntos de ruido: {n_noise:,} ({n_noise/len(coords_full)*100:.1f}%)")
print(f"  Puntos en clusters: {len(coords_full) - n_noise:,} ({(len(coords_full) - n_noise)/len(coords_full)*100:.1f}%)")

# Agregar labels al DataFrame
df['cluster_label'] = labels

# PASO 3: Análisis de clusters
print(f"\n=== PASO 3: ANÁLISIS DE CLUSTERS ===")

# Estadísticas por cluster
print("\nEstadísticas por cluster:")
cluster_stats = df.groupby('cluster_label').agg({
    'lat': ['count', 'min', 'max', 'mean'],
    'long': ['min', 'max', 'mean']
}).round(6)

print(cluster_stats)

# Tamaño de cada cluster
print(f"\nTamaño de cada cluster:")
cluster_sizes = df['cluster_label'].value_counts().sort_index()
for cluster_id, size in cluster_sizes.items():
    if cluster_id == -1:
        print(f"  Ruido (cluster -1): {size:,} puntos")
    else:
        print(f"  Cluster {cluster_id}: {size:,} puntos")

# PASO 4: Visualización de clusters
print(f"\n=== PASO 4: GENERANDO VISUALIZACIONES ===")

# Configurar matplotlib
plt.rcParams['figure.dpi'] = 100
plt.rcParams['savefig.dpi'] = 300

# Gráfica 1: Todos los clusters en un mapa
print("Generando mapa de todos los clusters...")
plt.figure(figsize=(15, 10))

# Crear colores para cada cluster
n_clusters_actual = len(set(labels)) - (1 if -1 in labels else 0)
colors = plt.cm.tab20(np.linspace(0, 1, n_clusters_actual))

# Graficar cada cluster
for i, cluster_id in enumerate(sorted(set(labels))):
    if cluster_id == -1:
        # Ruido en negro
        cluster_points = df[df['cluster_label'] == cluster_id]
        plt.scatter(cluster_points['long'], cluster_points['lat'], 
                   alpha=0.1, s=0.1, color='black', marker='.', label=f'Ruido ({len(cluster_points):,})')
    else:
        # Clusters en colores
        cluster_points = df[df['cluster_label'] == cluster_id]
        plt.scatter(cluster_points['long'], cluster_points['lat'], 
                   alpha=0.3, s=0.5, color=colors[i], marker='.', 
                   label=f'Cluster {cluster_id} ({len(cluster_points):,})')

plt.title(f'Clusters DBSCAN - {n_clusters} clusters, {n_noise:,} puntos de ruido', fontsize=16, fontweight='bold')
plt.xlabel('Longitud', fontsize=14)
plt.ylabel('Latitud', fontsize=14)
plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
plt.grid(True, alpha=0.3)

# Agregar líneas de referencia para NYC
plt.axhline(y=40.7128, color='red', linestyle='--', alpha=0.7, label='NYC Lat')
plt.axvline(x=-74.0060, color='red', linestyle='--', alpha=0.7, label='NYC Long')

plt.tight_layout()
plt.savefig('Preprocesamiento/AnalisisDBSCAN/clusters_dbscan_completo.png', dpi=300, bbox_inches='tight')
plt.close()
print("Mapa completo guardado: clusters_dbscan_completo.png")

# Gráfica 2: Solo clusters principales (sin ruido)
print("Generando mapa de clusters principales...")
plt.figure(figsize=(15, 10))

# Filtrar solo clusters (sin ruido)
df_clusters = df[df['cluster_label'] != -1]

for i, cluster_id in enumerate(sorted(df_clusters['cluster_label'].unique())):
    cluster_points = df_clusters[df_clusters['cluster_label'] == cluster_id]
    plt.scatter(cluster_points['long'], cluster_points['lat'], 
               alpha=0.4, s=1, color=colors[i], marker='.', 
               label=f'Cluster {cluster_id} ({len(cluster_points):,})')

plt.title(f'Clusters Principales DBSCAN - {n_clusters} clusters', fontsize=16, fontweight='bold')
plt.xlabel('Longitud', fontsize=14)
plt.ylabel('Latitud', fontsize=14)
plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
plt.grid(True, alpha=0.3)

# Agregar líneas de referencia para NYC
plt.axhline(y=40.7128, color='red', linestyle='--', alpha=0.7, label='NYC Lat')
plt.axvline(x=-74.0060, color='red', linestyle='--', alpha=0.7, label='NYC Long')

plt.tight_layout()
plt.savefig('Preprocesamiento/AnalisisDBSCAN/clusters_principales.png', dpi=300, bbox_inches='tight')
plt.close()
print("Mapa de clusters principales guardado: clusters_principales.png")

# Gráfica 3: Distribución de tamaños de clusters
print("Generando gráfica de distribución de clusters...")
plt.figure(figsize=(12, 6))

# Filtrar clusters (sin ruido)
cluster_sizes_no_noise = cluster_sizes[cluster_sizes.index != -1]

plt.subplot(1, 2, 1)
plt.bar(range(len(cluster_sizes_no_noise)), cluster_sizes_no_noise.values, color=colors[:len(cluster_sizes_no_noise)])
plt.xlabel('ID del Cluster')
plt.ylabel('Número de Puntos')
plt.title('Tamaño de Clusters')
plt.xticks(range(len(cluster_sizes_no_noise)), cluster_sizes_no_noise.index)
plt.grid(True, alpha=0.3)

plt.subplot(1, 2, 2)
plt.pie(cluster_sizes_no_noise.values, labels=[f'Cluster {i}' for i in cluster_sizes_no_noise.index], 
        autopct='%1.1f%%', startangle=90)
plt.title('Distribución de Clusters')

plt.tight_layout()
plt.savefig('Preprocesamiento/AnalisisDBSCAN/distribucion_clusters.png', dpi=300, bbox_inches='tight')
plt.close()
print("Distribución de clusters guardado: distribucion_clusters.png")

# PASO 5: Análisis de componentes PCA por cluster
print(f"\n=== PASO 5: ANÁLISIS DE COMPONENTES PCA POR CLUSTER ===")

pca_columns = [col for col in df.columns if col.startswith('PC')]
if len(pca_columns) > 0:
    print(f"Componentes PCA disponibles: {pca_columns}")
    
    # Estadísticas PCA por cluster
    print(f"\nEstadísticas PCA por cluster:")
    pca_stats = df.groupby('cluster_label')[pca_columns].agg(['mean', 'std']).round(4)
    print(pca_stats)
    
    # Visualizar componentes PCA por cluster
    print("Generando gráfica de componentes PCA por cluster...")
    plt.figure(figsize=(15, 10))
    
    for i, pc in enumerate(pca_columns[:4]):  # Solo primeras 4 componentes
        plt.subplot(2, 2, i+1)
        
        # Box plot por cluster
        df_clusters = df[df['cluster_label'] != -1]  # Sin ruido
        cluster_ids = sorted(df_clusters['cluster_label'].unique())
        
        data_to_plot = [df_clusters[df_clusters['cluster_label'] == cid][pc].values 
                       for cid in cluster_ids]
        
        plt.boxplot(data_to_plot, labels=[f'C{cid}' for cid in cluster_ids])
        plt.title(f'{pc} por Cluster')
        plt.ylabel('Valor')
        plt.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('Preprocesamiento/AnalisisDBSCAN/pca_por_cluster.png', dpi=300, bbox_inches='tight')
    plt.close()
    print("PCA por cluster guardado: pca_por_cluster.png")

# PASO 6: Guardar resultados
print(f"\n=== PASO 6: GUARDANDO RESULTADOS ===")

# Guardar DataFrame con clusters
output_file = 'Database/4clusters_dbscan_complete.csv'
df.to_csv(output_file, index=False)
print(f"Dataset con clusters guardado: {output_file}")

# Resumen final
print(f"\n=== RESUMEN FINAL ===")
print(f"Dataset original: {len(df):,} puntos")
print(f"EPS usado: {eps_recommended:.6f}")
print(f"Clusters encontrados: {n_clusters}")
print(f"Puntos en clusters: {len(df) - n_noise:,} ({(len(df) - n_noise)/len(df)*100:.1f}%)")
print(f"Puntos de ruido: {n_noise:,} ({n_noise/len(df)*100:.1f}%)")

print(f"\nArchivos generados:")
print(f"  - clusters_dbscan_completo.png (todos los clusters)")
print(f"  - clusters_principales.png (solo clusters, sin ruido)")
print(f"  - distribucion_clusters.png (estadísticas)")
print(f"  - pca_por_cluster.png (análisis PCA)")
print(f"  - 4clusters_dbscan_complete.csv (datos con clusters)")

print(f"\n¡DBSCAN completado! Ahora puedes ver los grupos en las imágenes generadas.") 