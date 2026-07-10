"""Paso 1 — Limpieza: coordenadas validas NYC + clip de negativos.
Correccion clave vs pipeline viejo: los negativos se recortan a 0 (clip),
NO se desplaza la columna entera (eso inflaba todas las tarifas).
Uso: python3 1_limpiar.py [entrada.csv] [salida.csv]
"""
import sys
import pandas as pd

entrada = sys.argv[1] if len(sys.argv) > 1 else 'database/2Mpuntos.csv'
salida = sys.argv[2] if len(sys.argv) > 2 else 'database/pipeline_1_limpio.csv'

df = pd.read_csv(entrada)
print(f"Registros originales: {len(df):,}")

NYC = dict(min_lat=40.5774, max_lat=40.9176, min_lon=-74.15, max_lon=-73.7004)
mask = (
    df['pickup_latitude'].between(NYC['min_lat'], NYC['max_lat']) &
    df['pickup_longitude'].between(NYC['min_lon'], NYC['max_lon'])
)
df = df[mask].copy()
print(f"Tras filtro geografico: {len(df):,}")

tarifas = ['fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge']
for col in tarifas:
    negativos = int((df[col] < 0).sum())
    if negativos:
        print(f"  {col}: {negativos} negativos -> clip a 0")
    df[col] = df[col].clip(lower=0)
df['total_amount'] = df[tarifas].sum(axis=1).round(2)

df = df.dropna()
df.to_csv(salida, index=False)
print(f"Guardado {salida}: {len(df):,} registros")
