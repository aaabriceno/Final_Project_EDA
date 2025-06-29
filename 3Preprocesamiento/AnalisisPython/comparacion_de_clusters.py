import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.cluster import MiniBatchKMeans, DBSCAN, OPTICS
from sklearn.mixture import GaussianMixture
from sklearn.preprocessing import StandardScaler
import warnings
warnings.filterwarnings('ignore')

print("=== COMPARACIÓN VISUAL DE MÉTODOS DE CLUSTERIZACIÓN ===")

# Intentar importar HDBSCAN
try:
    import hdbscan
    hdbscan_available = True
except ImportError:
    hdbscan_available = False
    print("\n[ADVERTENCIA] El paquete 'hdbscan' no está instalado. Puedes instalarlo con: pip install hdbscan\n")

# Cargar datos limpios de NYC
print("Cargando datos limpios de NYC...")
df = pd.read_csv('2Database/2processed_data_complete_limpio.csv')
print(f"Dataset shape: {df.shape}")

# Tomar una muestra para visualización
n_sample = 50000
if len(df) > n_sample:
    df_sample = df.sample(n=n_sample, random_state=42)
else:
    df_sample = df
print(f"Muestra para clustering: {len(df_sample):,} puntos")

# Extraer coordenadas
coords = df_sample[['pickup_latitude', 'pickup_longitude']].values

# Normalizar para métodos basados en distancia
scaler = StandardScaler()
coords_norm = scaler.fit_transform(coords)

# MiniBatchKMeans
print("Aplicando MiniBatchKMeans...")
n_clusters = 20
kmeans = MiniBatchKMeans(n_clusters=n_clusters, random_state=42, batch_size=10000, n_init=5, max_iter=100)
labels_kmeans = kmeans.fit_predict(coords)

# DBSCAN
print("Aplicando DBSCAN...")
dbscan = DBSCAN(eps=0.08, min_samples=20)
labels_dbscan = dbscan.fit_predict(coords_norm)

# OPTICS
print("Aplicando OPTICS...")
optics = OPTICS(min_samples=20, xi=0.05, min_cluster_size=0.05)
labels_optics = optics.fit_predict(coords_norm)

# GMM
print("Aplicando Gaussian Mixture Model...")
gmm = GaussianMixture(n_components=n_clusters, covariance_type='full', random_state=42)
gmm_labels = gmm.fit_predict(coords)

# HDBSCAN (si está disponible)
if hdbscan_available:
    print("Aplicando HDBSCAN...")
    hdb = hdbscan.HDBSCAN(min_cluster_size=100, min_samples=20)
    labels_hdbscan = hdb.fit_predict(coords_norm)
else:
    labels_hdbscan = None

# Visualización
fig, axes = plt.subplots(2, 3, figsize=(24, 14))
axes = axes.flatten()

# MiniBatchKMeans
scatter1 = axes[0].scatter(df_sample['pickup_longitude'], df_sample['pickup_latitude'], c=labels_kmeans, cmap='tab20', s=1, alpha=0.7)
axes[0].set_title('MiniBatchKMeans (20 clusters)', fontsize=13, fontweight='bold')
axes[0].set_xlabel('Longitud')
axes[0].set_ylabel('Latitud')
axes[0].grid(True, alpha=0.3)
plt.colorbar(scatter1, ax=axes[0], label='Cluster')

# DBSCAN
scatter2 = axes[1].scatter(df_sample['pickup_longitude'], df_sample['pickup_latitude'], c=labels_dbscan, cmap='tab20', s=1, alpha=0.7)
axes[1].set_title('DBSCAN (eps=0.08, min_samples=20)', fontsize=13, fontweight='bold')
axes[1].set_xlabel('Longitud')
axes[1].set_ylabel('Latitud')
axes[1].grid(True, alpha=0.3)
plt.colorbar(scatter2, ax=axes[1], label='Cluster/Noise')

# OPTICS
scatter3 = axes[2].scatter(df_sample['pickup_longitude'], df_sample['pickup_latitude'], c=labels_optics, cmap='tab20', s=1, alpha=0.7)
axes[2].set_title('OPTICS (min_samples=20)', fontsize=13, fontweight='bold')
axes[2].set_xlabel('Longitud')
axes[2].set_ylabel('Latitud')
axes[2].grid(True, alpha=0.3)
plt.colorbar(scatter3, ax=axes[2], label='Cluster/Noise')

# GMM
scatter4 = axes[3].scatter(df_sample['pickup_longitude'], df_sample['pickup_latitude'], c=gmm_labels, cmap='tab20', s=1, alpha=0.7)
axes[3].set_title('Gaussian Mixture Model (20 comp.)', fontsize=13, fontweight='bold')
axes[3].set_xlabel('Longitud')
axes[3].set_ylabel('Latitud')
axes[3].grid(True, alpha=0.3)
plt.colorbar(scatter4, ax=axes[3], label='Cluster')

# HDBSCAN (si está disponible)
if labels_hdbscan is not None:
    scatter5 = axes[4].scatter(df_sample['pickup_longitude'], df_sample['pickup_latitude'], c=labels_hdbscan, cmap='tab20', s=1, alpha=0.7)
    axes[4].set_title('HDBSCAN (min_cluster_size=100)', fontsize=13, fontweight='bold')
    axes[4].set_xlabel('Longitud')
    axes[4].set_ylabel('Latitud')
    axes[4].grid(True, alpha=0.3)
    plt.colorbar(scatter5, ax=axes[4], label='Cluster/Noise')
else:
    axes[4].text(0.5, 0.5, 'HDBSCAN no disponible', ha='center', va='center', fontsize=14)
    axes[4].set_axis_off()

# Panel vacío para balancear
axes[5].set_axis_off()

plt.tight_layout()
plt.savefig('3Preprocesamiento/img/comparacion_clusters.png', dpi=300, bbox_inches='tight')
plt.close()
print("Visualización guardada: comparacion_clusters.png")

print("\n¡Comparación completada!") 