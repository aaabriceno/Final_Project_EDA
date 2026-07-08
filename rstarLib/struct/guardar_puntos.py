#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Script para guardar una cantidad específica de puntos desde el archivo CSV
de clusterización geográfica y atributiva.
Optimizado para archivos grandes sin cargar todo en memoria.
"""

import pandas as pd
import os
import sys

# CONFIGURACIÓN: Cambia este valor para la cantidad de puntos que quieres guardar
CANTIDAD_PUNTOS = 2000000  # Cambia este número según necesites

def contar_lineas_archivo(archivo):
    """Cuenta las líneas del archivo de forma eficiente."""
    try:
        with open(archivo, 'r', encoding='utf-8') as f:
            # Contar líneas (menos 1 por el header)
            lineas = sum(1 for _ in f) - 1
        return lineas
    except Exception as e:
        print(f"Error contando líneas: {e}")
        return 0

def guardar_puntos():
    """
    Lee el archivo CSV en chunks y guarda la cantidad especificada de puntos.
    """
    
    # Obtener la ruta del directorio actual donde está el script
    directorio_actual = os.path.dirname(os.path.abspath(__file__))
    
    archivo_entrada = os.path.join(directorio_actual, '5clusterizacion_geografica_attr.csv')
    archivo_salida = os.path.join(directorio_actual, 'puntos.csv')
    
    # Mostrar información de debug
    print(f"Directorio actual del script: {directorio_actual}")
    print(f"Buscando archivo en: {archivo_entrada}")
    
    # Verificar que el archivo de entrada existe
    if not os.path.exists(archivo_entrada):
        print(f"Error: El archivo {archivo_entrada} no existe.")
        print(f"Archivos disponibles en el directorio:")
        for archivo in os.listdir(directorio_actual):
            if archivo.endswith('.csv'):
                print(f"  - {archivo}")
        return False
    
    # Mostrar tamaño del archivo
    tamaño_archivo = os.path.getsize(archivo_entrada)
    tamaño_mb = tamaño_archivo / (1024 * 1024)
    print(f"Tamaño del archivo: {tamaño_mb:.1f} MB")
    
    try:
        print("Contando líneas del archivo (esto puede tomar un momento)...")
        total_puntos_archivo = contar_lineas_archivo(archivo_entrada)
        print(f"Total de puntos en el archivo: {total_puntos_archivo:,}")
        print(f"Puntos solicitados: {CANTIDAD_PUNTOS:,}")
        
        # Verificar que la cantidad solicitada no exceda el total
        if CANTIDAD_PUNTOS > total_puntos_archivo:
            print(f"ADVERTENCIA: Se solicitan {CANTIDAD_PUNTOS:,} puntos pero solo hay {total_puntos_archivo:,} disponibles.")
            print(f"Se guardarán todos los {total_puntos_archivo:,} puntos disponibles.")
            cantidad_a_guardar = total_puntos_archivo
        else:
            cantidad_a_guardar = CANTIDAD_PUNTOS
            print(f"Se guardarán {cantidad_a_guardar:,} puntos como se solicitó.")
        
        print(f"Leyendo archivo en chunks para optimizar memoria...")
        
        # Leer el archivo en chunks para evitar problemas de memoria
        chunk_size = CANTIDAD_PUNTOS  # 100k líneas por chunk
        puntos_guardados = 0
        primer_chunk = True
        
        # Procesar archivo en chunks
        for chunk in pd.read_csv(archivo_entrada, chunksize=chunk_size):
            if primer_chunk:
                print(f"Columnas detectadas: {list(chunk.columns)}")
                primer_chunk = False
            
            # Calcular cuántos puntos tomar de este chunk
            puntos_restantes = cantidad_a_guardar - puntos_guardados
            if puntos_restantes <= 0:
                break
                
            puntos_en_chunk = min(len(chunk), puntos_restantes)
            chunk_a_guardar = chunk.head(puntos_en_chunk)
            
            # Guardar chunk (append si no es el primero)
            if puntos_guardados == 0:
                chunk_a_guardar.to_csv(archivo_salida, index=False, mode='w')
            else:
                chunk_a_guardar.to_csv(archivo_salida, index=False, mode='a', header=False)
            
            puntos_guardados += puntos_en_chunk
            print(f"Progreso: {puntos_guardados:,}/{cantidad_a_guardar:,} puntos guardados")
            
            # Liberar memoria
            del chunk
            del chunk_a_guardar
        
        print(f"Puntos guardados exitosamente en: {archivo_salida}")
        print(f"Información del archivo guardado:")
        print(f"   - Puntos guardados: {puntos_guardados:,}")
        print(f"   - Archivo de salida: {archivo_salida}")
        
        # Verificar el archivo guardado
        if os.path.exists(archivo_salida):
            tamaño_salida = os.path.getsize(archivo_salida)
            tamaño_salida_mb = tamaño_salida / (1024 * 1024)
            print(f"   - Tamaño del archivo guardado: {tamaño_salida_mb:.1f} MB")
        
        return True
        
    except Exception as e:
        print(f"Error al procesar el archivo: {str(e)}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    guardar_puntos() 
