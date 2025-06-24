import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def cargar_y_limpiar_datos(ruta_archivo):
    """Carga y limpia los datos del archivo CSV"""
    try:
        df = pd.read_csv(ruta_archivo)
    except FileNotFoundError:
        print(f"Error: No se pudo encontrar el archivo {ruta_archivo}")
        return None
    
    print(f"Forma del dataset original: {df.shape}")
    
    # Lista de atributos para PCA
    atributos_para_pca = [
        'passenger_count', 'trip_distance', 'payment_type',
        'fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge'
    ]
    
    # Verificar que las columnas existen
    columnas_faltantes = set(atributos_para_pca) - set(df.columns)
    if columnas_faltantes:
        print(f"Error: Columnas faltantes: {columnas_faltantes}")
        return None, None
    
    # Mejor manejo de valores negativos: eliminar filas con valores negativos en tarifas
    atributos_tarifa = ['fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge']
    filas_iniciales = len(df)
    
    # Eliminar filas con valores negativos en lugar de corregirlos artificialmente
    mask_positivos = (df[atributos_tarifa] >= 0).all(axis=1)
    df = df[mask_positivos].copy()
    
    filas_eliminadas = filas_iniciales - len(df)
    if filas_eliminadas > 0:
        print(f"Se eliminaron {filas_eliminadas} filas con valores negativos en tarifas")
    
    # Recalcular total_amount
    df['total_amount'] = (
        df['fare_amount'] + df['extra'] + df['mta_tax'] +
        df['tip_amount'] + df['tolls_amount'] + df['improvement_surcharge']
    )
    
    return df, atributos_para_pca

def realizar_pca(X, umbral_varianza=0.95):
    """Realiza PCA en los datos estandarizados"""
    n_muestras, n_atributos = X.shape
    
    # Calcular media y desviación estándar
    media_vector = np.mean(X, axis=0)
    std_vector = np.std(X, axis=0, ddof=1)
    
    # Estandarizar los datos
    X_estandarizado = (X - media_vector) / std_vector
    
    # Verificar estandarización
    print(f"Media de datos estandarizados (debe ser ~0): {np.mean(X_estandarizado, axis=0)}")
    print(f"Std de datos estandarizados (debe ser ~1): {np.std(X_estandarizado, axis=0, ddof=1)}")
    
    # Calcular matriz de covarianza (más eficiente)
    matriz_de_covarianza = np.cov(X_estandarizado, rowvar=False)
    
    # Verificar simetría
    print(f"Matriz de covarianza es simétrica: {np.allclose(matriz_de_covarianza, matriz_de_covarianza.T)}")
    
    # Calcular autovalores y autovectores
    autovalores, autovectores = np.linalg.eigh(matriz_de_covarianza)
    
    # Ordenar de mayor a menor
    idx_sorted = np.argsort(autovalores)[::-1]
    autovalores_ordenados = autovalores[idx_sorted]
    autovectores_ordenados = autovectores[:, idx_sorted]
    
    # Calcular varianza explicada
    total_varianza = np.sum(autovalores_ordenados)
    ratio_varianza_explicada = autovalores_ordenados / total_varianza
    varianza_acumulada = np.cumsum(ratio_varianza_explicada)
    
    # Seleccionar k componentes
    k = np.searchsorted(varianza_acumulada, umbral_varianza) + 1
    k = min(k, n_atributos)
    
    print(f"\nSeleccionando {k} componentes para explicar {umbral_varianza*100:.1f}% de la varianza")
    print(f"Varianza real explicada: {varianza_acumulada[k-1]*100:.2f}%")
    
    # Matriz de proyección
    W = autovectores_ordenados[:, :k]
    
    # Proyectar datos
    X_pca = np.dot(X_estandarizado, W)
    
    return X_pca, ratio_varianza_explicada, varianza_acumulada, k

def main():
    # Cargar y limpiar datos
    df, atributos_para_pca = cargar_y_limpiar_datos('Database/processed_data_complete.csv')
    if df is None:
        return
    
    print(f"Atributos para PCA: {atributos_para_pca}")
    
    # Seleccionar datos para PCA
    X = df[atributos_para_pca].values
    
    # Realizar PCA
    X_pca, ratio_varianza_explicada, varianza_acumulada, k = realizar_pca(X)
    
    # Obtener coordenadas
    latlong = df[['pickup_latitude', 'pickup_longitude']].values
    
    # Combinar resultados
    X_final = np.hstack((latlong, X_pca))
    
    # Crear DataFrame final
    columnas_pca = [f'PC{i+1}' for i in range(k)]
    df_final = pd.DataFrame(X_final, columns=['lat', 'long'] + columnas_pca)
    
    # Guardar resultados
    df_final.to_csv('Database/pca_data_set_complete_mejorado.csv', index=False)
    df.to_csv('Database/processed_data_complete_limpio_mejorado.csv', index=False)
    
    # Mostrar resultados con formato controlado
    print(f"\nForma final: {X_final.shape}")
    with np.printoptions(precision=6, suppress=True):
        print("Primeras 3 filas:")
        print(X_final[:3])
    
    # Graficar
    plt.figure(figsize=(10, 6))
    n_atributos = len(ratio_varianza_explicada)
    plt.plot(range(1, n_atributos + 1), ratio_varianza_explicada * 100, 
             marker='o', label='Varianza individual')
    plt.plot(range(1, n_atributos + 1), varianza_acumulada * 100, 
             marker='s', label='Varianza acumulada')
    plt.axvline(x=k, color='r', linestyle='--', label=f'k={k} componentes')
    plt.axhline(y=95, color='g', linestyle=':', label='95% varianza')
    plt.xlabel('Número de componentes')
    plt.ylabel('Varianza explicada (%)')
    plt.title('Análisis de Componentes Principales - Varianza Explicada')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.savefig('Preprocesamiento/varianza_explicada_mejorado.png', dpi=300, bbox_inches='tight')
    plt.show()
    
    print(f"\n=== RESUMEN ===")
    print(f"Dataset procesado: {df.shape[0]} muestras")
    print(f"Atributos originales: {len(atributos_para_pca)}")
    print(f"Componentes PCA: {k}")
    print(f"Varianza explicada: {varianza_acumulada[k-1]*100:.2f}%")
    print(f"Reducción dimensional: {len(atributos_para_pca)} → {k}")

if __name__ == "__main__":
    main()