import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.cluster import MiniBatchKMeans, KMeans
from sklearn.neighbors import NearestNeighbors
from sklearn.decomposition import PCA
import seaborn as sns
import gc
import warnings
warnings.filterwarnings('ignore')

# Configurar pandas para mantener alta precisión
pd.set_option('display.float_format', '{:.14f}'.format)
pd.set_option('display.precision', 13)

print("=== CLUSTERIZACIÓN MEJORADA CON MANEJO DE OUTLIERS ===")

# Cargar datos PCA
print("\nCargando datos PCA...")
try:
    df = pd.read_csv('2Database/3pca_data_set_complete.csv')
    print(f"Dataset PCA shape: {df.shape}")
    print(f"Columnas: {list(df.columns)}")
except FileNotFoundError:
    print("Archivo PCA no encontrado, intentando con datos procesados...")
    df = pd.read_csv('2Database/2processed_data_complete_limpio.csv')
    print(f"Dataset procesado shape: {df.shape}")
    print(f"Columnas: {list(df.columns)}")

# ANÁLISIS DE OUTLIERS MEJORADO
print(f"\n=== ANÁLISIS DE OUTLIERS MEJORADO ===")

# Identificar puntos con coordenadas (0.0, 0.0)
outlier_mask = (df['pickup_latitude'] == 0.0) & (df['pickup_longitude'] == 0.0)
n_outliers = outlier_mask.sum()

print(f"Puntos con coordenadas (0.0, 0.0): {n_outliers:,} ({n_outliers/len(df)*100:.2f}%)")

# Identificar otros outliers potenciales
lat_outliers = (df['pickup_latitude'] == 0.0).sum()
lon_outliers = (df['pickup_longitude'] == 0.0).sum()

print(f"Puntos con latitud 0.0: {lat_outliers:,}")
print(f"Puntos con longitud 0.0: {lon_outliers:,}")

# Separar outliers del dataset principal
df_outliers = df[outlier_mask].copy()
df_limpio = df[~outlier_mask].copy()

print(f"\nDataset limpio: {len(df_limpio):,} puntos ({len(df_limpio)/len(df)*100:.2f}%)")
print(f"Dataset outliers: {len(df_outliers):,} puntos ({len(df_outliers)/len(df)*100:.2f}%)")

# Análisis detallado de outliers
if len(df_outliers) > 0:
    print(f"\n=== ANÁLISIS DETALLADO DE OUTLIERS ===")
    
    # Verificamos si los outliers tienen atributos válidos
    exclude_cols = ['pickup_latitude', 'pickup_longitude']
    outlier_attr_cols = [col for col in df_outliers.columns if col not in exclude_cols]
    
    print(f"Atributos disponibles en outliers: {len(outlier_attr_cols)}")
    
    # Análisis por tipo de atributo
    numeric_cols = df_outliers[outlier_attr_cols].select_dtypes(include=[np.number]).columns.tolist()
    categorical_cols = df_outliers[outlier_attr_cols].select_dtypes(include=['object']).columns.tolist()
    
    print(f"Atributos numéricos: {len(numeric_cols)}")
    print(f"Atributos categóricos: {len(categorical_cols)}")
    
    # Mostrar estadísticas de atributos numéricos
    if len(numeric_cols) > 0:
        print(f"\nEstadísticas de atributos numéricos en outliers:")
        outlier_stats = df_outliers[numeric_cols].describe()
        print(outlier_stats)
        
        # Verificar si hay valores no nulos
        non_null_counts = df_outliers[numeric_cols].count()
        print(f"\nConteo de valores no nulos por atributo:")
        for col, count in non_null_counts.items():
            print(f"  {col}: {count:,} valores no nulos")
    
    # Análisis de atributos categóricos
    if len(categorical_cols) > 0:
        print(f"\nAnálisis de atributos categóricos en outliers:")
        for col in categorical_cols[:5]:  # Mostrar solo primeros 5
            unique_vals = df_outliers[col].nunique()
            print(f"  {col}: {unique_vals} valores únicos")
            if unique_vals <= 10:
                value_counts = df_outliers[col].value_counts().head(5)
                print(f"    Top 5 valores: {dict(value_counts)}")

# CALCULAR NÚMERO APROPIADO DE CLUSTERS OPTIMIZADO
print(f"\n=== CÁLCULO DE CLUSTERS OPTIMIZADO ===")

# Para 10.9 M puntos válidos, usar reglas más apropiadas
n_points = len(df_limpio)
n_points_outliers = len(df_outliers)
n_total = len(df)

