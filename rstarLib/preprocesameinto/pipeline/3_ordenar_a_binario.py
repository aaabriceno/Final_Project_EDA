"""Paso 3 — Orden por (lat, lon) + exportar .bin (escritura vectorizada).
El orden mejora el empaquetado del R*-tree en la carga (menos splits).
Formato binario:
  numPuntos (int32)
  por punto: tripID(int32), lat(f64), lon(f64), etiqueta(int32),
             numPCs(int32), PCs(f64 x numPCs)
Uso: python3 3_ordenar_a_binario.py [entrada.csv] [salida.bin]
"""
import sys
import numpy as np
import pandas as pd

entrada = sys.argv[1] if len(sys.argv) > 1 else 'database/pipeline_2_etiquetado.csv'
salida = sys.argv[2] if len(sys.argv) > 2 else 'database/pipeline_3_datos.bin'

df = pd.read_csv(entrada).sort_values(['pickup_latitude', 'pickup_longitude'])
pcs = [c for c in df.columns if c.startswith('PC')]
n, npc = len(df), len(pcs)

registro = np.dtype([('id', '<i4'), ('lat', '<f8'), ('lon', '<f8'),
                     ('etiqueta', '<i4'), ('npc', '<i4'), ('pcs', '<f8', (npc,))])
arr = np.empty(n, dtype=registro)
arr['id'] = df['tripID'].to_numpy(np.int32)
arr['lat'] = df['pickup_latitude'].to_numpy()
arr['lon'] = df['pickup_longitude'].to_numpy()
arr['etiqueta'] = df['etiqueta'].to_numpy(np.int32)
arr['npc'] = npc
arr['pcs'] = df[pcs].to_numpy()

with open(salida, 'wb') as f:
    np.array([n], dtype=np.int32).tofile(f)
    arr.tofile(f)
print(f"Guardado {salida}: {n:,} registros, {npc} PCs, orden (lat, lon)")
