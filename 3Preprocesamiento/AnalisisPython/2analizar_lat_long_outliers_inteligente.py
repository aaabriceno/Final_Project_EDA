import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.cluster import DBSCAN
from sklearn.preprocessing import StandardScaler
import seaborn as sns

print("=== ANÁLISIS INTELIGENTE DE OUTLIERS PARA NYC ===")

# Cargar el dataset
print("Cargando dataset...")
df = pd.read_csv('2Database/1processed_data_complete_limpio.csv')
print(f"Dataset original: {df.shape}")

# Definir rangos de NYC (más conservadores)
NYC_CORE = {
    'min_lat': 40.5,    # Manhattan y áreas centrales
    'max_lat': 40.9,    
    'min_long': -74.0,  
    'max_long': -73.7   
}

# Definir MBR ampliado para región metropolitana
NYC_METRO = {
    'min_lat': 40.4,    # Incluye Staten Island, Bronx, Queens
    'max_lat': 41.0,    
    'min_long': -74.3,  # Incluye NJ, aeropuertos
    'max_long': -73.5   
}

print(f"\nRangos definidos:")
print(f"NYC Core: Lat {NYC_CORE['min_lat']}°-{NYC_CORE['max_lat']}°, Long {NYC_CORE['min_long']}°-{NYC_CORE['max_long']}°")
print(f"NYC Metro: Lat {NYC_METRO['min_lat']}°-{NYC_METRO['max_lat']}°, Long {NYC_METRO['min_long']}°-{NYC_METRO['max_long']}°")

# ANÁLISIS DE OUTLIERS
print(f"\n=== ANÁLISIS DE OUTLIERS ===")

# 1. Detectar errores claros (0.0)
errores_cero = ((df['pickup_latitude'] == 0) & (df['pickup_longitude'] == 0)).sum()
print(f"Errores claros (0,0): {errores_cero:,}")

# 2. Detectar puntos fuera de NYC Core
fuera_core = ((df['pickup_latitude'] < NYC_CORE['min_lat']) | 
              (df['pickup_latitude'] > NYC_CORE['max_lat']) |
              (df['pickup_longitude'] < NYC_CORE['min_long']) | 
              (df['pickup_longitude'] > NYC_CORE['max_long'])).sum()

print(f"Puntos fuera de NYC Core: {fuera_core:,}")

# 3. Detectar puntos fuera de NYC Metro
fuera_metro = ((df['pickup_latitude'] < NYC_METRO['min_lat']) | 
               (df['pickup_latitude'] > NYC_METRO['max_lat']) |
               (df['pickup_longitude'] < NYC_METRO['min_long']) | 
               (df['pickup_longitude'] > NYC_METRO['max_long'])).sum()

print(f"Puntos fuera de NYC Metro: {fuera_metro:,}")

# 4. Puntos en la zona intermedia (entre Core y Metro)
zona_intermedia = ((df['pickup_latitude'] >= NYC_CORE['min_lat']) & 
                   (df['pickup_latitude'] <= NYC_CORE['max_lat']) &
                   (df['pickup_longitude'] >= NYC_CORE['min_long']) & 
                   (df['pickup_longitude'] <= NYC_CORE['max_long'])).sum()

zona_metro = ((df['pickup_latitude'] >= NYC_METRO['min_lat']) & 
              (df['pickup_latitude'] <= NYC_METRO['max_lat']) &
              (df['pickup_longitude'] >= NYC_METRO['min_long']) & 
              (df['pickup_longitude'] <= NYC_METRO['max_long'])).sum()

zona_intermedia_real = zona_metro - zona_intermedia
print(f"Puntos en zona intermedia (metro pero no core): {zona_intermedia_real:,}")

# 5. Análisis de coordenadas inválidas
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

# Identificar outliers (fuera de NYC Metro)
outliers_mask = ((df_valido['pickup_latitude'] < NYC_METRO['min_lat']) | 
                 (df_valido['pickup_latitude'] > NYC_METRO['max_lat']) |
                 (df_valido['pickup_longitude'] < NYC_METRO['min_long']) | 
                 (df_valido['pickup_longitude'] > NYC_METRO['max_long']))

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
            nyc_center_lat = (NYC_CORE['min_lat'] + NYC_CORE['max_lat']) / 2
            nyc_center_long = (NYC_CORE['min_long'] + NYC_CORE['max_long']) / 2
            
            distancias = np.sqrt((cluster_data['pickup_latitude'] - nyc_center_lat)**2 + 
                                (cluster_data['pickup_longitude'] - nyc_center_long)**2)
            print(f"  Distancia promedio a NYC: {distancias.mean():.4f}°")
            
            # Convertir a km aproximados (1° ≈ 111 km)
            dist_km = distancias.mean() * 111
            print(f"  Distancia promedio a NYC: {dist_km:.1f} km")

# ESTRATEGIAS DE FILTRADO
print(f"\n=== ESTRATEGIAS DE FILTRADO ===")

