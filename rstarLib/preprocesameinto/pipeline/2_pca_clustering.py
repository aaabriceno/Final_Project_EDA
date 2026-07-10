"""Paso 2 — PCA (componentes hasta 90% de varianza) + clustering atributivo
GLOBAL con MiniBatchKMeans (k elegido por silhouette).
Cambio clave vs pipeline viejo: ya NO hay cluster geografico (el R*-tree hace
el trabajo espacial); la etiqueta atributiva es global y comparable en todo
el dataset.
Uso: python3 2_pca_clustering.py [entrada.csv] [salida.csv]
"""
import sys
import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.decomposition import PCA
from sklearn.cluster import MiniBatchKMeans
from sklearn.metrics import silhouette_score

entrada = sys.argv[1] if len(sys.argv) > 1 else 'database/pipeline_1_limpio.csv'
salida = sys.argv[2] if len(sys.argv) > 2 else 'database/pipeline_2_etiquetado.csv'

df = pd.read_csv(entrada)
atributos = ['passenger_count', 'trip_distance', 'payment_type', 'fare_amount',
             'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge']
X = StandardScaler().fit_transform(df[atributos].values)

pca = PCA(n_components=0.90)   # componentes hasta 90% de varianza acumulada
Xp = pca.fit_transform(X)
print(f"PCA: {Xp.shape[1]} componentes ({pca.explained_variance_ratio_.sum()*100:.1f}% varianza)")

mejor_k, mejor_s = 2, -1.0
for k in range(2, 21):
    km = MiniBatchKMeans(n_clusters=k, random_state=42, n_init=3)
    labels = km.fit_predict(Xp)
    s = silhouette_score(Xp, labels, sample_size=min(10000, len(Xp)), random_state=42)
    print(f"  k={k}: silhouette={s:.4f}")
    if s > mejor_s:
        mejor_k, mejor_s = k, s
print(f"k elegido: {mejor_k} (silhouette {mejor_s:.4f})")

km = MiniBatchKMeans(n_clusters=mejor_k, random_state=42, n_init=3)
df_out = df[['tripID', 'pickup_latitude', 'pickup_longitude']].copy()
df_out['etiqueta'] = km.fit_predict(Xp)
for i in range(Xp.shape[1]):
    df_out[f'PC{i+1}'] = Xp[:, i].round(10)
df_out.to_csv(salida, index=False)
print(f"Guardado {salida}: {len(df_out):,} registros")
