import pandas as pd
import numpy as np

# Configurar pandas para mantener alta precisión
pd.set_option('display.float_format', '{:.13f}'.format)
pd.set_option('display.precision', 13)

#Analizamos algun tipo de anomalia en los datos, ya sea valores negativ, etc

print("ANALIZANDO DATOS PARA IDENTIFICAR VALORES EXTRAÑOS")
# Cargar el archivo de datos
try:
    df = pd.read_csv('2Database/1processed_data_complete.csv')
    print(f"Archivo cargado exitosamente: {df.shape[0]} filas, {df.shape[1]} columnas")
except Exception as e:
    print(f"Error cargando archivo: {e}")
    exit(1)

# Mostrar información general
print(f"\nCOLUMNAS DISPONIBLES:")
for i, col in enumerate(df.columns):
    print(f"  {i+1:2d}. {col}")

# Analizar cada columna numérica
print(f"\nANÁLISIS DETALLADO POR COLUMNA:")

atributos_tarifa = ['fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge']
coordenadas = ['pickup_latitude', 'pickup_longitude']

for columna in df.columns:
    if df[columna].dtype in ['int64', 'float64']:
        print(f"\n{columna.upper()}:")
        print(f"   Tipo: {df[columna].dtype}")
        
        # Determinar formato según el tipo de dato
        if columna in coordenadas:
            # Coordenadas geográficas: 13 decimales
            formato = '.13f'
        else:
            # Atributos monetarios: 2 decimales
            formato = '.2f'
        
        print(f"   Min: {df[columna].min():{formato}}")
        print(f"   Max: {df[columna].max():{formato}}")
        print(f"   Media: {df[columna].mean():{formato}}")
        print(f"   Mediana: {df[columna].median():{formato}}")
        print(f"   Desv. Est.: {df[columna].std():{formato}}")
        
        # Contar valores negativos
        negativos = (df[columna] < 0).sum()
        if negativos > 0:
            print(f"VALORES NEGATIVOS: {negativos} ({negativos/len(df)*100:.2f}%)")
            
            # Mostrar algunos ejemplos de valores negativos
            valores_neg = df[df[columna] < 0][columna]
            print(f"   Ejemplos de valores negativos:")
            for i, valor in enumerate(valores_neg.head(5)):
                print(f"     {valor:{formato}}")
            if len(valores_neg) > 5:
                print(f"     ... y {len(valores_neg)-5} más")
        
        # Contar valores cero
        ceros = (df[columna] == 0).sum()
        print(f"   Ceros: {ceros} ({ceros/len(df)*100:.2f}%)")
        
        # Contar valores nulos
        nulos = df[columna].isnull().sum()
        if nulos > 0:
            print(f"VALORES NULOS: {nulos} ({nulos/len(df)*100:.2f}%)")

# Analizar valores extremos
print(f"\nANÁLISIS DE VALORES EXTREMOS:")

for columna in atributos_tarifa:
    if columna in df.columns:
        print(f"\n {columna.upper()}:")
        
        # Valores más negativos
        valores_neg = df[df[columna] < 0][columna].sort_values()
        if len(valores_neg) > 0:
            print(f"   Valor más negativo:")
            for valor in valores_neg.head(1):
                print(f"     {valor:.2f}")
        
        # Valores más positivos
        valores_pos = df[df[columna] > 0][columna].sort_values(ascending=False)
        if len(valores_pos) > 0:
            print(f"   Valor más positivo:")
            for valor in valores_pos.head(1):
                print(f"     {valor:.2f}")

# Verificar si total_amount coincide con la suma
if 'total_amount' in df.columns:
    print(f"\nVERIFICACIÓN DE TOTAL_AMOUNT:")
    
    # Calcular suma manual
    suma_manual = (
        df['fare_amount'] + df['extra'] + df['mta_tax'] +
        df['tip_amount'] + df['tolls_amount'] + df['improvement_surcharge']
    )
    
    # Comparar con total_amount
    diferencia = abs(df['total_amount'] - suma_manual)
    max_diferencia = diferencia.max()
    media_diferencia = diferencia.mean()
    
    print(f"   Diferencia máxima: {max_diferencia:.2f}")
    print(f"   Diferencia media: {media_diferencia:.2f}")
    
    if max_diferencia > 0.01:  # Tolerancia de 1 centavo
        print(f"ADVERTENCIA: Hay diferencias significativas entre total_amount y la suma")
        
        # Mostrar ejemplos donde hay diferencias
        indices_diferentes = diferencia[diferencia > 0.01].index
        print(f"   Filas con diferencias > 0.01: {len(indices_diferentes)}")
        
        for i, idx in enumerate(indices_diferentes[:3]):
            print(f"\n   Ejemplo {i+1} (fila {idx}):")
            print(f"     fare_amount: {df.loc[idx, 'fare_amount']:.2f}")
            print(f"     extra: {df.loc[idx, 'extra']:.2f}")
            print(f"     mta_tax: {df.loc[idx, 'mta_tax']:.2f}")
            print(f"     tip_amount: {df.loc[idx, 'tip_amount']:.2f}")
            print(f"     tolls_amount: {df.loc[idx, 'tolls_amount']:.2f}")
            print(f"     improvement_surcharge: {df.loc[idx, 'improvement_surcharge']:.2f}")
            print(f"     Suma manual: {suma_manual[idx]:.2f}")
            print(f"     total_amount: {df.loc[idx, 'total_amount']:.2f}")
            print(f"     Diferencia: {diferencia[idx]:.2f}")

# Mostrar algunas filas problemáticas
print(f"\nMUESTRAS DE DATOS PROBLEMÁTICOS:")

# Filas con valores muy negativos
for columna in atributos_tarifa:
    if columna in df.columns:
        filas_negativas = df[df[columna] < -100]  # Valores muy negativos
        if len(filas_negativas) > 0:
            print(f"\nFilas con {columna} < -100:")
            print(f"   Encontradas: {len(filas_negativas)} filas")
            
            for i, (idx, row) in enumerate(filas_negativas.head(2).iterrows()):
                print(f"\n   Fila {idx}:")
                for col in ['fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge', 'total_amount']:
                    if col in df.columns:
                        print(f"     {col}: {row[col]:.2f}")

print(f"\nANÁLISIS COMPLETADO")