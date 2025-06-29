import pandas as pd
import datashader as ds
import datashader.transfer_functions as tf
from datashader.utils import export_image
import colorcet as cc

print("=== VISUALIZACIÓN COMPLETA DE TODOS LOS PUNTOS CON DATASHADER ===")

# Cargar el dataset completo
print("Cargando datos completos de NYC...")
df = pd.read_csv('2Database/2processed_data_complete_limpio.csv')
print(f"Dataset shape: {df.shape}")

# Definir el canvas (ajustar según el rango de tus datos)
canvas = ds.Canvas(
    plot_width=1800,
    plot_height=900,
    x_range=(df['pickup_longitude'].min(), df['pickup_longitude'].max()),
    y_range=(df['pickup_latitude'].min(), df['pickup_latitude'].max())
)

# Crear el agregado de puntos
agg = canvas.points(df, 'pickup_longitude', 'pickup_latitude')

# Renderizar la imagen con un mapa de color adecuado
img = tf.shade(agg, cmap=cc.fire, how='eq_hist')

# Guardar la imagen
output_path = '3Preprocesamiento/img/visualizacion_completa_datashader.png'
export_image(img, output_path.replace('.png', ''))
print(f"Imagen guardada: {output_path}")

print("\n¡Visualización completa generada!") 