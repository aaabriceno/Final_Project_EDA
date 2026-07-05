import pandas as pd
import numpy as np
import struct
import os

print("=== CONVERSIÓN DE DATOS CLUSTERIZADOS PARA R*-TREE ===")

# Cargar datos clusterizados
print("\nCargando datos clusterizados...")
try:
    df_clustered = pd.read_csv('2Database/6clusters_mejorados_completo.csv')
    print(f"Dataset clusterizado cargado: {len(df_clustered):,} puntos")
except FileNotFoundError:
    print("ERROR: Archivo de clusters no encontrado. Ejecuta primero 5clusterizacion_mejorada.py")
    exit(1)

# Verificar columnas necesarias
required_cols = ['pickup_latitude', 'pickup_longitude', 'cluster_espacial', 'sub_cluster', 'cluster_jerarquico']
missing_cols = [col for col in required_cols if col not in df_clustered.columns]
if missing_cols:
    print(f"ERROR: Faltan columnas: {missing_cols}")
    exit(1)

# Identificar columnas de atributos (excluir coordenadas y clusters)
exclude_cols = ['pickup_latitude', 'pickup_longitude', 'cluster_espacial', 'sub_cluster', 'cluster_jerarquico']
attr_cols = [col for col in df_clustered.columns if col not in exclude_cols]

print(f"Columnas de atributos identificadas: {len(attr_cols)}")
print(f"Atributos: {attr_cols[:5]}...")  # Mostrar primeros 5

# Preparar datos para conversión
print(f"\nPreparando datos para conversión...")

# Filtrar solo datos válidos (excluir outliers con cluster_espacial = -1)
df_validos = df_clustered[df_clustered['cluster_espacial'] != -1].copy()
df_outliers = df_clustered[df_clustered['cluster_espacial'] == -1].copy()

print(f"Datos válidos: {len(df_validos):,} puntos")
print(f"Outliers: {len(df_outliers):,} puntos")

# Función para convertir a formato binario
def convertir_a_binario(df, output_file):
    """
    Convierte DataFrame a formato binario para R*-Tree
    Formato: id (int), lat (double), lon (double), n_attrs (int), attrs (double[]), cluster_geo (int), subcluster (int)
    """
    print(f"\nConvirtiendo {len(df):,} puntos a formato binario...")
    
    with open(output_file, 'wb') as f:
        for idx, row in df.iterrows():
            # ID del punto
            punto_id = int(idx)
            
            # Coordenadas
            lat = float(row['pickup_latitude'])
            lon = float(row['pickup_longitude'])
            
            # Atributos
            atributos = []
            for col in attr_cols:
                if pd.notna(row[col]):
                    atributos.append(float(row[col]))
                else:
                    atributos.append(0.0)  # Valor por defecto para nulos
            
            n_atributos = len(atributos)
            
            # IDs de clusters
            cluster_geo = int(row['cluster_espacial'])
            subcluster = int(row['sub_cluster'])
            
            # Escribir en formato binario
            # ID (4 bytes), lat (8 bytes), lon (8 bytes), n_attrs (4 bytes)
            f.write(struct.pack('i', punto_id))
            f.write(struct.pack('d', lat))
            f.write(struct.pack('d', lon))
            f.write(struct.pack('i', n_atributos))
            
            # Atributos (n_attrs * 8 bytes)
            for attr in atributos:
                f.write(struct.pack('d', attr))
            
            # IDs de clusters (4 bytes cada uno)
            f.write(struct.pack('i', cluster_geo))
            f.write(struct.pack('i', subcluster))
    
    print(f"✅ Archivo binario creado: {output_file}")
    print(f"Tamaño del archivo: {os.path.getsize(output_file):,} bytes")

