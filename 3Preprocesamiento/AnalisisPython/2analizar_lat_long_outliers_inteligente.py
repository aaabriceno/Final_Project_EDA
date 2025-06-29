import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.cluster import DBSCAN
from sklearn.preprocessing import StandardScaler
import seaborn as sns

print("=== ANÁLISIS INTELIGENTE DE OUTLIERS PARA NYC ===")

# Cargar el dataset
print("Cargando dataset...")
df = pd.read_csv('2Database/1processed_data_complete.csv')
print(f"Dataset original: {df.shape}")

# Definir límites amplios para toda la región metropolitana de NYC
NYC = {
    'min_lat': 39.0,    # Límite sur amplio (incluye NJ, Long Island)
    'max_lat': 42.0,    # Límite norte amplio (incluye Connecticut, Upstate NY)
    'min_long': -75.0,  # Límite oeste amplio (incluye Pennsylvania)
    'max_long': -72.0   # Límite este amplio (incluye Long Island, Connecticut)
}

print(f"\nRangos definidos:")
print(f"NYC : Lat {NYC['min_lat']}°-{NYC['max_lat']}°, Long {NYC['min_long']}°-{NYC['max_long']}°")

# ANÁLISIS DE OUTLIERS
print(f"\n=== ANÁLISIS DE OUTLIERS ===")

# 1. Detectar errores claros (0.0)
errores_cero = ((df['pickup_latitude'] == 0) & (df['pickup_longitude'] == 0)).sum()
print(f"Errores claros (0,0): {errores_cero:,}")

# 2. Detectar puntos fuera de NYC 
fuera_metro = ((df['pickup_latitude'] < NYC['min_lat']) | 
               (df['pickup_latitude'] > NYC['max_lat']) |
               (df['pickup_longitude'] < NYC['min_long']) | 
               (df['pickup_longitude'] > NYC['max_long'])).sum()

print(f"Puntos fuera de NYC : {fuera_metro:,}")


# 4. Análisis de coordenadas inválidas
invalido_lat = ((df['pickup_latitude'] < -90) | (df['pickup_latitude'] > 90)).sum()
invalido_long = ((df['pickup_longitude'] < -180) | (df['pickup_longitude'] > 180)).sum()
print(f"Coordenadas lat inválidas: {invalido_lat:,}")
print(f"Coordenadas long inválidas: {invalido_long:,}")

# ANÁLISIS DE CLUSTERS DE OUTLIERS
print(f"\n=== ANÁLISIS DE CLUSTERS DE OUTLIERS ===")

# Filtrar datos válidos (sin errores claros)
df_valido = df[~((df['pickup_latitude'] == 0) & (df['pickup_longitude'] == 0))]
df_valido = df_valido[~(((df_valido['pickup_latitude'] < -90) | (df_valido['pickup_latitude'] > 90)) |
                        ((df_valido['pickup_longitude'] < -180) | (df_valido['pickup_longitude'] > 180)))]

print(f"Datos válidos para análisis: {len(df_valido):,}")

# Identificar outliers (fuera de NYC )
outliers_mask = ((df_valido['pickup_latitude'] < NYC['min_lat']) | 
                 (df_valido['pickup_latitude'] > NYC['max_lat']) |
                 (df_valido['pickup_longitude'] < NYC['min_long']) | 
                 (df_valido['pickup_longitude'] > NYC['max_long']))

df_outliers = df_valido[outliers_mask]
print(f"Outliers identificados: {len(df_outliers):,}")

