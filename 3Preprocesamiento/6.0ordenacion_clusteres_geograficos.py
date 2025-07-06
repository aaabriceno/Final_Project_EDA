import pandas as pd

# Leer el archivo con los clusters y centroides
input_file = '2Database/data500k/3clusterizacion_geografica_with_centroids.csv'
output_file = '2Database/data500k/4clusterizacion_geografica_ordenada.csv'

print(f"Leyendo archivo: {input_file}")
df = pd.read_csv(input_file)
print(f"Total de registros: {len(df):,}")

# Ordenar por cluster original, luego por latitud y longitud
print("Ordenando por cluster, latitud y longitud...")
df_sorted = df.sort_values(['cluster_espacial', 'pickup_latitude', 'pickup_longitude']).reset_index(drop=True)

# Guardar el archivo ordenado
print(f"Guardando archivo ordenado: {output_file}")
df_sorted.to_csv(output_file, index=False)
print("¡Archivo ordenado y guardado correctamente!") 