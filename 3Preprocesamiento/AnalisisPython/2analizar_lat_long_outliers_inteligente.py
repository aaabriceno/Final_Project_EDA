import pandas as pd
import numpy as np
import datashader as ds
import datashader.transfer_functions as tf
from datashader.utils import export_image
import colorcet as cc

print("=== ANÁLISIS INTELIGENTE DE OUTLIERS PARA NYC ===")

# Cargar el dataset
print("Cargando dataset...")
df = pd.read_csv('2Database/1processed_data_complete.csv')
print(f"Dataset original: {df.shape}")

# Definir límites amplios para toda la región metropolitana de NYC
NYC = {
    'min_lat': 40.5774,    # Límite sur amplio (incluye NJ, Long Island)
    'max_lat': 40.9176,    # Límite norte amplio (incluye Connecticut, Upstate NY)
    'min_long': -74.15,  # Límite oeste amplio (incluye Pennsylvania)
    'max_long': -73.7004   # Límite este amplio (incluye Long Island, Connecticut)
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

# Filtrar datos válidos (sin errores claros ni coordenadas inválidas)
df_valido = df[~((df['pickup_latitude'] == 0) & (df['pickup_longitude'] == 0))]
df_valido = df_valido[~(((df_valido['pickup_latitude'] < -90) | (df_valido['pickup_latitude'] > 90)) |
                        ((df_valido['pickup_longitude'] < -180) | (df_valido['pickup_longitude'] > 180)))]
print(f"Datos válidos para análisis: {len(df_valido):,}")

# Solo NYC Metro (incluye toda la región metropolitana)
df_metro = df_valido[((df_valido['pickup_latitude'] >= NYC['min_lat']) & 
                      (df_valido['pickup_latitude'] <= NYC['max_lat']) &
                      (df_valido['pickup_longitude'] >= NYC['min_long']) & 
                      (df_valido['pickup_longitude'] <= NYC['max_long']))]

print(f"Datos dentro de NYC: {len(df_metro):,} registros ({len(df_metro)/len(df)*100:.1f}%)")
print(f"Datos fuera de NYC: {len(df) - len(df_metro):,} registros ({(len(df) - len(df_metro))/len(df)*100:.1f}%)")

# VISUALIZACIÓN CON DATASHADER
print(f"\nGenerando visualización con datashader...")

# 1. Distribución completa
def plot_datashader(df, x, y, x_range, y_range, filename, title):
    cvs = ds.Canvas(plot_width=1000, plot_height=1000, x_range=x_range, y_range=y_range)
    agg = cvs.points(df, x, y)
    img = tf.shade(agg, cmap=cc.fire, how='eq_hist')
    export_image(img, filename)
    print(f"Imagen guardada: {filename}.png")

# Visualización solo NYC
df_nyc_plot = df_metro[['pickup_longitude', 'pickup_latitude']].dropna()
plot_datashader(
    df_nyc_plot,
    x='pickup_longitude',
    y='pickup_latitude',
    x_range=(NYC['min_long'], NYC['max_long']),
    y_range=(NYC['min_lat'], NYC['max_lat']),
    filename='3Preprocesamiento/img/analisis_outliers_inteligente_datashader_nyc',
    title='NYC (Datashader)'
)

# RECOMENDACIÓN
print(f"\n=== RECOMENDACIÓN ===")
print("Basado en el análisis, recomiendo:")
print("1. Usar NYC como filtro principal (40.5774°-40.9176° lat, -74.15° a -73.7004° long)")
print("2. Este filtro incluye toda la región metropolitana de NYC")
print("3. Elimina datos de otras ciudades y mantiene solo NYC")

print(f"\nProceso completado!")
print("=" * 60) 