# Estrategia 1: Solo NYC Core
df_core = df[((df['pickup_latitude'] >= NYC_CORE['min_lat']) & 
              (df['pickup_latitude'] <= NYC_CORE['max_lat']) &
              (df['pickup_longitude'] >= NYC_CORE['min_long']) & 
              (df['pickup_longitude'] <= NYC_CORE['max_long']))]

# Estrategia 2: NYC Metro (incluye zona intermedia)
df_metro = df[((df['pickup_latitude'] >= NYC_METRO['min_lat']) & 
               (df['pickup_latitude'] <= NYC_METRO['max_lat']) &
               (df['pickup_longitude'] >= NYC_METRO['min_long']) & 
               (df['pickup_longitude'] <= NYC_METRO['max_long']))]

# Estrategia 3: Eliminar solo errores claros
df_sin_errores = df[~((df['pickup_latitude'] == 0) & (df['pickup_longitude'] == 0))]
df_sin_errores = df_sin_errores[~(((df_sin_errores['pickup_latitude'] < -90) | (df_sin_errores['pickup_latitude'] > 90)) |
                                  ((df_sin_errores['pickup_longitude'] < -180) | (df_sin_errores['pickup_longitude'] > 180)))]

print(f"Estrategia 1 (NYC Core): {len(df_core):,} registros ({len(df_core)/len(df)*100:.1f}%)")
print(f"Estrategia 2 (NYC Metro): {len(df_metro):,} registros ({len(df_metro)/len(df)*100:.1f}%)")
print(f"Estrategia 3 (Sin errores): {len(df_sin_errores):,} registros ({len(df_sin_errores)/len(df)*100:.1f}%)")

# VISUALIZACIÓN
print(f"\nGenerando visualización...")

fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 12))

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

# Dibujar regiones
ax1.plot([NYC_CORE['min_long'], NYC_CORE['max_long'], NYC_CORE['max_long'], NYC_CORE['min_long'], NYC_CORE['min_long']],
         [NYC_CORE['min_lat'], NYC_CORE['min_lat'], NYC_CORE['max_lat'], NYC_CORE['max_lat'], NYC_CORE['min_lat']],
         'r-', linewidth=2, label='NYC Core')

ax1.plot([NYC_METRO['min_long'], NYC_METRO['max_long'], NYC_METRO['max_long'], NYC_METRO['min_long'], NYC_METRO['min_long']],
         [NYC_METRO['min_lat'], NYC_METRO['min_lat'], NYC_METRO['max_lat'], NYC_METRO['max_lat'], NYC_METRO['min_lat']],
         'g--', linewidth=2, label='NYC Metro')

ax1.legend()
ax1.grid(True, alpha=0.3)

# 2. NYC Core
if len(df_core) > 0:
    core_viz = df_core.sample(n=min(50000, len(df_core)), random_state=42) if len(df_core) > 50000 else df_core
    ax2.scatter(core_viz['pickup_longitude'], core_viz['pickup_latitude'], 
               alpha=0.3, s=1, color='red', marker='.')
ax2.set_title('NYC Core', fontweight='bold')
ax2.set_xlabel('Longitud')
ax2.set_ylabel('Latitud')
ax2.grid(True, alpha=0.3)

# 3. NYC Metro
if len(df_metro) > 0:
    metro_viz = df_metro.sample(n=min(50000, len(df_metro)), random_state=42) if len(df_metro) > 50000 else df_metro
    ax3.scatter(metro_viz['pickup_longitude'], metro_viz['pickup_latitude'], 
               alpha=0.3, s=1, color='blue', marker='.')
ax3.set_title('NYC Metro', fontweight='bold')
ax3.set_xlabel('Longitud')
ax3.set_ylabel('Latitud')
ax3.grid(True, alpha=0.3)

# 4. Outliers
if len(df_outliers) > 0:
    outliers_viz = df_outliers.sample(n=min(10000, len(df_outliers)), random_state=42) if len(df_outliers) > 10000 else df_outliers
    ax4.scatter(outliers_viz['pickup_longitude'], outliers_viz['pickup_latitude'], 
               alpha=0.5, s=2, color='orange', marker='.')
ax4.set_title('Outliers', fontweight='bold')
ax4.set_xlabel('Longitud')
ax4.set_ylabel('Latitud')
ax4.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('3Preprocesamiento/img/analisis_outliers_inteligente.png', dpi=300, bbox_inches='tight')
plt.close()

print("Visualización guardada: analisis_outliers_inteligente.png")

# RECOMENDACIÓN
print(f"\n=== RECOMENDACIÓN ===")
print("Basado en el análisis, recomiendo:")
print("1. Eliminar errores claros (0,0) y coordenadas inválidas")
print("2. Usar NYC Metro como filtro principal (incluye aeropuertos y áreas cercanas)")
print("3. Considerar clusters de outliers que estén a menos de 50km de NYC")

print(f"\nProceso completado!")
print("=" * 60) 