if len(df_outliers) > 0:
    print(f"\nEstadísticas de outliers:")
    print(f"Latitud: {df_outliers['pickup_latitude'].min():.6f} a {df_outliers['pickup_latitude'].max():.6f}")
    print(f"Longitud: {df_outliers['pickup_longitude'].min():.6f} a {df_outliers['pickup_longitude'].max():.6f}")
    
    # Clustering de outliers para identificar patrones
    if len(df_outliers) > 10:  # Necesitamos suficientes puntos para clustering
        print(f"\nAnalizando clusters de outliers...")
        
        # Preparar datos para clustering
        coords_outliers = df_outliers[['pickup_latitude', 'pickup_longitude']].values
        
        # Normalizar coordenadas
        scaler = StandardScaler()
        coords_scaled = scaler.fit_transform(coords_outliers)
        
        # Aplicar DBSCAN para encontrar clusters
        dbscan = DBSCAN(eps=0.5, min_samples=5)
        clusters = dbscan.fit_predict(coords_scaled)
        
        n_clusters = len(set(clusters)) - (1 if -1 in clusters else 0)
        n_noise = list(clusters).count(-1)
        
        print(f"Clusters de outliers encontrados: {n_clusters}")
        print(f"Outliers aislados (noise): {n_noise}")
        
        # Analizar cada cluster
        for cluster_id in range(n_clusters):
            cluster_mask = clusters == cluster_id
            cluster_data = df_outliers[cluster_mask]
            print(f"\nCluster {cluster_id + 1}:")
            print(f"  Puntos: {len(cluster_data):,}")
            print(f"  Latitud: {cluster_data['pickup_latitude'].mean():.6f} ± {cluster_data['pickup_latitude'].std():.6f}")
            print(f"  Longitud: {cluster_data['pickup_longitude'].mean():.6f} ± {cluster_data['pickup_longitude'].std():.6f}")
            
            # Calcular distancia al centro de NYC
            nyc_center_lat = (NYC['min_lat'] + NYC['max_lat']) / 2
            nyc_center_long = (NYC['min_long'] + NYC['max_long']) / 2
            
            distancias = np.sqrt((cluster_data['pickup_latitude'] - nyc_center_lat)**2 + 
                                (cluster_data['pickup_longitude'] - nyc_center_long)**2)
            print(f"  Distancia promedio a NYC: {distancias.mean():.4f}°")
            
            # Convertir a km aproximados (1° ≈ 111 km)
            dist_km = distancias.mean() * 111
            print(f"  Distancia promedio a NYC: {dist_km:.1f} km")

# ESTRATEGIAS DE FILTRADO
print(f"\n=== FILTRADO NYC ===")

# Solo NYC Metro (incluye toda la región metropolitana)
df_metro = df[((df['pickup_latitude'] >= NYC['min_lat']) & 
               (df['pickup_latitude'] <= NYC['max_lat']) &
               (df['pickup_longitude'] >= NYC['min_long']) & 
               (df['pickup_longitude'] <= NYC['max_long']))]

print(f"Datos dentro de NYC: {len(df_metro):,} registros ({len(df_metro)/len(df)*100:.1f}%)")
print(f"Datos fuera de NYC: {len(df) - len(df_metro):,} registros ({(len(df) - len(df_metro))/len(df)*100:.1f}%)")

# VISUALIZACIÓN
print(f"\nGenerando visualización...")

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 8))

# Muestra para visualización
if len(df) > 50000:
    df_viz = df.sample(n=50000, random_state=42)
else:
    df_viz = df

# 1. Distribución completa
ax1.scatter(df_viz['pickup_longitude'], df_viz['pickup_latitude'], 
           alpha=0.1, s=0.5, color='gray', marker='.')
ax1.set_title('Distribución Completa', fontweight='bold')
ax1.set_xlabel('Longitud')
ax1.set_ylabel('Latitud')

# Dibujar región NYC 
ax1.plot([NYC['min_long'], NYC['max_long'], NYC['max_long'], NYC['min_long'], NYC['min_long']],
         [NYC['min_lat'], NYC['min_lat'], NYC['max_lat'], NYC['max_lat'], NYC['min_lat']],
         'g--', linewidth=2, label='NYC ')

ax1.legend()
ax1.grid(True, alpha=0.3)

# 2. NYC 
if len(df_metro) > 0:
    metro_viz = df_metro.sample(n=min(50000, len(df_metro)), random_state=42) if len(df_metro) > 50000 else df_metro
    ax2.scatter(metro_viz['pickup_longitude'], metro_viz['pickup_latitude'], 
               alpha=0.3, s=1, color='blue', marker='.')
ax2.set_title('NYC ', fontweight='bold')
ax2.set_xlabel('Longitud')
ax2.set_ylabel('Latitud')
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('3Preprocesamiento/img/analisis_outliers_inteligente.png', dpi=300, bbox_inches='tight')
plt.close()

print("Visualización guardada: analisis_outliers_inteligente.png")

# RECOMENDACIÓN
print(f"\n=== RECOMENDACIÓN ===")
print("Basado en el análisis, recomiendo:")
print("1. Usar NYC como filtro principal (39°-42° lat, -75° a -72° long)")
print("2. Este filtro incluye toda la región metropolitana de NYC")
print("3. Elimina datos de otras ciudades y mantiene solo NYC")

print(f"\nProceso completado!")
print("=" * 60) 