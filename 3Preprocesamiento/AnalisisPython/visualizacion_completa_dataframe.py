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

# Definir el canvas (ajustar a NYC)
canvas = ds.Canvas(
    plot_width=2500,
    plot_height=1200,
    lat_min_max=(-74.15, -73.70),  # Bounding box NYC
    long_min_max=(40.58, 40.92)
)

# Crear el agregado de puntos
agg = canvas.points(df, 'pickup_longitude', 'pickup_latitude')

# Probar diferentes mapas de color y métodos de sombreado
colormaps = {
    'fire': cc.fire,
    'CET_L17': cc.CET_L17,
    'blues': cc.blues,
    'rainbow4': cc.rainbow4
}
hows = ['eq_hist', 'log']

for cmap_name, cmap in colormaps.items():
    for how in hows:
        img = tf.shade(agg, cmap=cmap, how=how)
        output_path = f'3Preprocesamiento/img/visualizacion_completa_datashader_{cmap_name}_{how}.png'
        export_image(img, output_path.replace('.png', ''))
        print(f"Imagen guardada: {output_path}")

print("\n¡Visualización completa generada!") 