# Función para crear archivo de metadatos
def crear_metadatos(df, output_file):
    """
    Crea archivo de metadatos con información sobre los clusters
    """
    print(f"\nCreando archivo de metadatos...")
    
    # Estadísticas de clusters
    cluster_stats = df.groupby('cluster_espacial').agg({
        'sub_cluster': 'nunique',
        'pickup_latitude': ['count', 'min', 'max'],
        'pickup_longitude': ['min', 'max']
    }).round(6)
    
    # Aplanar columnas multi-nivel
    cluster_stats.columns = ['_'.join(col).strip() for col in cluster_stats.columns]
    
    # Agregar información de subclusters
    subcluster_stats = df.groupby(['cluster_espacial', 'sub_cluster']).size().reset_index(name='count')
    
    # Guardar metadatos
    with open(output_file, 'w') as f:
        f.write("# METADATOS DE CLUSTERS PARA R*-TREE\n")
        f.write(f"# Total puntos: {len(df):,}\n")
        f.write(f"# Clusters espaciales: {df['cluster_espacial'].nunique()}\n")
        f.write(f"# Subclusters únicos: {df['sub_cluster'].nunique()}\n")
        f.write(f"# Atributos por punto: {len(attr_cols)}\n")
        f.write(f"# Atributos: {', '.join(attr_cols)}\n\n")
        
        f.write("# ESTADÍSTICAS DE CLUSTERS ESPACIALES\n")
        cluster_stats.to_string(f)
        f.write("\n\n# ESTADÍSTICAS DE SUBCLUSTERS\n")
        subcluster_stats.to_string(f)
    
    print(f"✅ Metadatos guardados: {output_file}")

# Convertir datos válidos
if len(df_validos) > 0:
    output_binario = '2Database/7datos_clusterizados_rtree.bin'
    output_metadatos = '2Database/7metadatos_clusters.txt'
    
    convertir_a_binario(df_validos, output_binario)
    crear_metadatos(df_validos, output_metadatos)
    
    # Crear archivo de outliers separado
    if len(df_outliers) > 0:
        output_outliers = '2Database/7outliers_clusterizados_rtree.bin'
        convertir_a_binario(df_outliers, output_outliers)
        print(f"✅ Outliers guardados en archivo separado: {output_outliers}")

# Crear archivo de configuración para C++
def crear_config_cpp():
    """
    Crea archivo de configuración para el programa C++
    """
    config_file = '2Database/7config_rtree.txt'
    
    with open(config_file, 'w') as f:
        f.write("# CONFIGURACIÓN PARA R*-TREE\n")
        f.write(f"ARCHIVO_DATOS=7datos_clusterizados_rtree.bin\n")
        f.write(f"ARCHIVO_OUTLIERS=7outliers_clusterizados_rtree.bin\n")
        f.write(f"TOTAL_PUNTOS={len(df_validos):,}\n")
        f.write(f"TOTAL_OUTLIERS={len(df_outliers):,}\n")
        f.write(f"CLUSTERS_ESPACIALES={df_validos['cluster_espacial'].nunique()}\n")
        f.write(f"SUBCLUSTERS_UNICOS={df_validos['sub_cluster'].nunique()}\n")
        f.write(f"ATRIBUTOS_POR_PUNTO={len(attr_cols)}\n")
        f.write(f"ATRIBUTOS={','.join(attr_cols)}\n")
        
        # Información de rangos
        f.write(f"LAT_MIN={df_validos['pickup_latitude'].min():.6f}\n")
        f.write(f"LAT_MAX={df_validos['pickup_latitude'].max():.6f}\n")
        f.write(f"LON_MIN={df_validos['pickup_longitude'].min():.6f}\n")
        f.write(f"LON_MAX={df_validos['pickup_longitude'].max():.6f}\n")
    
    print(f"✅ Configuración C++ guardada: {config_file}")

crear_config_cpp()

# Mostrar resumen final
print(f"\n=== RESUMEN DE CONVERSIÓN ===")
print(f"Dataset original: {len(df_clustered):,} puntos")
print(f"Datos válidos convertidos: {len(df_validos):,} puntos")
print(f"Outliers convertidos: {len(df_outliers):,} puntos")
print(f"Clusters espaciales: {df_validos['cluster_espacial'].nunique()}")
print(f"Subclusters únicos: {df_validos['sub_cluster'].nunique()}")
print(f"Atributos por punto: {len(attr_cols)}")

print(f"\nArchivos generados:")
print(f"  - 7datos_clusterizados_rtree.bin (datos válidos)")
if len(df_outliers) > 0:
    print(f"  - 7outliers_clusterizados_rtree.bin (outliers)")
print(f"  - 7metadatos_clusters.txt (estadísticas)")
print(f"  - 7config_rtree.txt (configuración C++)")

print(f"\n✅ Conversión completada! Los datos están listos para el R*-Tree.")
print(f"📝 Formato binario: id(int) + lat(double) + lon(double) + n_attrs(int) + attrs(double[]) + cluster_geo(int) + subcluster(int)") 