import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.cluster import MiniBatchKMeans, KMeans
from sklearn.neighbors import NearestNeighbors
from sklearn.decomposition import PCA
import seaborn as sns
import gc
import warnings
import matplotlib.ticker as mticker
from scipy.spatial import distance
warnings.filterwarnings('ignore')

# Configurar pandas para mantener alta precisión
pd.set_option('display.float_format', '{:.14f}'.format)
pd.set_option('display.precision', 13)

print("=== CLUSTERIZACIÓN GEOGRÁFICA PARA DATOS NYC ===")

# Cargar datos limpios de NYC
print("\nCargando datos limpios de NYC...")
try:
    df = pd.read_csv('2Database/3pca_data_set_complete.csv')
    print(f"Dataset PCA shape: {df.shape}")
    print(f"Columnas: {list(df.columns)}")
except FileNotFoundError:
    print("Archivo PCA no encontrado, usando datos limpios de NYC...")
    df = pd.read_csv('2Database/2processed_data_complete_limpio.csv')
    print(f"Dataset NYC limpio shape: {df.shape}")
    print(f"Columnas: {list(df.columns)}")

# VERIFICACIÓN RÁPIDA DE DATOS
print(f"\n=== VERIFICACIÓN DE DATOS NYC ===")

# Verificar rango NYC
lat_min, lat_max = df['pickup_latitude'].min(), df['pickup_latitude'].max()
long_min, long_max = df['pickup_longitude'].min(), df['pickup_longitude'].max()
print(f"Rango latitud: {lat_min:.6f}° - {lat_max:.6f}°")
print(f"Rango longitud: {long_min:.6f}° - {long_max:.6f}°")

# Verificar que está dentro de NYC Metro
nyc_metro_check = ((df['pickup_latitude'] >= 39.0) & (df['pickup_latitude'] <= 42.0) &
                   (df['pickup_longitude'] >= -75.0) & (df['pickup_longitude'] <= -72.0)).sum()
print(f"Puntos dentro de NYC Metro: {nyc_metro_check:,} ({nyc_metro_check/len(df)*100:.1f}%)")

# CÁLCULO OPTIMIZADO DE CLUSTERS PARA NYC
print(f"\n=== CÁLCULO OPTIMIZADO DE CLUSTERS ===")

n_points = len(df)
print(f"Dataset NYC: {n_points:,} puntos")

# Usar exactamente 55 clusters espaciales como solicitado
n_clusters_espacial = 65

print(f"Clusters espaciales fijos: {n_clusters_espacial}")
print(f"Promedio puntos por cluster espacial: {n_points // n_clusters_espacial:,}")

# Verificar que los parámetros son razonables
if n_clusters_espacial > 500:
    print(f"ADVERTENCIA: Muchos clusters espaciales ({n_clusters_espacial}). Ajustando...")
    n_clusters_espacial = 500
    print(f"Ajustado a: {n_clusters_espacial} clusters espaciales")

# === OPCIÓN DE CENTROIDES PERSONALIZADOS FIJOS ===
centroides_personalizados = [
    [40.45, -74.11]
]
usar_centroides_fijos = False  # Cambia a False para usar k-means++ normal
radio_asignacion = 0.015  # ~1km, ajustado para grupo aislado

