import os
import warnings
import numpy as np
import pandas as pd
import dask.dataframe as dd
import folium
import matplotlib.pyplot as plt
from sklearn.cluster import MiniBatchKMeans
from sklearn.metrics import silhouette_score, silhouette_samples

warnings.filterwarnings("ignore")

# 1. Carga de datos
archivo = '2Database/2processed_data_complete_limpio.csv'
df = dd.read_csv(archivo)
print("Columnas:", df.columns)
print(df.head())

# 2. Filtrado de outliers geográficos (bounding box de NYC)
bbox = {
    "lat_min": 40.5774,
    "lat_max": 40.9176,
    "lon_min": -74.15,
    "lon_max": -73.7004
}

# Outliers de pickup
outliers_pickup = df[
    (df.pickup_longitude <= bbox["lon_min"]) | (df.pickup_latitude <= bbox["lat_min"]) |
    (df.pickup_longitude >= bbox["lon_max"]) | (df.pickup_latitude >= bbox["lat_max"])
].compute()
print("Número de outliers (pickup):", len(outliers_pickup))

# Visualización de outliers (solo primeros 1000)
map_outliers = folium.Map(
    location=[40.734695, -73.990372],
    tiles='Stamen Toner',
    attr='Mapa del dataframe'
)
sample = outliers_pickup.head(1000)
for _, row in sample.iterrows():
    if row['pickup_latitude'] != 0:
        folium.Marker([row['pickup_latitude'], row['pickup_longitude']]).add_to(map_outliers)
map_outliers.save('3Preprocesamiento/img/outliers_pickup_map.html')
print("Mapa de outliers (pickup) guardado en: 3Preprocesamiento/img/outliers_pickup_map.html")

# 3. Filtrar el DataFrame para quedarte solo con los datos dentro del bounding box
df_clean = df[
    (df.pickup_longitude > bbox["lon_min"]) & (df.pickup_latitude > bbox["lat_min"]) &
    (df.pickup_longitude < bbox["lon_max"]) & (df.pickup_latitude < bbox["lat_max"])
].compute()
print("Datos limpios dentro del bounding box:", df_clean.shape)

# 4. Guardar el DataFrame limpio (opcional)
df_clean.to_csv('2Database/2processed_data_limpio_NYC.csv', index=False)
print("Archivo limpio guardado en: 2Database/2processed_data_limpio_NYC.csv")

# 5. Clustering geográfico con MiniBatchKMeans
coords = df_clean[['pickup_latitude', 'pickup_longitude']].values
n_clusters = 40  # Puedes ajustar este valor
kmeans = MiniBatchKMeans(n_clusters=n_clusters, batch_size=10000, random_state=42, init='k-means++')
kmeans.fit(coords)
df_clean['pickup_cluster'] = kmeans.predict(coords)
print(f"Clusterización terminada con {n_clusters} clusters.")

# 6. Visualización de los clusters
plt.figure(figsize=(10, 8))
plt.scatter(df_clean['pickup_longitude'], df_clean['pickup_latitude'], c=df_clean['pickup_cluster'], cmap='tab20', s=1, alpha=0.5)
plt.scatter(kmeans.cluster_centers_[:,1], kmeans.cluster_centers_[:,0], c='red', s=50, marker='x', label='Centroides')
plt.xlabel('Longitud')
plt.ylabel('Latitud')
plt.title('Clusters geográficos del dataframe')
plt.legend()
plt.tight_layout()
plt.savefig('3Preprocesamiento/img/clusters_geograficos.png', dpi=200)
plt.close()
print("Gráfico de clusters guardado en: 3Preprocesamiento/img/clusters_geograficos.png")

# 7. Visualización interactiva de los centroides en un mapa
map_clusters = folium.Map(
    location=[40.734695, -73.990372],
    tiles='Stamen Toner',
    attr='Mapa del dataframe'
)
for i, (lat, lon) in enumerate(kmeans.cluster_centers_):
    folium.Marker([lat, lon], popup=f'Cluster {i}').add_to(map_clusters)
map_clusters.save('3Preprocesamiento/img/centroides_clusters_map.html')
print("Mapa interactivo de centroides guardado en: 3Preprocesamiento/img/centroides_clusters_map.html")

# 8. Análisis de la calidad del clustering
# Tomar una muestra para el cálculo del silhouette (por eficiencia)
sample_size = min(10000, len(df_clean))
sample_indices = np.random.choice(df_clean.index, size=sample_size, replace=False)
sample_coords = df_clean.loc[sample_indices, ['pickup_latitude', 'pickup_longitude']].values
sample_labels = df_clean.loc[sample_indices, 'pickup_cluster'].values

sil_score = silhouette_score(sample_coords, sample_labels)
sil_samples = silhouette_samples(sample_coords, sample_labels)
print(f"Silhouette score promedio de la muestra: {sil_score:.4f}")

# Gráfica de silhouette
plt.figure(figsize=(10, 6))
y_lower = 10
for i in range(n_clusters):
    ith_cluster_silhouette_values = sil_samples[sample_labels == i]
    ith_cluster_silhouette_values.sort()
    size_cluster_i = ith_cluster_silhouette_values.shape[0]
    y_upper = y_lower + size_cluster_i
    plt.fill_betweenx(np.arange(y_lower, y_upper), 0, ith_cluster_silhouette_values, alpha=0.7)
    plt.text(-0.05, y_lower + 0.5 * size_cluster_i, str(i))
    y_lower = y_upper + 10  # espacio entre clusters

plt.axvline(x=sil_score, color="red", linestyle="--", label=f"Silhouette promedio: {sil_score:.2f}")
plt.xlabel("Valor del silhouette")
plt.ylabel("Índice de muestra")
plt.title("Gráfico de Silhouette para los clusters")
plt.legend()
plt.tight_layout()
plt.savefig('3Preprocesamiento/img/analisis_silhouette_clusters.png', dpi=200)
plt.close()
print("Gráfico de análisis de silhouette guardado en: 3Preprocesamiento/img/analisis_silhouette_clusters.png")
