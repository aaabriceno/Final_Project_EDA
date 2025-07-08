#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Script ULTRA-OPTIMIZADO para convertir CSV a binario
Especialmente diseñado para archivos grandes de datos geoespaciales
"""

import pandas as pd
import numpy as np
import struct
import os
import sys
from tqdm import tqdm

def convertir_csv_a_binario_ultra_rapido(archivo_csv, archivo_binario):
    """
    Convierte CSV a binario de forma ultra-rápida usando pandas y numpy
    """
    print(f"CONVIRTIENDO CSV A BINARIO (ULTRA-RÁPIDO)")
    print(f"Archivo CSV: {archivo_csv}")
    print(f"Archivo binario: {archivo_binario}")
    
    # Verificar que el archivo CSV existe
    if not os.path.exists(archivo_csv):
        print(f"Error: No se encuentra el archivo {archivo_csv}")
        return False
    
    try:
        # Leer CSV en chunks para manejar archivos grandes
        chunk_size = 100000  # Procesar 100k líneas a la vez
        total_puntos = 0
        
        # Contar líneas totales para el progreso
        print("Contando líneas del archivo...")
        with open(archivo_csv, 'r') as f:
            total_lineas = sum(1 for _ in f) - 1  # Menos 1 por el header
        
        print(f" Total de líneas: {total_lineas:,}")
        
        # Crear archivo binario
        with open(archivo_binario, 'wb') as bin_file:
            # Escribir placeholder para número de puntos (se actualizará al final)
            bin_file.write(struct.pack('i', 0))
            
            # Procesar archivo en chunks
            print("⚡ Procesando archivo en chunks...")
            
            for chunk in tqdm(pd.read_csv(archivo_csv, chunksize=chunk_size), 
                            desc="Procesando chunks", unit="chunk"):
                
                # Procesar cada fila del chunk
                for _, row in chunk.iterrows():
                    try:
                        # Extraer datos
                        cluster_espacial = int(row.iloc[0])
                        cluster_atributivo = int(row.iloc[1])
                        trip_id = int(row.iloc[2])
                        lat = float(row.iloc[3])
                        lon = float(row.iloc[4])
                        
                        # Extraer atributos (columnas 5 en adelante)
                        atributos = row.iloc[5:].values.astype(float)
                        
                        # Limitar a 12 atributos máximo
                        if len(atributos) > 12:
                            atributos = atributos[:12]
                        elif len(atributos) < 12:
                            # Rellenar con ceros si faltan atributos
                            atributos = np.pad(atributos, (0, 12 - len(atributos)), 'constant')
                        
                        # Escribir al archivo binario
                        # Formato: id(4) + lat(8) + lon(8) + cluster_esp(4) + cluster_attr(4) + num_attr(4) + atributos(12*8)
                        bin_file.write(struct.pack('i', trip_id))
                        bin_file.write(struct.pack('d', lat))
                        bin_file.write(struct.pack('d', lon))
                        bin_file.write(struct.pack('i', cluster_espacial))
                        bin_file.write(struct.pack('i', cluster_atributivo))
                        bin_file.write(struct.pack('i', len(atributos)))
                        
                        # Escribir atributos
                        for attr in atributos:
                            bin_file.write(struct.pack('d', attr))
                        
                        total_puntos += 1
                        
                    except Exception as e:
                        print(f"Error en línea: {e}")
                        continue
            
            # Volver al inicio y escribir el número real de puntos
            bin_file.seek(0)
            bin_file.write(struct.pack('i', total_puntos))
        
        # Mostrar estadísticas finales
        tamaño_archivo = os.path.getsize(archivo_binario) / (1024 * 1024)  # MB
        
        print(f"\nCONVERSIÓN COMPLETADA")
        print(f"=================================")
        print(f"Puntos procesados: {total_puntos:,}")
        print(f"Archivo binario: {archivo_binario}")
        print(f"Tamaño del archivo: {tamaño_archivo:.1f} MB")
        print(f"Velocidad: {total_puntos/total_lineas*100:.1f}% de líneas procesadas")
        
        # Verificar el archivo creado
        with open(archivo_binario, 'rb') as f:
            num_puntos_verificacion = struct.unpack('i', f.read(4))[0]
            print(f"Verificación: {num_puntos_verificacion:,} puntos en archivo binario")
        
        return True
        
    except Exception as e:
        print(f"Error durante la conversión: {e}")
        return False

def main():
    """
    Función principal
    """
    print("CONVERTIDOR CSV A BINARIO (ULTRA-RÁPIDO)")
    print("=" * 50)
    
    # Archivos por defecto
    archivo_csv = "5Estructura/5clusterizacion_geografica_attr.csv"
    archivo_binario = "5Estructura/5clusterizacion_geografica_attr.bin"
    
    # Verificar argumentos de línea de comandos
    if len(sys.argv) >= 2:
        archivo_csv = sys.argv[1]
    if len(sys.argv) >= 3:
        archivo_binario = sys.argv[2]
    
    print(f"Archivo CSV de entrada: {archivo_csv}")
    print(f"Archivo binario de salida: {archivo_binario}")
    
    # Verificar que el archivo CSV existe
    if not os.path.exists(archivo_csv):
        print(f"Error: No se encuentra el archivo {archivo_csv}")
        print("Asegúrate de que el archivo CSV esté en el directorio correcto")
        return
    
    # Confirmar conversión
    print(f"\n¿Continuar con la conversión? (s/n): ", end="")
    respuesta = input().lower()
    
    if respuesta in ['s', 'si', 'y', 'yes']:
        # Realizar conversión
        if convertir_csv_a_binario_ultra_rapido(archivo_csv, archivo_binario):
            print(f"\n¡Conversión exitosa!")
            print(f"Ahora puedes usar el archivo binario para inserción ultra-rápida en C++")
        else:
            print(f"\nError en la conversión")
    else:
        print("Conversión cancelada")

if __name__ == "__main__":
    main() 
