import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.cluster import MiniBatchKMeans
from sklearn.metrics import silhouette_score

input_file = '2Database/4clusterizacion_geografica_ordenada.csv'
print(f"Leyendo archivo: {input_file}")
df = pd.read_csv(input_file)
print(f"Total de registros: {len(df):,}")

atributos = ['passenger_count','trip_distance','payment_type','fare_amount','extra','mta_tax','tip_amount','tolls_amount','improvement_surcharge','total_amount']

resultados = []

for cluster_id in sorted(df['cluster_espacial'].unique()):
    grupo = df[df['cluster_espacial'] == cluster_id].copy()
    n = len(grupo)
    print(f"\nAnalizando cluster geográfico {cluster_id} con {n:,} puntos...")
    if n < 3:
        print("  Muy pocos puntos, solo 1 subcluster.")
        resultados.append({'cluster': cluster_id, 'best_k': 1, 'silhouette': None})
        continue
    X = grupo[atributos].values
    X_scaled = StandardScaler().fit_transform(X)
    best_k = 2
    best_score = -1
    max_k = min(20, n // 1000)  # No probar más de 20 clusters o 1 por cada 1000 puntos
    if max_k < 2:
        max_k = 2
    for k in range(2, max_k+1):
        try:
            kmeans = MiniBatchKMeans(n_clusters=k, random_state=42)
            labels = kmeans.fit_predict(X_scaled)
            # Usar una muestra para eficiencia si el grupo es muy grande
            sample_size = min(10000, n)
            score = silhouette_score(X_scaled, labels, sample_size=sample_size, random_state=42)
            print(f'  k={k}, silhouette_score={score:.4f}')
            if score > best_score:
                best_score = score
                best_k = k
        except Exception as e:
            print(f'  k={k}, error: {e}')
    print(f'  Mejor número de subclusters: {best_k} (silhouette_score={best_score:.4f})')
    resultados.append({'cluster': cluster_id, 'best_k': best_k, 'silhouette': best_score})

# Mostrar resumen final
df_resultados = pd.DataFrame(resultados)
print("\nResumen óptimo de subclusters por cluster geográfico:")
print(df_resultados)

df_resultados.to_csv('2Database/analisis_optimo_subclusters_por_cluster.csv', index=False)
print("Resultados guardados en '2Database/analisis_optimo_subclusters_por_cluster.csv'") 