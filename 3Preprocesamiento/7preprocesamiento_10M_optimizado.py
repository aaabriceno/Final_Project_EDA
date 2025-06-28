#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Preprocesamiento OPTIMIZADO para 10M puntos
- Usa K-means en lugar de DBSCAN para clustering espacial (mucho más rápido)
- Clustering atributivo por cluster espacial
- Generación de microclusters para R*-Tree
"""

import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler
from sklearn.decomposition import PCA
from sklearn.cluster import KMeans
from sklearn.metrics import silhouette_score
import matplotlib.pyplot as plt
from datetime import datetime
import gc
import warnings
warnings.filterwarnings('ignore')

class Preprocesador10MOptimizado:
    def __init__(self, archivo_entrada="2Database/3pca_data_set_complete.csv"):
        self.archivo_entrada = archivo_entrada
        self.datos = None
        self.datos_procesados = None
        self.scaler = StandardScaler()
        self.pca = None
        self.kmeans_espacial = None
        self.kmeans_atributivos = {}  # Un K-means por cluster espacial
        
    def cargar_datos(self):
        """Cargar datos de 10M puntos"""
        print(f"🔄 Cargando datos desde {self.archivo_entrada}...")
        start_time = datetime.now()
        
        # Cargar en chunks para manejar memoria
        chunk_size = 200000  # Chunks más grandes para mejor rendimiento
        chunks = []
        
        for chunk in pd.read_csv(self.archivo_entrada, chunksize=chunk_size):
            chunks.append(chunk)
            if len(chunks) % 5 == 0:
                print(f"  Chunks cargados: {len(chunks)}")
        
        self.datos = pd.concat(chunks, ignore_index=True)
        print(f"✅ Datos cargados: {len(self.datos):,} puntos")
        print(f"⏱️  Tiempo: {datetime.now() - start_time}")
        
    def limpiar_datos(self):
        """Limpieza básica de datos (asumiendo que ya tienes datos limpios)"""
        print("🧹 Verificando limpieza de datos...")
        start_time = datetime.now()
        
        # Verificar que no hay valores negativos (ya los procesaste)
        columnas_numericas = self.datos.select_dtypes(include=[np.number]).columns
        for col in columnas_numericas:
            if self.datos[col].min() < 0:
                print(f"  ⚠️  Columna {col} tiene valores negativos")
        
        # Eliminar duplicados si los hay
        antes = len(self.datos)
        self.datos = self.datos.drop_duplicates()
        print(f"  Duplicados eliminados: {antes - len(self.datos)}")
        
        # Eliminar filas con valores nulos
        antes = len(self.datos)
        self.datos = self.datos.dropna()
        print(f"  Filas con nulos eliminadas: {antes - len(self.datos)}")
        
        print(f"✅ Datos limpios: {len(self.datos):,} puntos")
        print(f"⏱️  Tiempo: {datetime.now() - start_time}")
        
    def aplicar_pca(self, n_components=8):
        """Aplicar PCA para reducción de dimensionalidad"""
        print(f"🔧 Aplicando PCA con {n_components} componentes...")
        start_time = datetime.now()
        
        # Seleccionar atributos numéricos (excluyendo id, latitud, longitud)
        columnas_atributos = [col for col in self.datos.columns 
                             if col not in ['id', 'latitud', 'longitud'] and 
                             self.datos[col].dtype in ['float64', 'int64']]
        
        print(f"  Atributos seleccionados: {len(columnas_atributos)}")
        
        # Preparar datos para PCA
        X_atributos = self.datos[columnas_atributos].values
        
        # Estandarizar
        X_scaled = self.scaler.fit_transform(X_atributos)
        
        # Aplicar PCA
        self.pca = PCA(n_components=n_components, random_state=42)
        X_pca = self.pca.fit_transform(X_scaled)
        
        # Crear DataFrame con resultados
        columnas_pca = [f'pca_{i+1}' for i in range(n_components)]
        df_pca = pd.DataFrame(X_pca, columns=columnas_pca)
        
        # Combinar con datos originales
        self.datos_procesados = pd.concat([
            self.datos[['id', 'latitud', 'longitud']].reset_index(drop=True),
            df_pca.reset_index(drop=True)
        ], axis=1)
        
        # Mostrar varianza explicada
        varianza_explicada = self.pca.explained_variance_ratio_
        print(f"  Varianza explicada: {varianza_explicada.sum():.4f}")
        
        print(f"✅ PCA completado: {len(self.datos_procesados):,} puntos")
        print(f"⏱️  Tiempo: {datetime.now() - start_time}")
        
        # Liberar memoria
        del X_atributos, X_scaled, X_pca, df_pca
        gc.collect()
        
    def clustering_espacial_kmeans(self, n_clusters_espaciales=1000):
        """Clustering espacial con K-means (MUCHO MÁS RÁPIDO que DBSCAN)"""
        print(f"🌍 Aplicando clustering espacial (K-means: {n_clusters_espaciales} clusters)...")
        start_time = datetime.now()
        
        # Preparar coordenadas
        coords = self.datos_procesados[['latitud', 'longitud']].values
        
        # Aplicar K-means espacial
        self.kmeans_espacial = KMeans(
            n_clusters=n_clusters_espaciales, 
            random_state=42, 
            n_init=5,  # Menos inicializaciones para velocidad
            max_iter=100  # Menos iteraciones para velocidad
        )
        clusters_espaciales = self.kmeans_espacial.fit_predict(coords)
        
        # Agregar clusters al DataFrame
        self.datos_procesados['cluster_espacial'] = clusters_espaciales
        
        # Estadísticas
        n_clusters_unicos = len(set(clusters_espaciales))
        print(f"  Clusters espaciales creados: {n_clusters_unicos}")
        
        # Mostrar distribución de clusters
        cluster_counts = pd.Series(clusters_espaciales).value_counts()
        print(f"  Tamaño promedio por cluster: {cluster_counts.mean():.1f}")
        print(f"  Tamaño mínimo: {cluster_counts.min()}")
        print(f"  Tamaño máximo: {cluster_counts.max()}")
        
        print(f"✅ Clustering espacial completado")
        print(f"⏱️  Tiempo: {datetime.now() - start_time}")
        
    def clustering_atributivo_por_cluster(self, n_subclusters_por_cluster=5):
        """Clustering atributivo por cada cluster espacial"""
        print(f"📊 Aplicando clustering atributivo por cluster espacial...")
        start_time = datetime.now()
        
        # Seleccionar componentes PCA
        columnas_pca = [col for col in self.datos_procesados.columns if col.startswith('pca_')]
        
        # Inicializar columna de subclusters
        self.datos_procesados['subcluster_atributivo'] = -1
        
        # Contador global para subclusters
        subcluster_id_global = 0
        
        # Procesar cada cluster espacial
        clusters_unicos = sorted(self.datos_procesados['cluster_espacial'].unique())
        
        for cluster_id in clusters_unicos:
            # Obtener puntos de este cluster espacial
            mask = self.datos_procesados['cluster_espacial'] == cluster_id
            puntos_cluster = self.datos_procesados[mask]
            
            if len(puntos_cluster) < n_subclusters_por_cluster * 2:
                # Si hay muy pocos puntos, asignar todos al mismo subcluster
                self.datos_procesados.loc[mask, 'subcluster_atributivo'] = subcluster_id_global
                subcluster_id_global += 1
                continue
            
            # Preparar atributos PCA para este cluster
            X_pca_cluster = puntos_cluster[columnas_pca].values
            
            # Determinar número de subclusters para este cluster
            n_subclusters = min(n_subclusters_por_cluster, len(puntos_cluster) // 2)
            
            # Aplicar K-means para atributos
            kmeans_cluster = KMeans(
                n_clusters=n_subclusters, 
                random_state=42, 
                n_init=3,
                max_iter=50
            )
            subclusters = kmeans_cluster.fit_predict(X_pca_cluster)
            
            # Asignar IDs globales a los subclusters
            for i, subcluster_local in enumerate(subclusters):
                self.datos_procesados.loc[puntos_cluster.index[i], 'subcluster_atributivo'] = subcluster_id_global + subcluster_local
            
            subcluster_id_global += n_subclusters
            
            # Guardar el modelo para este cluster
            self.kmeans_atributivos[cluster_id] = kmeans_cluster
            
            if cluster_id % 100 == 0:
                print(f"  Procesados {cluster_id}/{len(clusters_unicos)} clusters espaciales")
        
        print(f"✅ Clustering atributivo completado")
        print(f"  Total subclusters creados: {subcluster_id_global}")
        print(f"⏱️  Tiempo: {datetime.now() - start_time}")
        
    def generar_microclusters(self):
        """Generar información de microclusters para R*-Tree"""
        print("🔬 Generando información de microclusters...")
        start_time = datetime.now()
        
        # Agrupar por cluster espacial y subcluster atributivo
        microclusters_info = self.datos_procesados.groupby(['cluster_espacial', 'subcluster_atributivo']).agg({
            'id': 'count',
            'latitud': ['mean', 'std'],
            'longitud': ['mean', 'std']
        }).reset_index()
        
        # Renombrar columnas
        microclusters_info.columns = [
            'cluster_espacial', 'subcluster_atributivo', 'num_puntos',
            'lat_centro', 'lat_std', 'lon_centro', 'lon_std'
        ]
        
        # Calcular radio aproximado del microcluster
        microclusters_info['radio_aproximado'] = np.sqrt(
            microclusters_info['lat_std']**2 + microclusters_info['lon_std']**2
        )
        
        # Guardar información de microclusters
        microclusters_info.to_csv("2Database/microclusters_info.csv", index=False)
        
        print(f"✅ Información de microclusters generada")
        print(f"  Total microclusters: {len(microclusters_info)}")
        print(f"  Tamaño promedio: {microclusters_info['num_puntos'].mean():.1f} puntos")
        print(f"⏱️  Tiempo: {datetime.now() - start_time}")
        
        return microclusters_info
        
    def generar_archivo_final(self, archivo_salida="2Database/puntos_10M_procesados.csv"):
        """Generar archivo final para R*-Tree"""
        print(f"💾 Generando archivo final: {archivo_salida}")
        start_time = datetime.now()
        
        # Crear archivo final con formato optimizado para R*-Tree
        archivo_final = self.datos_procesados.copy()
        
        # Renombrar columnas para claridad
        archivo_final = archivo_final.rename(columns={
            'cluster_espacial': 'id_cluster_geografico',
            'subcluster_atributivo': 'id_subcluster_atributivo'
        })
        
        # Reorganizar columnas
        columnas_finales = ['id', 'latitud', 'longitud', 'id_cluster_geografico', 'id_subcluster_atributivo']
        columnas_pca = [col for col in archivo_final.columns if col.startswith('pca_')]
        columnas_finales.extend(columnas_pca)
        
        archivo_final = archivo_final[columnas_finales]
        
        # Guardar archivo
        archivo_final.to_csv(archivo_salida, index=False)
        
        print(f"✅ Archivo guardado: {archivo_salida}")
        print(f"📊 Tamaño: {len(archivo_final):,} puntos")
        print(f"📊 Columnas: {list(archivo_final.columns)}")
        print(f"⏱️  Tiempo: {datetime.now() - start_time}")
        
        # Mostrar estadísticas finales
        print("\n📈 ESTADÍSTICAS FINALES:")
        print(f"  Total puntos: {len(archivo_final):,}")
        print(f"  Clusters geográficos: {archivo_final['id_cluster_geografico'].nunique()}")
        print(f"  Subclusters atributivos: {archivo_final['id_subcluster_atributivo'].nunique()}")
        print(f"  Rango latitud: [{archivo_final['latitud'].min():.6f}, {archivo_final['latitud'].max():.6f}]")
        print(f"  Rango longitud: [{archivo_final['longitud'].min():.6f}, {archivo_final['longitud'].max():.6f}]")
        
        return archivo_final
        
    def generar_visualizaciones(self):
        """Generar visualizaciones del preprocesamiento"""
        print("📊 Generando visualizaciones...")
        
        # Crear directorio si no existe
        import os
        os.makedirs("3Preprocesamiento/Analisis_10M_Optimizado", exist_ok=True)
        
        # 1. Distribución de clusters espaciales
        plt.figure(figsize=(12, 8))
        cluster_counts = self.datos_procesados['cluster_espacial'].value_counts()
        plt.hist(cluster_counts.values, bins=50, alpha=0.7, edgecolor='black')
        plt.xlabel('Tamaño del Cluster Espacial')
        plt.ylabel('Frecuencia')
        plt.title('Distribución de Tamaños de Clusters Espaciales (K-means)')
        plt.yscale('log')
        plt.grid(True, alpha=0.3)
        plt.savefig("3Preprocesamiento/Analisis_10M_Optimizado/distribucion_clusters_espaciales.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        # 2. Distribución de subclusters atributivos
        plt.figure(figsize=(12, 8))
        subcluster_counts = self.datos_procesados['subcluster_atributivo'].value_counts()
        plt.hist(subcluster_counts.values, bins=50, alpha=0.7, edgecolor='black')
        plt.xlabel('Tamaño del Subcluster Atributivo')
        plt.ylabel('Frecuencia')
        plt.title('Distribución de Tamaños de Subclusters Atributivos')
        plt.yscale('log')
        plt.grid(True, alpha=0.3)
        plt.savefig("3Preprocesamiento/Analisis_10M_Optimizado/distribucion_subclusters_atributivos.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        # 3. Mapa de clusters espaciales (muestra)
        muestra = self.datos_procesados.sample(n=min(50000, len(self.datos_procesados)))
        plt.figure(figsize=(15, 10))
        scatter = plt.scatter(muestra['longitud'], muestra['latitud'], 
                            c=muestra['cluster_espacial'], cmap='tab20', alpha=0.6, s=0.5)
        plt.colorbar(scatter)
        plt.xlabel('Longitud')
        plt.ylabel('Latitud')
        plt.title('Mapa de Clusters Espaciales (Muestra de 50k puntos)')
        plt.grid(True, alpha=0.3)
        plt.savefig("3Preprocesamiento/Analisis_10M_Optimizado/mapa_clusters_espaciales.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        print("✅ Visualizaciones guardadas en 3Preprocesamiento/Analisis_10M_Optimizado/")
        
    def ejecutar_preprocesamiento_optimizado(self):
        """Ejecutar todo el pipeline de preprocesamiento optimizado"""
        print("🚀 INICIANDO PREPROCESAMIENTO OPTIMIZADO PARA 10M PUNTOS")
        print("=" * 70)
        print("💡 ESTRATEGIA: K-means espacial + Clustering atributivo por cluster")
        print("=" * 70)
        
        try:
            self.cargar_datos()
            self.limpiar_datos()
            self.aplicar_pca(n_components=8)
            self.clustering_espacial_kmeans(n_clusters_espaciales=1000)
            self.clustering_atributivo_por_cluster(n_subclusters_por_cluster=5)
            self.generar_microclusters()
            self.generar_archivo_final()
            self.generar_visualizaciones()
            
            print("\n🎉 PREPROCESAMIENTO OPTIMIZADO COMPLETADO EXITOSAMENTE!")
            print("=" * 70)
            print("📋 PRÓXIMOS PASOS:")
            print("1. Ejecutar este script para procesar 10M puntos")
            print("2. Implementar microclusters en R*-Tree")
            print("3. Implementar funciones de consulta")
            print("=" * 70)
            
        except Exception as e:
            print(f"❌ Error durante el preprocesamiento: {str(e)}")
            raise

if __name__ == "__main__":
    # Configurar para mejor rendimiento
    import warnings
    warnings.filterwarnings('ignore')
    
    # Crear preprocesador y ejecutar
    preprocesador = Preprocesador10MOptimizado()
    preprocesador.ejecutar_preprocesamiento_optimizado() 