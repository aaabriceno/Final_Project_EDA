import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.cluster import KMeans, DBSCAN
from sklearn.preprocessing import StandardScaler
from scipy import stats

#Analizamos la parta geografica lat y long para 
#ver la existencia de outliers

# Cargar el dataset limpio
print("Cargando dataset...")
df = pd.read_csv('Database/2processed_data_complete_limpio.csv')

print("=== ANÁLISIS DE DISTRIBUCIÓN DE COORDENADAS ===")
print(f"Dataset Tamano: {df.shape}")
print(f"Columnas: {list(df.columns)}")

# Para datasets grandes, usar una muestra para visualización
if len(df) > 100000:
    print(f"\nDataset muy grande ({len(df):,} registros). Usando muestra de 100,000 para visualización...")
    df_sample = df.sample(n=100000, random_state=42)
    print(f"Muestra para visualización: {len(df_sample):,} registros")
else:
    df_sample = df

# Análisis de coordenadas
print("\nESTADÍSTICAS DE LATITUD")
print(df['pickup_latitude'].describe())
print(f"Valores únicos en lat: {df['pickup_latitude'].nunique()}")

print("\nESTADÍSTICAS DE LONGITUD")
print(df['pickup_longitude'].describe())
print(f"Valores únicos en long: {df['pickup_longitude'].nunique()}")

# Detectar outliers (0,0) y valores sospechosos
print("\nDETECCIÓN DE OUTLIERS")
ceros_lat = (df['pickup_latitude'] == 0).sum()
ceros_long = (df['pickup_longitude'] == 0).sum()
ceros_ambos = ((df['pickup_latitude'] == 0) & (df['pickup_longitude'] == 0)).sum()

print(f"Puntos con lat = 0: {ceros_lat:,}")
print(f"Puntos con long = 0: {ceros_long:,}")
print(f"Puntos con (lat, long) = (0, 0): {ceros_ambos:,}")

# Detectar otros valores sospechosos o que no estan entre valores de lat y long
print(f"\nPuntos con lat < -90 o lat > 90: {((df['pickup_latitude'] < -90) | (df['pickup_latitude'] > 90)).sum():,}")
print(f"Puntos con long < -180 o long > 180: {((df['pickup_longitude'] < -180) | (df['pickup_longitude'] > 180)).sum():,}")

# Análisis de distribución sin outliers
df_clean = df[(df['pickup_latitude'] != 0) & (df['pickup_longitude'] != 0) & 
              (df['pickup_latitude'] >= -90) & (df['pickup_latitude'] <= 90) &
              (df['pickup_longitude'] >= -180) & (df['pickup_longitude'] <= 180)]

print(f"\nPuntos válidos (sin outliers): {len(df_clean):,}")
print(f"Porcentaje de datos válidos: {len(df_clean)/len(df)*100:.2f}%")

if len(df_clean) > 0:
    print("\nESTADÍSTICAS SIN OUTLIERS")
    print("Latitud:")
    print(df_clean['pickup_latitude'].describe())
    print("\nLongitud:")
    print(df_clean['pickup_longitude'].describe())

# Configurar matplotlib para mejor rendimiento
plt.rcParams['figure.dpi'] = 250
plt.rcParams['savefig.dpi'] = 300

# IMAGEN 1: Distribución de latitud y longitud
print("\nGenerando imagen 1: Distribución de lat/long...")
plt.figure(figsize=(12, 8))

# Distribución de latitud
plt.subplot(1, 2, 1)
plt.hist(df_sample['pickup_latitude'], bins=100, alpha=0.7, color='blue', edgecolor='black')
plt.title('Distribución de Latitud', fontsize=14, fontweight='bold')
plt.xlabel('Latitud', fontsize=12)
plt.ylabel('Frecuencia', fontsize=12)
plt.grid(True, alpha=0.3)

# Distribución de longitud
plt.subplot(1, 2, 2)
plt.hist(df_sample['pickup_longitude'], bins=100, alpha=0.7, color='red', edgecolor='black')
plt.title('Distribución de Longitud', fontsize=14, fontweight='bold')
plt.xlabel('Longitud', fontsize=12)
plt.ylabel('Frecuencia', fontsize=12)
plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('Preprocesamiento/distribucion_lat_long.png', dpi=300, bbox_inches='tight')
plt.close()  # Cerrar figura para liberar memoria
print("Imagen 1 guardada: distribucion_lat_long.png")

# IMAGEN 2: Distribución de puntos en el mapa
print("Generando imagen 2: Distribución de puntos en mapa...")
plt.figure(figsize=(12, 8))

# Scatter plot de coordenadas (usar TODOS los puntos)
plt.scatter(df['pickup_longitude'], df['pickup_latitude'], 
           alpha=0.1, s=0.1, color='blue', marker='.')
plt.title('Distribución de Coordenadas Geográficas', fontsize=16, fontweight='bold')
plt.xlabel('Longitud', fontsize=14)
plt.ylabel('Latitud', fontsize=14)
plt.grid(True, alpha=0.3)

