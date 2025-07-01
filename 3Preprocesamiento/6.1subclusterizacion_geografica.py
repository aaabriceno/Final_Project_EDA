import pandas as pd
import os
import matplotlib.pyplot as plt
from sklearn.cluster import MiniBatchKMeans

input_file = '2Database/4clusterizacion_geografica_ordenada.csv'
output_file = '2Database/4subclusterizacion_geografica.csv'
img_dir = '3Preprocesamiento/subclusterizaciongeo'

if not os.path.exists(img_dir):
    os.makedirs(img_dir)

print(f"Leyendo archivo: {input_file}")
df = pd.read_csv(input_file)
print(f"Total de registros: {len(df):,}")

df_resultados = []

for cluster_id in sorted(df['cluster_espacial'].unique()):
    grupo = df[df['cluster_espacial'] == cluster_id].copy()
    n = len(grupo)
    print(f"Procesando cluster geográfico {cluster_id} con {n:,} puntos...")
    if n < 2:
        grupo['subcluster_geo'] = 0
        df_resultados.append(grupo)
        continue
    X = grupo[['pickup_latitude', 'pickup_longitude']].values
    # Definir rango de k según el tamaño del grupo
    if n <= 4000:
        max_k = min(n, 10)
        k_values = range(2, max_k+1)
    else:
        k_values = range(2, 16)
    for k in k_values:
        kmeans_geo = MiniBatchKMeans(n_clusters=k, random_state=42)
        labels = kmeans_geo.fit_predict(X)
        inertia = kmeans_geo.inertia_
        print(f'  k={k}, inercia={inertia:.2f}')
        # Graficar y guardar imagen
        plt.figure(figsize=(8, 6))
        plt.scatter(grupo['pickup_longitude'], grupo['pickup_latitude'], c=labels, cmap='tab20', s=1, alpha=0.5)
        plt.scatter(kmeans_geo.cluster_centers_[:,1], kmeans_geo.cluster_centers_[:,0], c='red', s=50, marker='x', label='Centroides')
        plt.xlabel('Longitud')
        plt.ylabel('Latitud')
        plt.title(f'Subclusterización geográfica - Cluster {cluster_id} (k={k})')
        plt.legend()
        plt.tight_layout()
        img_path = os.path.join(img_dir, f'image_cluster{cluster_id}_k{k}.png')
        plt.savefig(img_path, dpi=150)
        plt.close()
        print(f'    Imagen guardada: {img_path}')
    # Por defecto, asignar el primer k probado (puedes cambiar esto luego)
    grupo['subcluster_geo'] = MiniBatchKMeans(n_clusters=k_values[0], random_state=42).fit_predict(X)
    df_resultados.append(grupo)

# Concatenar todos los resultados
df_final = pd.concat(df_resultados, ignore_index=True)

# Guardar el archivo final
print(f"Guardando archivo final: {output_file}")
df_final.to_csv(output_file, index=False)
print("¡Archivo guardado correctamente!") 