# FUNCIÓN: CLUSTERIZACIÓN ESPACIAL OPTIMIZADA
def clusterizacion_espacial_optimizada(coords, n_clusters=100):
    """
    Clustering espacial optimizado para datos NYC limpios
    """
    print(f"\n=== CLUSTERIZACIÓN ESPACIAL OPTIMIZADA ===")
    print(f"Clusters objetivo: {n_clusters}")
    print(f"Puntos a procesar: {len(coords):,}")
    
    batch_size = min(50000, max(10000, len(coords) // 10))
    n_init = 3
    max_iter = 100
    
    print(f"  Parámetros optimizados:")
    print(f"    Batch size: {batch_size:,}")
    print(f"    Inicializaciones: {n_init}")
    print(f"    Iteraciones máximas: {max_iter}")
    
    if usar_centroides_fijos:
        # 1. Asignar manualmente los puntos cercanos a los centroides personalizados
        dists = distance.cdist(coords, np.array(centroides_personalizados))
        asignaciones = np.argmin(dists, axis=1)
        min_dists = np.min(dists, axis=1)
        mask_fijos = min_dists < radio_asignacion
        labels_fijos = asignaciones[mask_fijos]
        idx_fijos = np.where(mask_fijos)[0]
        print(f"Puntos asignados manualmente a centroides fijos: {len(idx_fijos)}")
        # 2. El resto de los puntos
        mask_restantes = ~mask_fijos
        coords_restantes = coords[mask_restantes]
        n_clusters_restantes = n_clusters - len(centroides_personalizados)
        # 3. KMeans solo en los puntos restantes
        kmeans_rest = MiniBatchKMeans(
            n_clusters=n_clusters_restantes,
            batch_size=batch_size,
            random_state=42,
            n_init=n_init,
            max_iter=max_iter,
            tol=1e-3,
            init='k-means++'
        )
        labels_rest = kmeans_rest.fit_predict(coords_restantes)
        # 4. Unir resultados
        labels_final = np.full(len(coords), -1)
        # Asignar clusters fijos (0 a len(centroides_personalizados)-1)
        for i, idx in enumerate(idx_fijos):
            labels_final[idx] = labels_fijos[i]
        # Asignar clusters del resto (empezando desde len(centroides_personalizados))
        idx_restantes = np.where(mask_restantes)[0]
        for i, idx in enumerate(idx_restantes):
            labels_final[idx] = len(centroides_personalizados) + labels_rest[i]
        # Calcular centroides finales
        centroids_fijos = np.array(centroides_personalizados)
        centroids_rest = kmeans_rest.cluster_centers_
        centroids_final = np.vstack([centroids_fijos, centroids_rest])
        inertia = kmeans_rest.inertia_  # Solo del clustering automático
        
        return labels_final, centroids_final, inertia
    else:
        # MiniBatchKMeans estándar
        kmeans_espacial = MiniBatchKMeans(
            n_clusters=n_clusters,
            batch_size=batch_size,
            random_state=42,
            n_init=n_init,
            max_iter=max_iter,
            tol=1e-3
        )
        labels_espacial = kmeans_espacial.fit_predict(coords)
        centroids_espacial = kmeans_espacial.cluster_centers_
        inertia = kmeans_espacial.inertia_
        
        return labels_espacial, centroids_espacial, inertia

# FUNCIÓN PRINCIPAL: CLUSTERIZACIÓN GEOGRÁFICA
def clusterizacion_geografica_nyc(df, n_clusters_espacial=100):
    """
    Clusterización geográfica para datos NYC limpios
    """
    print(f"\n=== CLUSTERIZACIÓN GEOGRÁFICA PARA NYC ===")
    print(f"Dataset: {len(df):,} puntos")
    print(f"Clusters espaciales: {n_clusters_espacial}")
    
    # Inicializar resultado
    df_result = df.copy()
    df_result['cluster_espacial'] = -1
    
    # CLUSTERING ESPACIAL
    print(f"\n=== CLUSTERING ESPACIAL ===")
    coords = df[['pickup_latitude', 'pickup_longitude']].values
    labels_espacial, centroids_espacial, inertia = clusterizacion_espacial_optimizada(
        coords, n_clusters_espacial
    )
    
    # Asignar clusters espaciales
    df_result['cluster_espacial'] = labels_espacial
    
    return df_result, centroids_espacial, inertia

# APLICAR CLUSTERIZACIÓN GEOGRÁFICA
print(f"\n=== APLICANDO CLUSTERIZACIÓN GEOGRÁFICA ===")

print(f"Parámetros de clustering:")
print(f"  Clusters espaciales: {n_clusters_espacial}")

# Aplicar clusterización geográfica
df_clustered, centroids_espacial, inertia_total = clusterizacion_geografica_nyc(
    df, n_clusters_espacial
)

# ANÁLISIS DE RESULTADOS
print(f"\n=== ANÁLISIS DE RESULTADOS ===")

# Estadísticas generales
n_total = len(df_clustered)
print(f"Total puntos procesados: {n_total:,}")

# Estadísticas de clusters espaciales
cluster_espacial_counts = df_clustered['cluster_espacial'].value_counts()

print(f"\nEstadísticas de clusters espaciales:")
print(f"Clusters espaciales únicos: {len(cluster_espacial_counts)}")
print(f"Promedio puntos por cluster: {cluster_espacial_counts.mean():.0f}")
print(f"Mínimo puntos por cluster: {cluster_espacial_counts.min()}")
print(f"Máximo puntos por cluster: {cluster_espacial_counts.max()}")

# VISUALIZACIÓN
print(f"\n=== GENERANDO VISUALIZACIONES ===")

# Usar TODOS los datos para visualización
df_viz = df_clustered
print(f"Generando visualización con TODOS los datos: {len(df_viz):,} puntos")

# Gráfica: Clusters espaciales
plt.figure(figsize=(15, 10))

# Filtrar datos limpios para visualización
df_viz_limpio = df_viz[df_viz['cluster_espacial'] != -1]
if len(df_viz_limpio) > 0:
    scatter = plt.scatter(df_viz_limpio['pickup_longitude'], df_viz_limpio['pickup_latitude'], 
                         c=df_viz_limpio['cluster_espacial'], cmap='tab20', alpha=0.6, s=0.5)
    plt.colorbar(scatter, label='Cluster Espacial')

# Mostrar outliers en rojo (sin coordenadas válidas, se muestran en 0,0)
df_viz_outliers = df_viz[df_viz['cluster_espacial'] == -1]
if len(df_viz_outliers) > 0:
    plt.scatter(0, 0, c='red', alpha=0.8, s=100, marker='x', 
               label=f'Outliers ({len(df_viz_outliers):,})')

ax = plt.gca()
ax.xaxis.set_major_locator(mticker.MultipleLocator(0.1))
ax.yaxis.set_major_locator(mticker.MultipleLocator(0.1))
ax.xaxis.set_major_formatter(mticker.FormatStrFormatter('%.2f'))
ax.yaxis.set_major_formatter(mticker.FormatStrFormatter('%.2f'))
for label in (ax.get_xticklabels() + ax.get_yticklabels()):
    label.set_fontsize(8)

plt.title(f'Clusters Espaciales NYC ({n_clusters_espacial} clusters)', fontsize=14)
plt.xlabel('Longitud')
plt.ylabel('Latitud')
plt.grid(True, alpha=0.3)
plt.legend()

# Gráfica: Análisis detallado
plt.figure(figsize=(15, 10))

# Subplot 1: Tamaño de clusters espaciales
plt.subplot(2, 2, 1)
cluster_sizes = df_clustered['cluster_espacial'].value_counts().sort_index()
plt.bar(range(len(cluster_sizes)), cluster_sizes.values, alpha=0.7)
plt.xlabel('Cluster Espacial ID')
plt.ylabel('Número de Puntos')
plt.title('Tamaño de Clusters Espaciales')
plt.grid(True, alpha=0.3)

# Subplot 2: Distribución de puntos por cluster
plt.subplot(2, 2, 2)
plt.hist(cluster_sizes.values, bins=20, alpha=0.7, color='orange')
plt.xlabel('Número de Puntos por Cluster')
plt.ylabel('Frecuencia')
plt.title('Distribución de Tamaños de Clusters')
plt.grid(True, alpha=0.3)

# Subplot 3: Centroides de clusters
plt.subplot(2, 2, 3)
if len(centroids_espacial) > 0:
    plt.scatter(centroids_espacial[:, 1], centroids_espacial[:, 0], 
               c=range(len(centroids_espacial)), cmap='tab20', s=50, alpha=0.8)
    plt.xlabel('Longitud')
    plt.ylabel('Latitud')
    plt.title('Centroides de Clusters')
    plt.grid(True, alpha=0.3)

# Subplot 4: Resumen estadístico
plt.subplot(2, 2, 4)
plt.axis('off')
summary_text = f"""
RESUMEN CLUSTERING GEOGRÁFICO:
Dataset Total: {len(df):,} puntos
Dataset Procesado: {len(df_clustered):,} puntos
Outliers: {len(df_clustered[df_clustered['cluster_espacial'] == -1]):,} puntos

CLUSTERS:
Espaciales: {n_clusters_espacial}
Promedio puntos/cluster: {cluster_espacial_counts.mean():.0f}
Mínimo puntos/cluster: {cluster_espacial_counts.min()}
Máximo puntos/cluster: {cluster_espacial_counts.max()}

CALIDAD:
Inercia Total: {inertia_total:,.0f}
"""
plt.text(0.1, 0.5, summary_text, fontsize=10, verticalalignment='center',
         bbox=dict(boxstyle="round,pad=0.3", facecolor="lightblue", alpha=0.8))

plt.tight_layout()
plt.savefig('3Preprocesamiento/img/analisis_geografico.png', dpi=300, bbox_inches='tight')
plt.close()
print("Análisis detallado guardado: analisis_geografico.png")

print(f"\n=== RESUMEN FINAL ===")
print(f"Dataset original: {len(df):,} puntos")
print(f"Dataset procesado: {len(df_clustered):,} puntos")
print(f"Outliers: {len(df_clustered[df_clustered['cluster_espacial'] == -1]):,} puntos")
print(f"Clusters espaciales: {n_clusters_espacial}")
print(f"Inercia total: {inertia_total:,.0f}")

print(f"\nArchivos generados:")
print(f"  - clusters_geograficos.png (visualización de clusters)")
print(f"  - analisis_geografico.png (análisis detallado)")

print(f"\n¡Clusterización geográfica completada!")

# Visualización de los clusters
plt.figure(figsize=(10, 8))
plt.scatter(df_clustered['pickup_longitude'], df_clustered['pickup_latitude'], c=df_clustered['cluster_espacial'], cmap='tab20', s=1, alpha=0.5)
plt.scatter(centroids_espacial[:, 1], centroids_espacial[:, 0], c='red', s=50, marker='x', label='Centroides')
plt.xlabel('Longitud')
plt.ylabel('Latitud')
plt.title('Clusters geográficos del dataframe')
plt.legend()
plt.tight_layout()
plt.savefig('3Preprocesamiento/img/clusters_geograficos.png', dpi=200)
plt.close()
print("Gráfico de clusters guardado en: 3Preprocesamiento/img/clusters_geograficos.png")

# Guardar dataset extendido con centroides
# Crear DataFrame de centroides
if len(centroids_espacial) > 0:
    df_centroides = pd.DataFrame(centroids_espacial, columns=['centroide_lat', 'centroide_long'])
    df_centroides['cluster_espacial'] = df_centroides.index
    # Unir centroides a cada punto según su cluster
    df_clustered = df_clustered.merge(df_centroides, on='cluster_espacial', how='left')
    # Guardar el dataset extendido
    df_clustered.to_csv('2Database/3clusterizacion_geografica_with_centroids.csv', index=False)
    print("Archivo guardado: 2Database/3clusterizacion_geografica_with_centroids.csv") 