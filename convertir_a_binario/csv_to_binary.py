#!/usr/bin/env python3
"""
Script para convertir archivos CSV a formato binario para el GeoCluster-Tree
Autor: Tu Nombre
Fecha: 2024
Uso: python csv_to_binary.py archivo_entrada.csv archivo_salida.bin
"""

import sys
import struct
import csv
import os
import time
from pathlib import Path
import argparse

def mostrar_progreso(actual, total, inicio_tiempo):
    """Muestra una barra de progreso"""
    porcentaje = (actual / total) * 100
    tiempo_transcurrido = time.time() - inicio_tiempo
    if actual > 0:
        tiempo_estimado = (tiempo_transcurrido / actual) * (total - actual)
        print(f"\rProgreso: [{actual:>8}/{total:<8}] {porcentaje:5.1f}% | "
              f"Tiempo: {tiempo_transcurrido:5.1f}s | "
              f"ETA: {tiempo_estimado:5.1f}s", end="", flush=True)
    else:
        print(f"\rProgreso: [{actual:>8}/{total:<8}] {porcentaje:5.1f}% | "
              f"Tiempo: {tiempo_transcurrido:5.1f}s", end="", flush=True)

def convertir_csv_a_binario(archivo_csv, archivo_binario, mostrar_detalles=True):
    """
    Convierte un archivo CSV a formato binario optimizado para C++
    
    Args:
        archivo_csv (str): Ruta al archivo CSV de entrada
        archivo_binario (str): Ruta al archivo binario de salida
        mostrar_detalles (bool): Si mostrar información detallada
    
    Returns:
        bool: True si la conversión fue exitosa, False en caso contrario
    """
    if mostrar_detalles:
        print(f"🔄 Convirtiendo {archivo_csv} a {archivo_binario}...")
        print("=" * 60)
    
    # Verificar que el archivo CSV existe
    if not os.path.exists(archivo_csv):
        print(f"❌ ERROR: No se encontró el archivo {archivo_csv}")
        return False
    
    puntos = []
    contador = 0
    errores = 0
    inicio_tiempo = time.time()
    
    try:
        with open(archivo_csv, 'r', encoding='utf-8') as csvfile:
            reader = csv.reader(csvfile)
            
            # Saltar la línea de encabezado
            try:
                header = next(reader)
                if mostrar_detalles:
                    print(f"📋 Encabezado detectado: {len(header)} columnas")
                    print(f"   Columnas: {', '.join(header[:5])}{'...' if len(header) > 5 else ''}")
            except StopIteration:
                print("❌ ERROR: Archivo CSV vacío")
                return False
            
            # Leer todos los puntos
            for row_num, row in enumerate(reader, start=2):
                if len(row) < 3:  # Mínimo: ID, latitud, longitud
                    errores += 1
                    continue
                
                try:
                    # Extraer datos básicos
                    id_punto = int(row[0])
                    latitud = float(row[1])
                    longitud = float(row[2])
                    
                    # Extraer atributos (columnas 3 en adelante)
                    atributos = []
                    for i in range(3, len(row)):
                        atributos.append(float(row[i]))
                    
                    puntos.append({
                        'id': id_punto,
                        'latitud': latitud,
                        'longitud': longitud,
                        'atributos': atributos
                    })
                    
                    contador += 1
                    if contador % 10000 == 0 and mostrar_detalles:
                        mostrar_progreso(contador, 0, inicio_tiempo)
                        
                except (ValueError, IndexError) as e:
                    errores += 1
                    if errores <= 5:  # Solo mostrar los primeros 5 errores
                        print(f"\n⚠️  Error en línea {row_num}: {e}")
                    continue
    
    except Exception as e:
        print(f"\n❌ Error leyendo CSV: {e}")
        return False
    
    if mostrar_detalles:
        print(f"\n✅ Total de puntos leídos: {len(puntos):,}")
        if errores > 0:
            print(f"⚠️  Errores encontrados: {errores}")
    
    # Escribir archivo binario
    try:
        with open(archivo_binario, 'wb') as binfile:
            # Escribir número total de puntos
            binfile.write(struct.pack('i', len(puntos)))
            
            # Escribir cada punto
            for i, punto in enumerate(puntos):
                # ID (int)
                binfile.write(struct.pack('i', punto['id']))
                
                # Latitud y longitud (double)
                binfile.write(struct.pack('dd', punto['latitud'], punto['longitud']))
                
                # Número de atributos (int)
                num_atributos = len(punto['atributos'])
                binfile.write(struct.pack('i', num_atributos))
                
                # Atributos (array de doubles, máximo 12)
                atributos_array = punto['atributos'][:12]  # Limitar a 12 atributos
                while len(atributos_array) < 12:  # Rellenar con ceros si es necesario
                    atributos_array.append(0.0)
                
                for atributo in atributos_array:
                    binfile.write(struct.pack('d', atributo))
                
                if i % 10000 == 0 and mostrar_detalles:
                    mostrar_progreso(i, len(puntos), inicio_tiempo)
        
        # Calcular estadísticas
        tamaño_csv = os.path.getsize(archivo_csv)
        tamaño_bin = os.path.getsize(archivo_binario)
        compresion = (1 - tamaño_bin / tamaño_csv) * 100
        tiempo_total = time.time() - inicio_tiempo
        
        if mostrar_detalles:
            print(f"\n" + "=" * 60)
            print(f"🎉 CONVERSIÓN COMPLETADA EXITOSAMENTE!")
            print(f"📊 ESTADÍSTICAS:")
            print(f"   📁 Archivo CSV: {tamaño_csv:,} bytes ({tamaño_csv/1024/1024:.1f} MB)")
            print(f"   💾 Archivo binario: {tamaño_bin:,} bytes ({tamaño_bin/1024/1024:.1f} MB)")
            print(f"   📉 Compresión: {compresion:.1f}%")
            print(f"   📈 Puntos convertidos: {len(puntos):,}")
            print(f"   ⏱️  Tiempo total: {tiempo_total:.1f} segundos")
            print(f"   🚀 Velocidad: {len(puntos)/tiempo_total:.0f} puntos/segundo")
            print(f"   📍 Ubicación: {os.path.abspath(archivo_binario)}")
        
        return True
        
    except Exception as e:
        print(f"\n❌ Error escribiendo archivo binario: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(
        description='Convierte archivos CSV a formato binario para GeoCluster-Tree',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Ejemplos de uso:
  python csv_to_binary.py Database/puntos500k.csv Database/puntos500k.bin
  python csv_to_binary.py puntos10k.csv puntos10k.bin --quiet
  python csv_to_binary.py *.csv --output-dir Database/
        """
    )
    
    parser.add_argument('input', help='Archivo CSV de entrada')
    parser.add_argument('output', nargs='?', help='Archivo binario de salida')
    parser.add_argument('--output-dir', help='Directorio de salida para archivos binarios')
    parser.add_argument('--quiet', action='store_true', help='Modo silencioso (menos output)')
    
    args = parser.parse_args()
    
    # Procesar argumentos
    if args.output_dir:
        # Si se especifica directorio de salida, generar nombre automáticamente
        input_path = Path(args.input)
        output_path = Path(args.output_dir) / f"{input_path.stem}.bin"
    elif args.output:
        output_path = Path(args.output)
    else:
        # Generar nombre automáticamente
        input_path = Path(args.input)
        output_path = input_path.with_suffix('.bin')
    
    # Convertir
    if convertir_csv_a_binario(str(args.input), str(output_path), not args.quiet):
        print(f"\n✅ Conversión exitosa!")
        print(f"📁 Archivo binario: {output_path}")
        print(f"🚀 Ahora puedes usar este archivo en tu programa C++")
    else:
        print(f"\n❌ Error en la conversión")
        sys.exit(1)

if __name__ == "__main__":
    main() 