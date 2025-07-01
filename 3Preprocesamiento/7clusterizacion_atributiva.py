import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.cluster import MiniBatchKMeans

# Leer el archivo ordenado
input_file = '2Database/4clusterizacion_geografica_ordenada.csv'
output_file = '2Database/5clusterizacion_geografica_attr.csv'

print(f"Leyendo archivo: {input_file}")
df = pd.read_csv(input_file)
print(f"Total de registros: {len(df):,}")

# Atributos para clusterización atributiva
atributos = ['passenger_count','trip_distance','payment_type','fare_amount','extra','mta_tax','tip_amount','tolls_amount'
             ,'improvement_surcharge','total_amount']

# DataFrame para resultados
df_resultados = []

# Iterar por cada cluster geográfico
for cluster_id in sorted(df['cluster_espacial'].unique()):
    grupo = df[df['cluster_espacial'] == cluster_id].copy()
    print(f"Procesando cluster geográfico {cluster_id} con {len(grupo):,} puntos...")
    if len(grupo) < 2:
        grupo['cluster_atributivo'] = 0
        df_resultados.append(grupo)
        continue
    # Estandarizar atributos
    X = grupo[atributos].values
    X_scaled = StandardScaler().fit_transform(X)
    # Elegir número de clusters atributivos (puedes ajustar)
    n_clusters_attr = min(5, len(grupo))  # Máximo 5 clusters o menos si hay pocos puntos
    kmeans_attr = MiniBatchKMeans(n_clusters=n_clusters_attr, random_state=42)
    grupo['cluster_atributivo'] = kmeans_attr.fit_predict(X_scaled)
    df_resultados.append(grupo)

# Concatenar todos los resultados
print("Concatenando resultados...")
df_final = pd.concat(df_resultados, ignore_index=True)

# Reordenar columnas: primero cluster_espacial, luego cluster_atributivo, luego el resto
columnas = list(df_final.columns)
# Quitar los clusters si ya están
columnas.remove('cluster_espacial')
columnas.remove('cluster_atributivo')
columnas_final = ['cluster_espacial', 'cluster_atributivo'] + columnas

# Guardar el archivo final
print(f"Guardando archivo final: {output_file}")
df_final[columnas_final].to_csv(output_file, index=False)
print("¡Archivo guardado correctamente!") 