#Líneas de referencia para NYC
plt.axhline(y=40.7128, color='red', linestyle='--', alpha=0.7, label='NYC Lat (40.7128°)')
plt.axvline(x=-74.0060, color='red', linestyle='--', alpha=0.7, label='NYC Long (-74.0060°)')
plt.legend()

plt.tight_layout()
plt.savefig('Preprocesamiento/distribucion_puntos_mapa.png', dpi=300, bbox_inches='tight')
plt.close()  # Cerrar figura para liberar memoria
print("Imagen 2 guardada: distribucion_puntos_mapa.png (con todos los puntos)")

# IMAGEN 3: Box plots de latitud y longitud
print("Generando imagen 3: Box plots...")
plt.figure(figsize=(12, 5))

# Box plot de latitud
plt.subplot(1, 2, 1)
plt.boxplot(df_sample['pickup_latitude'], patch_artist=True, boxprops=dict(facecolor='lightblue'))
plt.title('Box Plot - Latitud', fontsize=14, fontweight='bold')
plt.ylabel('Latitud', fontsize=12)
plt.grid(True, alpha=0.3)

# Box plot de longitud
plt.subplot(1, 2, 2)
plt.boxplot(df_sample['pickup_longitude'], patch_artist=True, boxprops=dict(facecolor='lightcoral'))
plt.title('Box Plot - Longitud', fontsize=14, fontweight='bold')
plt.ylabel('Longitud', fontsize=12)
plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('Preprocesamiento/boxplots_lat_long.png', dpi=300, bbox_inches='tight')
plt.close()  # Cerrar figura para liberar memoria
print("Imagen 3 guardada: boxplots_lat_long.png")

# IMAGEN 4: Distribución sin outliers (opcional)
if len(df_clean) > 0:
    print("Generando imagen 4: Distribución sin outliers...")
    plt.figure(figsize=(12, 8))
    
    # Usar muestra para scatter plot sin outliers
    if len(df_clean) > 100000:
        df_clean_sample = df_clean.sample(n=100000, random_state=42)
    else:
        df_clean_sample = df_clean
    
    plt.scatter(df_clean_sample['pickup_longitude'], df_clean_sample['pickup_latitude'], 
               alpha=0.3, s=0.5, color='green', marker='.')
    plt.title('Distribución de Coordenadas Sin Outliers', fontsize=16, fontweight='bold')
    plt.xlabel('Longitud', fontsize=14)
    plt.ylabel('Latitud', fontsize=14)
    plt.grid(True, alpha=0.3)
    
    # Agregar líneas de referencia para NYC
    plt.axhline(y=40.7128, color='red', linestyle='--', alpha=0.7, label='NYC Lat (40.7128°)')
    plt.axvline(x=-74.0060, color='red', linestyle='--', alpha=0.7, label='NYC Long (-74.0060°)')
    plt.legend()
    
    plt.tight_layout()
    plt.savefig('Preprocesamiento/distribucion_sin_outliers.png', dpi=300, bbox_inches='tight')
    plt.close()  # Cerrar figura para liberar memoria
    print("Imagen 4 guardada: distribucion_sin_outliers.png")

# Análisis de componentes PCA (si existen en el archivo limpio)
print("\n=== ANÁLISIS DE COMPONENTES PCA ===")
pca_columns = [col for col in df.columns if col.startswith('PC')]
print(f"Componentes PCA encontradas: {pca_columns}")

if len(pca_columns) > 0:
    print("\nEstadísticas de componentes PCA:")
    print(df[pca_columns].describe())
    
    # Correlación entre componentes
    print("\nMatriz de correlación entre componentes PCA:")
    corr_matrix = df[pca_columns].corr()
    print(corr_matrix)
    
    # Visualizar correlaciones
    print("Generando imagen 5: Correlación PCA...")
    plt.figure(figsize=(8, 6))
    sns.heatmap(corr_matrix, annot=True, cmap='coolwarm', center=0)
    plt.title('Correlación entre Componentes PCA')
    plt.tight_layout()
    plt.savefig('Preprocesamiento/correlacion_pca.png', dpi=300, bbox_inches='tight')
    plt.close()
    print("Imagen 5 guardada: correlacion_pca.png")
else:
    print("No se encontraron componentes PCA en el archivo limpio.")

print("\n=== RECOMENDACIONES PARA CLUSTERING ===")
print("1. FASE 1 - Clustering Espacial:")
print("   - Usar DBSCAN para detectar clusters naturales")
print("   - Filtrar outliers (0,0) antes del clustering")
print("   - Considerar K-means si los clusters son bien definidos")

print("\n2. FASE 2 - Clustering por Atributos:")
print("   - Aplicar K-means a las componentes PCA dentro de cada cluster espacial")
print("   - Usar número de clusters proporcional al tamaño del cluster espacial")

print("\n3. ESTRATEGIA RECOMENDADA:")
print("   - DBSCAN para clustering espacial (maneja outliers automáticamente)")
print("   - K-means para clustering de atributos (más control sobre número de clusters)")

print("\n¡Análisis completado! Todas las imágenes han sido generadas.") 