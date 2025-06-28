import pandas as pd
import numpy as np

# Configurar pandas para mantener alta precisión solo para coordenadas
pd.set_option('display.float_format', '{:.2f}'.format)
pd.set_option('display.precision', 2)

print("Limpieza de Datos")

# Cargar archivo CSV original
print("Cargar Archivo:")
df = pd.read_csv('2Database/1processed_data_complete.csv')
print(f"Forma del dataset original: {df.shape}") #Tamano del dataframe (filas y columnas)
print(f"Registros originales: {df.shape[0]:,}")

# Verificar columnas existentes
print(f"\nColumnas disponibles: {list(df.columns)}")

# Lista de atributos para PCA (todos los numéricos excepto total_amount)
atributos_para_pca = [
    'passenger_count', 'trip_distance', 'payment_type',
    'fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge'
]

print(f"\nAtributos que se usarán para PCA: {atributos_para_pca}")

# Verificar que no hay valores que no son numeros en los atributos de PCA
print(f"Valores No Numeros en atributos PCA: {df[atributos_para_pca].isnull().sum().sum()}")

# Verificar valores negativos en todos los atributos de tarifa
atributos_tarifa = ['fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge']
print("\nValores negativos por atributo:")
for columna in atributos_tarifa:
    valores_negativos = (df[columna] < 0).sum()
    min_valor = df[columna].min()
    print(f"{columna}: {valores_negativos} valores negativos, min={min_valor:.2f}")

# Corregir valores negativos en todos los atributos de tarifa
print("\nCorrigiendo valores negativos...")
for columna in atributos_tarifa:
    min_valor = df[columna].min()
    if min_valor < 0:
        print(f"Corrigiendo valores negativos en {columna}: min={min_valor:.2f}")
        df[columna] = df[columna] + abs(min_valor)
        # Redondear a 2 decimales para valores monetarios
        df[columna] = df[columna].round(2)
        print(f"  Nuevo min en {columna}: {df[columna].min():.2f}")

# Recalculando total_amount con redondeo a 2 decimales
print("\nRecalculando total_amount...")
df['total_amount'] = (
    df['fare_amount'] + df['extra'] + df['mta_tax'] +
    df['tip_amount'] + df['tolls_amount'] + df['improvement_surcharge']
).round(2)

# Verificar que no se perdieron registros después de la limpieza
print(f"\nRegistros después de la limpieza: {df.shape[0]:,}")
print(f"¿Se mantuvieron todos los registros? {df.shape[0] == df.shape[0]}")

# Verificar que no hay valores NaN después de la limpieza
print(f"Valores No Numeros después de la limpieza: {df[atributos_para_pca].isnull().sum().sum()}")

# Guardar archivo limpio
print("\nGuardando archivo limpio...")
df.to_csv('2Database/2processed_data_complete_limpio.csv', index=False)
print("Archivo guardado en 'Database/2processed_data_complete_limpio.csv'")

# Verificar el archivo guardado
print("\nVerificando archivo guardado...")
df_verificacion = pd.read_csv('2Database/2processed_data_complete_limpio.csv')
print(f"Registros en archivo guardado: {df_verificacion.shape[0]:,}")
print(f"¿Coinciden los registros? {df.shape[0] == df_verificacion.shape[0]}")


# Mostrar estadísticas finales
print(f"\n=== RESUMEN DE LIMPIEZA ===")
print(f"Registros originales: {df.shape[0]:,}")
print(f"Registros después de limpieza: {df.shape[0]:,}")
print(f"Registros en archivo guardado: {df_verificacion.shape[0]:,}")
print(f"Columnas en archivo final: {list(df.columns)}")
print("\n¡Limpieza completada!") 