print(f"Dataset total: {n_total:,} puntos")
print(f"Dataset válido: {n_points:,} puntos")
print(f"Dataset outiliers: {n_points_outliers:,} puntos")

print(f"Porcentaje válido: {n_points/n_total*100:.2f}%")

# Cálculo optimizado de clusters
# Regla: 1 cluster espacial por cada 1,000-2,000 puntos
n_clusters_espacial = max(200, min(2000, n_points // 1500))  # Más clusters para mejor granularidad

# Regla: 1 sub-cluster por cada 10,000-20,000 puntos
n_sub_clusters = max(3, min(8, n_points // 15000))

print(f"Clusters espaciales calculados: {n_clusters_espacial}")
print(f"Sub-clusters por cluster: {n_sub_clusters}")
print(f"Total clusters jerárquicos estimados: {n_clusters_espacial * n_sub_clusters}")
print(f"Promedio puntos por cluster espacial: {n_points // n_clusters_espacial:,}")
print(f"Promedio puntos por sub-cluster: {n_points // (n_clusters_espacial * n_sub_clusters):,}")

# Verificar si los parámetros son razonables
if n_clusters_espacial > 1000:
    print(f"ADVERTENCIA: Muchos clusters espaciales ({n_clusters_espacial}). Considerando reducir...")
    n_clusters_espacial = min(1000, n_clusters_espacial)
    print(f"Ajustado a: {n_clusters_espacial} clusters espaciales")

if n_sub_clusters > 5:
    print(f"ADVERTENCIA: Muchos sub-clusters ({n_sub_clusters}). Considerando reducir...")
    n_sub_clusters = min(5, n_sub_clusters)
    print(f"Ajustado a: {n_sub_clusters} sub-clusters")

# FUNCIÓN FASE 1: CLUSTERIZACIÓN ESPACIAL MEJORADA
def clusterizacion_espacial_mejorada(coords, n_clusters=100):
    """
    Primera fase: Clustering espacial con parámetros optimizados para datasets grandes
    """
    print(f"\n=== FASE 1: CLUSTERIZACIÓN ESPACIAL MEJORADA ===")
    print(f"Clusters objetivo: {n_clusters}")
    print(f"Puntos a procesar: {len(coords):,}")
    
    # Verificar que no hay coordenadas (0.0, 0.0)
    zero_coords = ((coords[:, 0] == 0.0) & (coords[:, 1] == 0.0)).sum()
    if zero_coords > 0:
        print(f"ADVERTENCIA: {zero_coords} puntos con coordenadas (0.0, 0.0) encontrados")
    
    # Parámetros optimizados para datasets grandes
    batch_size = min(20000, max(5000, len(coords) // 20))  # Batch size adaptativo
    n_init = 3 if n_clusters > 500 else 5  # Menos inicializaciones para muchos clusters
    max_iter = 150 if n_clusters > 500 else 200  # Menos iteraciones para muchos clusters
    
    print(f"  Parámetros optimizados:")
    print(f"    Batch size: {batch_size:,}")
    print(f"    Inicializaciones: {n_init}")
    print(f"    Iteraciones máximas: {max_iter}")
    
    # Usar Mini-Batch K-Means con parámetros optimizados
    kmeans_espacial = MiniBatchKMeans(
        n_clusters=n_clusters,
        batch_size=batch_size,
        random_state=42,
        n_init=n_init,
        max_iter=max_iter,
        tol=1e-4
    )
    
    print(f"  Aplicando Mini-Batch K-Means...")
    
    # Aplicar clustering
    labels_espacial = kmeans_espacial.fit_predict(coords)
    centroids_espacial = kmeans_espacial.cluster_centers_
    
    # Calcular métricas de calidad
    inertia = kmeans_espacial.inertia_
    inertia_per_point = inertia / len(coords)
    
    print(f"  Resultados:")
    print(f"    Clusters creados: {n_clusters}")
    print(f"    Inercia total: {inertia:,.2f}")
    print(f"    Inercia promedio por punto: {inertia_per_point:.6f}")
    
    # Análisis de distribución de clusters
    unique_labels, counts = np.unique(labels_espacial, return_counts=True)
    print(f"    Clusters con puntos: {len(unique_labels)}")
    print(f"    Promedio puntos por cluster: {counts.mean():.0f}")
    print(f"    Mínimo puntos por cluster: {counts.min()}")
    print(f"    Máximo puntos por cluster: {counts.max()}")
    
    return labels_espacial, centroids_espacial, inertia

# FUNCIÓN FASE 2: SUB-CLUSTERIZACIÓN MEJORADA
def sub_clusterizacion_mejorada(df_cluster, cluster_id, n_sub_clusters=5):
    """
    Segunda fase: Sub-clustering con análisis de atributos mejorado para datasets grandes
    """
    # Identificar columnas de atributos
    exclude_cols = ['pickup_latitude', 'pickup_longitude', 'cluster_espacial', 'sub_cluster']
    attr_cols = [col for col in df_cluster.columns if col not in exclude_cols]
    
    if len(attr_cols) == 0:
        print(f"  Cluster {cluster_id}: No hay atributos para sub-clustering")
        return np.zeros(len(df_cluster), dtype=int)
    
    # Prioridad: PCA components > atributos numéricos > otros
    pca_cols = [col for col in attr_cols if col.startswith('PC')]
    numeric_cols = df_cluster[attr_cols].select_dtypes(include=[np.number]).columns.tolist()
    
    if len(pca_cols) > 0:
        features = df_cluster[pca_cols].values
        print(f"  Cluster {cluster_id}: Usando {len(pca_cols)} componentes PCA")
    elif len(numeric_cols) > 0:
        # Limitar a los primeros 10 atributos numéricos para eficiencia
        features = df_cluster[numeric_cols[:10]].values
        print(f"  Cluster {cluster_id}: Usando {len(numeric_cols[:10])} atributos numéricos")
    else:
        print(f"  Cluster {cluster_id}: No hay atributos numéricos válidos")
        return np.zeros(len(df_cluster), dtype=int)
    
    # Verificar que hay suficientes puntos para clustering
    min_points_per_cluster = 10  # Mínimo 10 puntos por sub-cluster
    max_sub_clusters = len(features) // min_points_per_cluster
    
    if max_sub_clusters < n_sub_clusters:
        n_sub_clusters = max(1, max_sub_clusters)
        print(f"  Cluster {cluster_id}: Ajustando a {n_sub_clusters} sub-clusters (insuficientes puntos)")
    
    if n_sub_clusters <= 1:
        print(f"  Cluster {cluster_id}: No se pueden crear sub-clusters (muy pocos puntos)")
        return np.zeros(len(df_cluster), dtype=int)
    
    # Normalizar features
    features_norm = (features - features.mean(axis=0)) / (features.std(axis=0) + 1e-8)
    
    # Parámetros optimizados para sub-clustering
    n_init_sub = 5 if n_sub_clusters <= 3 else 3  # Menos inicializaciones para pocos clusters
    max_iter_sub = 100 if len(features) > 10000 else 200  # Menos iteraciones para muchos puntos
    
    # Aplicar K-Means con parámetros optimizados
    kmeans_sub = KMeans(
        n_clusters=n_sub_clusters,
        random_state=42,
        n_init=n_init_sub,
        max_iter=max_iter_sub,
        tol=1e-4
    )
    
    sub_labels = kmeans_sub.fit_predict(features_norm)
    
    # Calcular métricas de calidad del sub-clustering
    sub_inertia = kmeans_sub.inertia_
    sub_inertia_per_point = sub_inertia / len(features)
    
    print(f"  Cluster {cluster_id}: {n_sub_clusters} sub-clusters creados")
    print(f"    Inercia sub-clustering: {sub_inertia:.2f}")
    print(f"    Inercia promedio por punto: {sub_inertia_per_point:.6f}")
    
    return sub_labels

# FUNCIÓN PARA ANALIZAR Y DECIDIR SOBRE OUTLIERS
def analizar_y_decidir_outliers(df_outliers):
    """
    Analiza los outliers y decide si se pueden recuperar o deben eliminarse
    """
    print(f"\n=== ANÁLISIS Y DECISIÓN SOBRE OUTLIERS ===")
    
    if len(df_outliers) == 0:
        print("No hay outliers para analizar")
        return None, None
    
    # Verificar si outliers tienen información útil
    exclude_cols = ['pickup_latitude', 'pickup_longitude']
    attr_cols = [col for col in df_outliers.columns if col not in exclude_cols]
    
    if len(attr_cols) == 0:
        print("Outliers no tienen atributos adicionales - marcados para eliminación")
        return None, df_outliers
    
    # Analizar atributos numéricos
    numeric_cols = df_outliers[attr_cols].select_dtypes(include=[np.number]).columns.tolist()
    
    if len(numeric_cols) == 0:
        print("Outliers no tienen atributos numéricos - marcados para eliminación")
        return None, df_outliers
    
    # Verificar si hay valores no nulos significativos
    non_null_counts = df_outliers[numeric_cols].count()
    total_possible = len(df_outliers) * len(numeric_cols)
    non_null_percentage = non_null_counts.sum() / total_possible * 100
    
    print(f"Porcentaje de valores no nulos en outliers: {non_null_percentage:.1f}%")
    
    if non_null_percentage < 50:
        print("Outliers tienen muchos valores nulos - marcados para eliminación")
        return None, df_outliers
    
    # Verificar si hay variabilidad en los atributos
    numeric_stats = df_outliers[numeric_cols].describe()
    has_variability = False
    
    for col in numeric_cols:
        std_val = numeric_stats.loc['std', col]
        if std_val > 0:
            has_variability = True
            break
    
    if not has_variability:
        print("Outliers no tienen variabilidad en atributos - marcados para eliminación")
        return None, df_outliers
    
    # Si llegamos aquí, los outliers tienen información útil
    print("Outliers tienen información útil - se pueden usar para clustering por atributos")
    
    # Intentar recuperar coordenadas aproximadas basadas en otros atributos
    # (esto es una aproximación simple)
    df_recoverable = df_outliers.copy()
    
    # Asignar coordenadas aproximadas basadas en patrones en los datos
    # Por ejemplo, si hay atributos que sugieren ubicación
    location_indicators = []
    
    for col in numeric_cols:
        if any(keyword in col.lower() for keyword in ['zone', 'borough', 'area', 'region']):
            location_indicators.append(col)
    
    if len(location_indicators) > 0:
        print(f"Indicadores de ubicación encontrados: {location_indicators}")
        # Aquí podrías implementar lógica para asignar coordenadas aproximadas
        # Por ahora, los mantenemos como outliers especiales
    
    return df_recoverable, None

# FUNCIÓN PARA CLUSTERIZAR OUTLIERS POR ATRIBUTOS
def clusterizar_outliers_por_atributos(df_outliers, n_clusters_outliers=10):
    """
    Clusterizar outliers basándose solo en sus atributos (sin coordenadas)
    """
    print(f"\n=== CLUSTERIZACIÓN DE OUTLIERS POR ATRIBUTOS ===")
    
    if len(df_outliers) == 0:
        print("No hay outliers para clusterizar")
        return df_outliers
    
    print(f"Outliers a clusterizar: {len(df_outliers):,} puntos")
    
    # Identificar atributos disponibles (excluir coordenadas)
    exclude_cols = ['pickup_latitude', 'pickup_longitude', 'cluster_espacial', 'sub_cluster', 'cluster_jerarquico']
    attr_cols = [col for col in df_outliers.columns if col not in exclude_cols]
    
    if len(attr_cols) == 0:
        print("No hay atributos disponibles para clustering de outliers")
        return df_outliers
    
    # Seleccionar atributos numéricos
    numeric_cols = df_outliers[attr_cols].select_dtypes(include=[np.number]).columns.tolist()
    
    if len(numeric_cols) == 0:
        print("No hay atributos numéricos para clustering de outliers")
        return df_outliers
    
    print(f"Atributos numéricos disponibles: {len(numeric_cols)}")
    print(f"Atributos: {numeric_cols[:5]}...")  # Mostrar primeros 5
    
    # Limitar a los primeros 15 atributos para eficiencia
    features_cols = numeric_cols[:15]
    features = df_outliers[features_cols].values
    
    print(f"Usando {len(features_cols)} atributos para clustering")
    
    # Verificar que hay suficientes puntos para clustering
    if len(features) < n_clusters_outliers * 2:
        n_clusters_outliers = max(1, len(features) // 2)
        print(f"Ajustando número de clusters a {n_clusters_outliers}")
    
    if n_clusters_outliers <= 1:
        print("No se pueden crear clusters (muy pocos puntos)")
        df_outliers['cluster_espacial'] = -1
        df_outliers['sub_cluster'] = -1
        df_outliers['cluster_jerarquico'] = 'outlier_single'
        return df_outliers
    
    # Normalizar features
    features_norm = (features - features.mean(axis=0)) / (features.std(axis=0) + 1e-8)
    
    # Aplicar K-Means para clustering de outliers
    print(f"Aplicando K-Means para {n_clusters_outliers} clusters de outliers...")
    
    kmeans_outliers = KMeans(
        n_clusters=n_clusters_outliers,
        random_state=42,
        n_init=5,
        max_iter=200,
        tol=1e-4
    )
    
    outlier_labels = kmeans_outliers.fit_predict(features_norm)
    
    # Calcular métricas de calidad
    outlier_inertia = kmeans_outliers.inertia_
    outlier_inertia_per_point = outlier_inertia / len(features)
    
    print(f"Clustering de outliers completado:")
    print(f"  Clusters creados: {n_clusters_outliers}")
    print(f"  Inercia total: {outlier_inertia:.2f}")
    print(f"  Inercia promedio por punto: {outlier_inertia_per_point:.6f}")
    
    # Asignar clusters a outliers
    df_outliers_clustered = df_outliers.copy()
    df_outliers_clustered['cluster_espacial'] = -1  # Cluster especial para outliers
    df_outliers_clustered['sub_cluster'] = outlier_labels
    df_outliers_clustered['cluster_jerarquico'] = 'outlier_' + outlier_labels.astype(str)
    
    # Análisis de distribución de clusters de outliers
    outlier_cluster_counts = df_outliers_clustered['sub_cluster'].value_counts().sort_index()
    print(f"\nDistribución de clusters de outliers:")
    for cluster_id, count in outlier_cluster_counts.items():
        print(f"  Cluster outlier {cluster_id}: {count:,} puntos")
    
    return df_outliers_clustered

# FUNCIÓN PRINCIPAL: CLUSTERIZACIÓN COMPLETA CON OUTLIERS
def clusterizacion_completa_mejorada(df, n_clusters_espacial=100, n_sub_clusters=5, n_clusters_outliers=10):
    """
    Clusterización completa con manejo inteligente de outliers
    """
    print(f"\n=== CLUSTERIZACIÓN COMPLETA MEJORADA ===")
    
    # Separar outliers
    outlier_mask = (df['pickup_latitude'] == 0.0) & (df['pickup_longitude'] == 0.0)
    df_outliers = df[outlier_mask].copy()
    df_limpio = df[~outlier_mask].copy()
    
    print(f"Dataset limpio: {len(df_limpio):,} puntos")
    print(f"Outliers: {len(df_outliers):,} puntos")
    
    # FASE 1: Clustering espacial en datos limpios
    coords_limpio = df_limpio[['pickup_latitude', 'pickup_longitude']].values
    labels_espacial, centroids_espacial, inertia = clusterizacion_espacial_mejorada(
        coords_limpio, n_clusters_espacial
    )
    
    # Agregar labels espaciales
    df_result = df_limpio.copy()
    df_result['cluster_espacial'] = labels_espacial
    
    # FASE 2: Sub-clustering por atributos en datos limpios
    print(f"\n=== FASE 2: SUB-CLUSTERIZACIÓN POR ATRIBUTOS (DATOS LIMPIOS) ===")
    df_result['sub_cluster'] = -1
    
    # Procesar cada cluster espacial
    for cluster_id in range(n_clusters_espacial):
        mask = labels_espacial == cluster_id
        df_cluster = df_result[mask].copy()
        
        if len(df_cluster) == 0:
            continue
            
        print(f"\nProcesando cluster espacial {cluster_id}: {len(df_cluster):,} puntos")
        
        sub_labels = sub_clusterizacion_mejorada(df_cluster, cluster_id, n_sub_clusters)
        df_result.loc[mask, 'sub_cluster'] = cluster_id * n_sub_clusters + sub_labels
    
    # Crear identificador jerárquico para datos limpios
    df_result['cluster_jerarquico'] = df_result['cluster_espacial'].astype(str) + '_' + df_result['sub_cluster'].astype(str)
    
    # FASE 3: Clustering de outliers por atributos
    print(f"\n=== FASE 3: CLUSTERING DE OUTLIERS POR ATRIBUTOS ===")
    
    # Calcular número apropiado de clusters para outliers
    if len(df_outliers) > 0:
        # Regla: 1 cluster por cada 1,000-2,000 outliers
        n_clusters_outliers_ajustado = max(5, min(50, len(df_outliers) // 2000))
        print(f"Clusters para outliers calculados: {n_clusters_outliers_ajustado}")
        
        df_outliers_clustered = clusterizar_outliers_por_atributos(
            df_outliers, n_clusters_outliers_ajustado
        )
        
        # Concatenar outliers clusterizados al resultado
        df_result = pd.concat([df_result, df_outliers_clustered], ignore_index=True)
        print(f"Outliers clusterizados agregados: {len(df_outliers_clustered):,} puntos")
    else:
        print("No hay outliers para clusterizar")
    
    return df_result, centroids_espacial, inertia

# APLICAR CLUSTERIZACIÓN MEJORADA
print(f"\n=== APLICANDO CLUSTERIZACIÓN MEJORADA ===")

# Calcular número de clusters para outliers
n_clusters_outliers = max(5, min(50, len(df_outliers) // 2000)) if len(df_outliers) > 0 else 10

print(f"Parámetros de clustering:")
print(f"  Clusters espaciales: {n_clusters_espacial}")
print(f"  Sub-clusters por cluster: {n_sub_clusters}")
print(f"  Clusters para outliers: {n_clusters_outliers}")

# Aplicar clusterización
df_clustered, centroids_espacial, inertia_total = clusterizacion_completa_mejorada(
    df, n_clusters_espacial, n_sub_clusters, n_clusters_outliers
)

# ANÁLISIS DE RESULTADOS
print(f"\n=== ANÁLISIS DE RESULTADOS ===")

# Estadísticas generales
n_total = len(df_clustered)
n_outliers_final = (df_clustered['cluster_espacial'] == -1).sum()
n_clustered = n_total - n_outliers_final

print(f"Total puntos procesados: {n_total:,}")
print(f"Puntos en clusters espaciales: {n_clustered:,}")
print(f"Outliers clusterizados: {n_outliers_final:,}")

# Estadísticas de clusters espaciales (excluyendo outliers)
df_clusters_only = df_clustered[df_clustered['cluster_espacial'] != -1]
cluster_espacial_counts = df_clusters_only['cluster_espacial'].value_counts()

print(f"\nEstadísticas de clusters espaciales:")
print(f"Clusters espaciales únicos: {len(cluster_espacial_counts)}")
print(f"Promedio puntos por cluster: {cluster_espacial_counts.mean():.0f}")
print(f"Mínimo puntos por cluster: {cluster_espacial_counts.min()}")
print(f"Máximo puntos por cluster: {cluster_espacial_counts.max()}")

# Estadísticas de sub-clusters (datos limpios)
sub_cluster_counts = df_clusters_only['sub_cluster'].value_counts()

print(f"\nEstadísticas de sub-clusters (datos limpios):")
print(f"Sub-clusters únicos: {len(sub_cluster_counts)}")
print(f"Promedio puntos por sub-cluster: {sub_cluster_counts.mean():.0f}")
print(f"Mínimo puntos por sub-cluster: {sub_cluster_counts.min()}")
print(f"Máximo puntos por sub-cluster: {sub_cluster_counts.max()}")

# Estadísticas de clusters de outliers
df_outliers_only = df_clustered[df_clustered['cluster_espacial'] == -1]
if len(df_outliers_only) > 0:
    outlier_cluster_counts = df_outliers_only['sub_cluster'].value_counts()
    
    print(f"\nEstadísticas de clusters de outliers:")
    print(f"Clusters de outliers únicos: {len(outlier_cluster_counts)}")
    print(f"Promedio puntos por cluster outlier: {outlier_cluster_counts.mean():.0f}")
    print(f"Mínimo puntos por cluster outlier: {outlier_cluster_counts.min()}")
    print(f"Máximo puntos por cluster outlier: {outlier_cluster_counts.max()}")
    
    print(f"\nDistribución de clusters de outliers:")
    for cluster_id, count in outlier_cluster_counts.sort_index().items():
        print(f"  Cluster outlier {cluster_id}: {count:,} puntos")

# Distribución de clusters jerárquicos
hierarchical_dist = df_clustered['cluster_jerarquico'].value_counts()
print(f"\nDistribución de clusters jerárquicos:")
print(f"Clusters jerárquicos únicos: {len(hierarchical_dist)}")

# Contar tipos de clusters
outlier_clusters = hierarchical_dist[hierarchical_dist.index.str.startswith('outlier_')]
normal_clusters = hierarchical_dist[~hierarchical_dist.index.str.startswith('outlier_')]

print(f"Clusters normales (espaciales + atributos): {len(normal_clusters)}")
print(f"Clusters de outliers (solo atributos): {len(outlier_clusters)}")

# VISUALIZACIÓN
print(f"\n=== GENERANDO VISUALIZACIONES ===")

# Usar muestra para visualización
if len(df_clustered) > 100000:
    df_viz = df_clustered.sample(n=100000, random_state=42)
else:
    df_viz = df_clustered

# Gráfica 1: Clusters espaciales
plt.figure(figsize=(20, 8))

plt.subplot(1, 2, 1)
# Filtrar datos limpios para visualización
df_viz_limpio = df_viz[df_viz['cluster_espacial'] != -1]
if len(df_viz_limpio) > 0:
    scatter1 = plt.scatter(df_viz_limpio['pickup_longitude'], df_viz_limpio['pickup_latitude'], 
                          c=df_viz_limpio['cluster_espacial'], cmap='tab20', alpha=0.6, s=0.5)
    plt.colorbar(scatter1, label='Cluster Espacial')

# Mostrar outliers en rojo (sin coordenadas válidas, se muestran en 0,0)
df_viz_outliers = df_viz[df_viz['cluster_espacial'] == -1]
if len(df_viz_outliers) > 0:
    plt.scatter(0, 0, c='red', alpha=0.8, s=100, marker='x', 
               label=f'Outliers ({len(df_viz_outliers):,}) - Clusterizados por atributos')

plt.title(f'Fase 1: Clusters Espaciales ({n_clusters_espacial} clusters)\n+ Outliers clusterizados por atributos', fontsize=14)
plt.xlabel('Longitud')
plt.ylabel('Latitud')
plt.grid(True, alpha=0.3)
plt.legend()

# Gráfica 2: Clusters jerárquicos
plt.subplot(1, 2, 2)
if len(df_viz_limpio) > 0:
    scatter2 = plt.scatter(df_viz_limpio['pickup_longitude'], df_viz_limpio['pickup_latitude'], 
                          c=df_viz_limpio['cluster_espacial'], cmap='tab20', alpha=0.6, s=0.5)
    plt.colorbar(scatter2, label='Cluster Espacial')

if len(df_viz_outliers) > 0:
    plt.scatter(0, 0, c='red', alpha=0.8, s=100, marker='x', 
               label=f'Outliers ({len(df_viz_outliers):,}) - {len(outlier_clusters)} clusters')

plt.title(f'Fase 2: Clusters Jerárquicos\n({len(hierarchical_dist)} clusters totales)', fontsize=14)
plt.xlabel('Longitud')
plt.ylabel('Latitud')
plt.grid(True, alpha=0.3)
plt.legend()

plt.tight_layout()
plt.savefig('3Preprocesamiento/AnalisisCoord/clusters_mejorados.png', dpi=300, bbox_inches='tight')
plt.close()
print("Visualización guardada: clusters_mejorados.png")

# Gráfica 3: Análisis detallado
plt.figure(figsize=(20, 12))

# Subplot 1: Tamaño de clusters espaciales
plt.subplot(2, 3, 1)
cluster_sizes = df_clusters_only['cluster_espacial'].value_counts().sort_index()
plt.bar(range(len(cluster_sizes)), cluster_sizes.values, alpha=0.7)
plt.xlabel('Cluster Espacial ID')
plt.ylabel('Número de Puntos')
plt.title('Tamaño de Clusters Espaciales')
plt.grid(True, alpha=0.3)

# Subplot 2: Distribución de sub-clusters (datos limpios)
plt.subplot(2, 3, 2)
sub_cluster_sizes = df_clusters_only['sub_cluster'].value_counts().sort_index()
plt.bar(range(len(sub_cluster_sizes)), sub_cluster_sizes.values, alpha=0.7, color='orange')
plt.xlabel('Sub-cluster ID')
plt.ylabel('Número de Puntos')
plt.title('Tamaño de Sub-clusters (Datos Limpios)')
plt.grid(True, alpha=0.3)

# Subplot 3: Distribución de clusters de outliers
plt.subplot(2, 3, 3)
if len(df_outliers_only) > 0:
    outlier_cluster_sizes = df_outliers_only['sub_cluster'].value_counts().sort_index()
    plt.bar(range(len(outlier_cluster_sizes)), outlier_cluster_sizes.values, alpha=0.7, color='red')
    plt.xlabel('Cluster Outlier ID')
    plt.ylabel('Número de Puntos')
    plt.title('Tamaño de Clusters de Outliers')
    plt.grid(True, alpha=0.3)
else:
    plt.text(0.5, 0.5, 'No hay outliers', ha='center', va='center', transform=plt.gca().transAxes)
    plt.title('Clusters de Outliers')

# Subplot 4: Sub-clusters por cluster espacial
plt.subplot(2, 3, 4)
sub_per_espacial = df_clusters_only.groupby('cluster_espacial')['sub_cluster'].nunique()
plt.bar(range(len(sub_per_espacial)), sub_per_espacial.values, alpha=0.7, color='green')
plt.xlabel('Cluster Espacial ID')
plt.ylabel('Número de Sub-clusters')
plt.title('Sub-clusters por Cluster Espacial')
plt.grid(True, alpha=0.3)

# Subplot 5: Análisis de atributos de outliers
plt.subplot(2, 3, 5)
if len(df_outliers) > 0:
    outlier_cols = [col for col in df_outliers.columns if col not in ['pickup_latitude', 'pickup_longitude', 'cluster_espacial', 'sub_cluster', 'cluster_jerarquico']]
    if len(outlier_cols) > 0:
        numeric_outlier_cols = df_outliers[outlier_cols].select_dtypes(include=[np.number]).columns.tolist()
        if len(numeric_outlier_cols) > 0:
            # Mostrar distribución de un atributo numérico en outliers
            plt.hist(df_outliers[numeric_outlier_cols[0]], bins=50, alpha=0.7, color='red')
            plt.xlabel(numeric_outlier_cols[0])
            plt.ylabel('Frecuencia')
            plt.title(f'Distribución de {numeric_outlier_cols[0]} en Outliers')
            plt.grid(True, alpha=0.3)

# Subplot 6: Resumen estadístico
plt.subplot(2, 3, 6)
plt.axis('off')
summary_text = f"""
RESUMEN MEJORADO:
Dataset Total: {len(df):,} puntos
Dataset Limpio: {len(df_limpio):,} puntos
Outliers: {len(df_outliers):,} puntos

CLUSTERS:
Espaciales: {n_clusters_espacial}
Sub-clusters por cluster: {n_sub_clusters}
Clusters de outliers: {len(outlier_clusters) if len(df_outliers_only) > 0 else 0}
Total Jerárquicos: {len(hierarchical_dist)}

CALIDAD:
Inercia Total: {inertia_total:,.0f}
Promedio puntos/cluster: {cluster_espacial_counts.mean():.0f}
"""
plt.text(0.1, 0.5, summary_text, fontsize=10, verticalalignment='center',
         bbox=dict(boxstyle="round,pad=0.3", facecolor="lightblue", alpha=0.8))

plt.tight_layout()
plt.savefig('3Preprocesamiento/AnalisisCoord/analisis_mejorado.png', dpi=300, bbox_inches='tight')
plt.close()
print("Análisis detallado guardado: analisis_mejorado.png")

# GUARDAR RESULTADOS
print(f"\n=== GUARDANDO RESULTADOS ===")

# Dataset completo
output_file = '2Database/6clusters_mejorados_completo.csv'
df_clustered.to_csv(output_file, index=False)
print(f"Dataset mejorado guardado: {output_file}")

# Resumen de clusters espaciales
cluster_espacial_summary = df_clusters_only.groupby('cluster_espacial').agg({
    'pickup_latitude': ['count', 'min', 'max', 'mean'],
    'pickup_longitude': ['min', 'max', 'mean']
}).round(6)

# Aplanar las columnas multi-nivel
cluster_espacial_summary.columns = ['_'.join(col).strip() for col in cluster_espacial_summary.columns]

cluster_espacial_file = '2Database/6clusters_espaciales_mejorados.csv'
cluster_espacial_summary.to_csv(cluster_espacial_file)
print(f"Resumen clusters espaciales: {cluster_espacial_file}")

# Análisis de outliers
if len(df_outliers) > 0:
    outliers_file = '2Database/6outliers_analisis.csv'
    df_outliers.to_csv(outliers_file, index=False)
    print(f"Análisis de outliers: {outliers_file}")

# Centroides espaciales
centroids_df = pd.DataFrame(centroids_espacial, columns=['centroid_lat', 'centroid_lon'])
centroids_df['cluster_espacial_id'] = range(len(centroids_espacial))
centroids_file = '2Database/6centroides_mejorados.csv'
centroids_df.to_csv(centroids_file, index=False)
print(f"Centroides mejorados: {centroids_file}")

print(f"\n=== RESUMEN FINAL ===")
print(f"Dataset original: {len(df):,} puntos")
print(f"Dataset limpio: {len(df_limpio):,} puntos")
print(f"Outliers: {len(df_outliers):,} puntos")
print(f"Clusters espaciales: {n_clusters_espacial}")
print(f"Sub-clusters por cluster: {n_sub_clusters}")
print(f"Clusters de outliers: {len(outlier_clusters) if len(df_outliers_only) > 0 else 0}")
print(f"Total clusters jerárquicos: {len(hierarchical_dist)}")
print(f"Inercia total: {inertia_total:,.0f}")

print(f"\nTipos de clusters generados:")
print(f"  - Clusters normales (espaciales + atributos): {len(normal_clusters)}")
print(f"  - Clusters de outliers (solo atributos): {len(outlier_clusters)}")

print(f"\nArchivos generados:")
print(f"  - clusters_mejorados.png (visualización)")
print(f"  - analisis_mejorado.png (análisis detallado)")
print(f"  - 6clusters_mejorados_completo.csv (datos completos)")
print(f"  - 6clusters_espaciales_mejorados.csv (resumen espacial)")
if len(df_outliers) > 0:
    print(f"  - 6outliers_analisis.csv (análisis de outliers)")
print(f"  - 6centroides_mejorados.csv (centroides)")

print(f"\n¡Clusterización mejorada completada!") 