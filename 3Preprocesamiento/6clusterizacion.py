import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.neighbors import NearestNeighbors
from sklearn.cluster import DBSCAN
from scipy.spatial.distance import pdist
import seaborn as sns
import gc
import warnings
warnings.filterwarnings('ignore')

# Configurar pandas para mantener alta precisión
pd.set_option('display.float_format', '{:.14f}'.format)
pd.set_option('display.precision', 13)

print("=== APLICACIÓN COMPLETA DE DBSCAN ===")

# Cargar datos PCA
print("Cargando datos PCA...")
try:
    df = pd.read_csv('2Database/3pca_data_set_complete.csv')
    print(f"Dataset PCA shape: {df.shape}")
    print(f"Columnas: {list(df.columns)}")
except FileNotFoundError:
    print("Archivo PCA no encontrado, intentando con datos procesados...")
    df = pd.read_csv('2Database/processed_data_complete.csv')
    print(f"Dataset procesado shape: {df.shape}")
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

# Para datasets muy grandes, usar estrategia de procesamiento por chunks
if len(df) > 1000000:  # Más de 1 millón de puntos
    print(f"Dataset extremadamente grande ({len(df):,} puntos). Aplicando estrategia de procesamiento por chunks...")
    
    # Estrategia 1: Usar muestra representativa para clustering inicial
    print("Usando muestra representativa para clustering inicial...")
    sample_size = min(500000, len(df) // 20)  # 5% del dataset o 500k máximo
    df_sample_cluster = df.sample(n=sample_size, random_state=42)
    
    coords_sample_cluster = df_sample_cluster[['lat', 'long']].values
    print(f"Aplicando DBSCAN a muestra de {len(coords_sample_cluster):,} puntos...")
    
    # Aplicar DBSCAN a la muestra
    dbscan = DBSCAN(eps=eps_recommended, min_samples=5, metric='euclidean', n_jobs=-1)
    labels_sample = dbscan.fit_predict(coords_sample_cluster)
    
    # Análisis de resultados de la muestra
    n_clusters_sample = len(set(labels_sample)) - (1 if -1 in labels_sample else 0)
    n_noise_sample = int((labels_sample == -1).sum())
    
    print(f"\nResultados DBSCAN (muestra):")
    print(f"  Número de clusters: {n_clusters_sample}")
    print(f"  Puntos de ruido: {n_noise_sample:,} ({n_noise_sample/len(coords_sample_cluster)*100:.1f}%)")
    print(f"  Puntos en clusters: {len(coords_sample_cluster) - n_noise_sample:,} ({(len(coords_sample_cluster) - n_noise_sample)/len(coords_sample_cluster)*100:.1f}%)")
    
    # Agregar labels a la muestra
    df_sample_cluster['cluster_label'] = labels_sample
    
    # Estrategia 2: Asignar clusters al resto de puntos usando KNN
    print("\nAsignando clusters al resto de puntos usando KNN...")
    
    # Separar puntos con cluster y sin cluster
    df_clustered = df_sample_cluster[df_sample_cluster['cluster_label'] != -1]
    df_unclustered = df[~df.index.isin(df_sample_cluster.index)]
    
    if len(df_clustered) > 0 and len(df_unclustered) > 0:
        # Entrenar KNN con puntos clusterizados
        coords_clustered = df_clustered[['lat', 'long']].values
        labels_clustered = df_clustered['cluster_label'].values
        
        # Usar KNN para asignar clusters
        knn = NearestNeighbors(n_neighbors=1, algorithm='ball_tree', n_jobs=-1)
        knn.fit(coords_clustered)
        
        # Procesar en chunks para evitar problemas de memoria
        chunk_size = 100000
        all_labels = []
        
        for i in range(0, len(df_unclustered), chunk_size):
            chunk = df_unclustered.iloc[i:i+chunk_size]
            coords_chunk = chunk[['lat', 'long']].values
            
            # Encontrar vecino más cercano
            distances, indices = knn.kneighbors(coords_chunk)
            
            # Asignar cluster del vecino más cercano
            chunk_labels = labels_clustered[indices.flatten()]
            
            # Verificar si está dentro del radio eps
            mask_within_eps = distances.flatten() <= eps_recommended
            chunk_labels[~mask_within_eps] = -1  # Marcar como ruido si está muy lejos
            
            all_labels.extend(chunk_labels)
            
            if (i // chunk_size + 1) % 10 == 0:
                print(f"  Procesados {i + len(chunk):,} de {len(df_unclustered):,} puntos...")
        
        # Combinar resultados
        df_unclustered['cluster_label'] = all_labels
        df_final = pd.concat([df_sample_cluster, df_unclustered], ignore_index=True)
        
    else:
        # Si no hay puntos clusterizados, marcar todo como ruido
        df_final = df.copy()
        df_final['cluster_label'] = -1
    
    # Ordenar por índice original
    df_final = df_final.sort_index()
    
else:
    # Para datasets más pequeños, usar DBSCAN completo
    coords_full = df[['lat', 'long']].values
    print(f"Aplicando DBSCAN a {len(coords_full):,} puntos...")
    
    # Aplicar DBSCAN
    dbscan = DBSCAN(eps=eps_recommended, min_samples=5, metric='euclidean', n_jobs=-1)
    labels = dbscan.fit_predict(coords_full)
    
    # Agregar labels al DataFrame
    df['cluster_label'] = labels
    df_final = df

# Análisis de resultados finales
labels_final = df_final['cluster_label'].values
n_clusters = len(set(labels_final)) - (1 if -1 in labels_final else 0)
n_noise = int((labels_final == -1).sum())

print(f"\nResultados DBSCAN (finales):")
print(f"  Número de clusters: {n_clusters}")
print(f"  Puntos de ruido: {n_noise:,} ({n_noise/len(labels_final)*100:.1f}%)")
print(f"  Puntos en clusters: {len(labels_final) - n_noise:,} ({(len(labels_final) - n_noise)/len(labels_final)*100:.1f}%)")

# PASO 3: Análisis de clusters
print(f"\n=== PASO 3: ANÁLISIS DE CLUSTERS ===")

# Estadísticas por cluster
print("\nEstadísticas por cluster:")
cluster_stats = df_final.groupby('cluster_label').agg({
    'lat': ['count', 'min', 'max', 'mean'],
    'long': ['min', 'max', 'mean']
}).round(6)

print(cluster_stats)

# Tamaño de cada cluster
print(f"\nTamaño de cada cluster:")
cluster_sizes = df_final['cluster_label'].value_counts().sort_index()
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

# Para datasets grandes, usar muestra para visualización
if len(df_final) > 100000:
    print("Dataset grande, usando muestra para visualización...")
    df_viz = df_final.sample(n=100000, random_state=42)
else:
    df_viz = df_final

# Gráfica 1: Todos los clusters en un mapa
print("Generando mapa de todos los clusters...")
plt.figure(figsize=(15, 10))

# Crear colores para cada cluster - sistema cíclico para muchos clusters
n_clusters_actual = len(set(labels_final)) - (1 if -1 in labels_final else 0)

# Crear paleta de colores que se repita cíclicamente
base_colors = plt.cm.Set3(np.linspace(0, 1, 12))  # 12 colores base
if n_clusters_actual > 12:
    # Repetir colores si hay más de 12 clusters
    colors = np.tile(base_colors, (n_clusters_actual // 12 + 1, 1))[:n_clusters_actual]
else:
    colors = base_colors[:n_clusters_actual]

# Crear mapeo de cluster_id a índice de color
cluster_ids_sorted = sorted([cid for cid in set(labels_final) if cid != -1])
cluster_to_color_idx = {cid: idx for idx, cid in enumerate(cluster_ids_sorted)}

# Graficar cada cluster
for i, cluster_id in enumerate(sorted(set(labels_final))):
    if cluster_id == -1:
        # Ruido en negro
        cluster_points = df_viz[df_viz['cluster_label'] == cluster_id]
        plt.scatter(cluster_points['long'], cluster_points['lat'], 
                   alpha=0.1, s=0.1, color='black', marker='.', label=f'Ruido ({len(cluster_points):,})')
    else:
        # Clusters en colores
        cluster_points = df_viz[df_viz['cluster_label'] == cluster_id]
        color_idx = cluster_to_color_idx[cluster_id]
        plt.scatter(cluster_points['long'], cluster_points['lat'], 
                   alpha=0.3, s=0.5, color=colors[color_idx], marker='.', 
                   label=f'Cluster {cluster_id} ({len(cluster_points):,})')

plt.title(f'Clusters DBSCAN - {n_clusters} clusters, {n_noise:,} puntos de ruido', fontsize=16, fontweight='bold')
plt.xlabel('Longitud', fontsize=13)
plt.ylabel('Latitud', fontsize=13)
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
df_clusters_viz = df_viz[df_viz['cluster_label'] != -1]

for cluster_id in sorted(df_clusters_viz['cluster_label'].unique()):
    cluster_points = df_clusters_viz[df_clusters_viz['cluster_label'] == cluster_id]
    color_idx = cluster_to_color_idx[cluster_id]
    plt.scatter(cluster_points['long'], cluster_points['lat'], 
               alpha=0.4, s=1, color=colors[color_idx], marker='.', 
               label=f'Cluster {cluster_id} ({len(cluster_points):,})')

plt.title(f'Clusters Principales DBSCAN - {n_clusters} clusters', fontsize=16, fontweight='bold')
plt.xlabel('Longitud', fontsize=13)
plt.ylabel('Latitud', fontsize=13)
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
# Usar colores correspondientes para cada cluster en el gráfico de barras
bar_colors = [colors[cluster_to_color_idx.get(cid, 0)] for cid in cluster_sizes_no_noise.index]
plt.bar(range(len(cluster_sizes_no_noise)), cluster_sizes_no_noise.to_numpy(), color=bar_colors)
plt.xlabel('ID del Cluster')
plt.ylabel('Número de Puntos')
plt.title('Tamaño de Clusters')
plt.xticks(range(len(cluster_sizes_no_noise)), [str(i) for i in cluster_sizes_no_noise.index])
plt.grid(True, alpha=0.3)

plt.subplot(1, 2, 2)
plt.pie(cluster_sizes_no_noise.to_numpy(), labels=[f'Cluster {i}' for i in cluster_sizes_no_noise.index], 
        autopct='%1.1f%%', startangle=90)
plt.title('Distribución de Clusters')

plt.tight_layout()
plt.savefig('Preprocesamiento/AnalisisDBSCAN/distribucion_clusters.png', dpi=300, bbox_inches='tight')
plt.close()
print("Distribución de clusters guardado: distribucion_clusters.png")

# PASO 5: Análisis de componentes PCA por cluster
print(f"\n=== PASO 5: ANÁLISIS DE COMPONENTES PCA POR CLUSTER ===")

pca_columns = [col for col in df_final.columns if col.startswith('PC')]
if len(pca_columns) > 0:
    print(f"Componentes PCA disponibles: {pca_columns}")
    
    # Estadísticas PCA por cluster
    print(f"\nEstadísticas PCA por cluster:")
    pca_stats = df_final.groupby('cluster_label')[pca_columns].agg(['mean', 'std']).round(4)
    print(pca_stats)
    
    # Visualizar componentes PCA por cluster
    print("Generando gráfica de componentes PCA por cluster...")
    plt.figure(figsize=(15, 10))
    
    for i, pc in enumerate(pca_columns[:4]):  # Solo primeras 4 componentes
        plt.subplot(2, 2, i+1)
        
        # Box plot por cluster
        df_clusters = df_final[df_final['cluster_label'] != -1]  # Sin ruido
        cluster_ids = sorted(df_clusters['cluster_label'].unique())
        
        data_to_plot = [df_clusters[df_clusters['cluster_label'] == cid][pc].to_numpy() 
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
df_final.to_csv(output_file, index=False)
print(f"Dataset con clusters guardado: {output_file}")

# Resumen final
print(f"\n=== RESUMEN FINAL ===")
print(f"Dataset original: {len(df):,} puntos")
print(f"EPS usado: {eps_recommended:.6f}")
print(f"Clusters encontrados: {n_clusters}")
print(f"Puntos en clusters: {len(labels_final) - n_noise:,} ({(len(labels_final) - n_noise)/len(labels_final)*100:.1f}%)")
print(f"Puntos de ruido: {n_noise:,} ({n_noise/len(labels_final)*100:.1f}%)")

print(f"\nArchivos generados:")
print(f"  - clusters_dbscan_completo.png (todos los clusters)")
print(f"  - clusters_principales.png (solo clusters, sin ruido)")
print(f"  - distribucion_clusters.png (estadísticas)")
print(f"  - pca_por_cluster.png (análisis PCA)")
print(f"  - 4clusters_dbscan_complete.csv (datos con clusters)")

print(f"\n¡DBSCAN completado! Ahora puedes ver los grupos en las imágenes generadas.") 