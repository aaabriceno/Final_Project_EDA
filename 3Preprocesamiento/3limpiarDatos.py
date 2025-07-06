import pandas as pd
import numpy as np

# Configurar pandas para mantener alta precisión solo para coordenadas
pd.set_option('display.float_format', '{:.2f}'.format)
pd.set_option('display.precision', 2)

print("Limpieza de Datos")

# Cargar archivo CSV original
print("Cargar Archivo:")
df = pd.read_csv('2Database/data500k/puntos500k.csv')
print(f"Forma del dataset original: {df.shape}") #Tamano del dataframe (filas y columnas)
print(f"Registros originales: {df.shape[0]:,}")

# Verificar columnas existentes
print(f"\nColumnas disponibles: {list(df.columns)}")

# PASO 1: CORREGIR DATOS NEGATIVOS EN ATRIBUTOS
print(f"\n=== PASO 1: CORRECCIÓN DE DATOS NEGATIVOS ===")

# Lista de atributos para PCA (todos los numéricos excepto total_amount)
atributos_para_pca = [
    'passenger_count', 'trip_distance', 'payment_type',
    'fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge'
]

print(f"Atributos que se usarán para PCA: {atributos_para_pca}")

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

print("Datos negativos corregidos")



# PASO 2: FILTRADO POR RANGO NYC 
print(f"\n=== PASO 2: FILTRADO POR RANGO NYC  ===")

# Definir límites amplios para toda la región metropolitana de NYC
NYC_METRO = {
    'min_lat': 40.5774,    # Límite sur amplio (incluye NJ, Long Island)
    'max_lat': 40.9176,    # Límite norte amplio (incluye Connecticut, Upstate NY)
    'min_long': -74.15,  # Límite oeste amplio (incluye Pennsylvania)
    'max_long': -73.7004   # Límite este amplio (incluye Long Island, Connecticut)
}

print(f"Rangos NYC definidos:")
print(f"Latitud: {NYC_METRO['min_lat']}° - {NYC_METRO['max_lat']}°")
print(f"Longitud: {NYC_METRO['min_long']}° - {NYC_METRO['max_long']}°")

# ANÁLISIS DE OUTLIERS GEOGRÁFICOS
print(f"\nAnálisis de outliers geográficos:")

# 1. Detectar errores claros (0.0)
errores_cero = ((df['pickup_latitude'] == 0) & (df['pickup_longitude'] == 0)).sum()
print(f"Errores claros (0,0): {errores_cero:,}")

# 2. Detectar coordenadas inválidas
invalido_lat = ((df['pickup_latitude'] < -90) | (df['pickup_latitude'] > 90)).sum()
invalido_long = ((df['pickup_longitude'] < -180) | (df['pickup_longitude'] > 180)).sum()
print(f"Coordenadas lat inválidas: {invalido_lat:,}")
print(f"Coordenadas long inválidas: {invalido_long:,}")

# 3. Detectar puntos fuera de NYC 
fuera_metro = ((df['pickup_latitude'] < NYC_METRO['min_lat']) | 
               (df['pickup_latitude'] > NYC_METRO['max_lat']) |
               (df['pickup_longitude'] < NYC_METRO['min_long']) | 
               (df['pickup_longitude'] > NYC_METRO['max_long'])).sum()

print(f"Puntos fuera de NYC : {fuera_metro:,}")

# 4. Total de outliers geográficos
total_outliers_geo = fuera_metro
print(f"Total outliers geográficos: {total_outliers_geo:,}")

# Crear máscara para datos válidos geográficamente
mask_geo_valido = (
    # No errores claros (0,0)
    ~((df['pickup_latitude'] == 0) & (df['pickup_longitude'] == 0)) &
    # Coordenadas válidas
    (df['pickup_latitude'] >= -90) & (df['pickup_latitude'] <= 90) &
    (df['pickup_longitude'] >= -180) & (df['pickup_longitude'] <= 180) &
    # Dentro de NYC 
    (df['pickup_latitude'] >= NYC_METRO['min_lat']) & (df['pickup_latitude'] <= NYC_METRO['max_lat']) &
    (df['pickup_longitude'] >= NYC_METRO['min_long']) & (df['pickup_longitude'] <= NYC_METRO['max_long'])
)

# Separar datos con y sin outliers
df_con_outliers = df.copy()  # Datos originales con outliers
df_sin_outliers = df[mask_geo_valido].copy()  # Datos sin outliers geográficos

print(f"\nRegistros con outliers: {len(df_con_outliers):,}")
print(f"Registros sin outliers: {len(df_sin_outliers):,}")
print(f"Outliers eliminados: {len(df_con_outliers) - len(df_sin_outliers):,}")
print(f"Porcentaje de datos conservados: {len(df_sin_outliers)/len(df_con_outliers)*100:.1f}%")

# Verificar que no hay valores NaN después de la limpieza
print(f"Valores No Numeros después de la limpieza: {df_sin_outliers[atributos_para_pca].isnull().sum().sum()}")

# PASO 3: GUARDAR ARCHIVOS CSV
print(f"\n=== PASO 3: GUARDAR ARCHIVOS CSV ===")

# Guardar archivo SIN outliers (solo NYC )
print("Guardando archivo SIN outliers (solo NYC )...")
df_sin_outliers.to_csv('2Database/data500k/2processed_data_500k_limpio.csv', index=False)
print("Archivo guardado en '2Database/data500k/2processed_data_500k_limpio.csv'")

# Verificar los archivos guardados
print(f"\nVerificando archivos guardados...")
df_sin_outliers_verif = pd.read_csv('2Database/data500k/2processed_data_500k_limpio.csv')


print(f"Registros en archivo SIN outliers: {df_sin_outliers_verif.shape[0]:,}")

# Mostrar estadísticas finales
print(f"\n=== RESUMEN DE LIMPIEZA ===")
print(f"Registros originales: {df.shape[0]:,}")
print(f"Registros con outliers: {len(df_con_outliers):,}")
print(f"Registros sin outliers (NYC ): {len(df_sin_outliers):,}")
print(f"Outliers eliminados: {len(df_con_outliers) - len(df_sin_outliers):,}")
print(f"Porcentaje de datos conservados: {len(df_sin_outliers)/len(df_con_outliers)*100:.1f}%")
print(f"Columnas en archivo final: {list(df_sin_outliers.columns)}")
print("\n¡Limpieza completada!") 