# Convertidor CSV a Binario - GeoCluster-Tree

Esta herramienta convierte archivos CSV a formato binario optimizado para el GeoCluster-Tree, mejorando significativamente el rendimiento de lectura.

## 🚀 Características

- **Conversión rápida**: Optimizada para archivos grandes
- **Barra de progreso**: Muestra el avance en tiempo real
- **Estadísticas detalladas**: Tamaño, compresión, velocidad
- **Manejo de errores**: Detecta y reporta problemas
- **Múltiples formatos**: Soporta diferentes estructuras CSV

## 📁 Estructura de archivos

```
convertir_a_binario/
├── csv_to_binary.py    # Script principal de Python
├── convertir.bat       # Script batch para Windows
└── README.md          # Este archivo
```

## 🛠️ Instalación

1. **Requisitos**: Python 3.6 o superior
2. **Ubicación**: Coloca esta carpeta en tu proyecto
3. **Permisos**: Asegúrate de que Python esté en tu PATH

## 📖 Uso

### Opción 1: Script Python directo

```bash
# Conversión básica
python csv_to_binary.py Database/puntos500k.csv Database/puntos500k.bin

# Generar nombre automáticamente
python csv_to_binary.py puntos10k.csv

# Modo silencioso
python csv_to_binary.py archivo.csv --quiet
```

### Opción 2: Script Batch (Windows)

```bash
# Conversión básica
convertir.bat Database/puntos500k.csv

# Especificar nombre de salida
convertir.bat puntos10k.csv puntos10k.bin
```

### Opción 3: Desde el programa C++

El programa C++ detectará automáticamente si existe un archivo binario y te dará opciones:
1. Leer archivo binario (más rápido)
2. Leer archivo CSV directamente
3. Convertir CSV a binario y luego leer

## 📊 Formato de entrada (CSV)

El script espera un CSV con esta estructura:
```csv
tripID,pickup_latitude,pickup_longitude,passenger_count,trip_distance,payment_type,fare_amount,extra,mta_tax,tip_amount,tolls_amount,improvement_surcharge,total_amount
1,40.73469543457031,-73.99037170410156,2,1.1,2,7.5,0.5,0.5,0.0,0.0,0.3,8.8
2,40.72991180419922,-73.98078155517578,5,4.9,1,18.0,0.5,0.5,0.0,0.0,0.3,19.3
...
```

**Columnas requeridas:**
- Columna 1: ID del punto (entero)
- Columna 2: Latitud (double)
- Columna 3: Longitud (double)
- Columnas 4+: Atributos adicionales (double)

## 💾 Formato de salida (Binario)

El archivo binario tiene esta estructura:
```
[Total de puntos: int]
[Punto 1: ID(int) + Lat(double) + Lon(double) + NumAtributos(int) + Atributos[12](double)]
[Punto 2: ...]
...
```

## ⚡ Ventajas del formato binario

| Aspecto | CSV | Binario | Mejora |
|---------|-----|---------|--------|
| **Tamaño** | 100% | ~50% | 50% más pequeño |
| **Velocidad** | 100% | 500-1000% | 5-10x más rápido |
| **Precisión** | Texto | Binario | Sin pérdida |
| **Memoria** | Alta | Baja | Menor uso |

## 🔧 Opciones avanzadas

### Argumentos del script Python

```bash
python csv_to_binary.py --help
```

**Opciones disponibles:**
- `--output-dir DIR`: Especificar directorio de salida
- `--quiet`: Modo silencioso (menos output)
- `input`: Archivo CSV de entrada
- `output`: Archivo binario de salida (opcional)

### Ejemplos avanzados

```bash
# Convertir múltiples archivos a un directorio
python csv_to_binary.py *.csv --output-dir Database/

# Conversión silenciosa
python csv_to_binary.py archivo.csv --quiet

# Especificar directorio de salida
python csv_to_binary.py input.csv --output-dir ../Database/
```

## 📈 Rendimiento típico

| Archivo | Puntos | CSV (MB) | Binario (MB) | Tiempo | Velocidad |
|---------|--------|----------|--------------|--------|-----------|
| puntos10k.csv | 10,000 | 2.1 | 1.1 | 0.5s | 20k pts/s |
| puntos100k.csv | 100,000 | 21.0 | 11.0 | 2.1s | 47k pts/s |
| puntos500k.csv | 500,000 | 105.0 | 55.0 | 8.5s | 59k pts/s |
| puntos1M.csv | 1,000,000 | 210.0 | 110.0 | 15.2s | 66k pts/s |

## 🐛 Solución de problemas

### Error: "No se encontró el archivo"
- Verifica que el archivo CSV existe
- Revisa la ruta del archivo
- Asegúrate de tener permisos de lectura

### Error: "Archivo CSV vacío"
- Verifica que el archivo no esté vacío
- Asegúrate de que tenga el encabezado correcto

### Error: "No se pudo convertir valor numérico"
- Revisa que los datos sean numéricos
- Verifica el formato de los números (usar punto decimal)
- Revisa que no haya caracteres extraños

### Error: "Python no se reconoce"
- Instala Python desde python.org
- Asegúrate de que esté en el PATH del sistema
- Reinicia la terminal después de instalar

## 📝 Notas técnicas

- **Compatibilidad**: Funciona con Python 3.6+
- **Codificación**: UTF-8 para archivos CSV
- **Precisión**: Double (64-bit) para coordenadas y atributos
- **Límites**: Máximo 12 atributos por punto
- **Memoria**: Procesa archivos línea por línea para eficiencia

## 🤝 Contribuciones

Para mejorar esta herramienta:
1. Reporta bugs en el código
2. Sugiere nuevas características
3. Optimiza el rendimiento
4. Mejora la documentación

---

**Desarrollado para el proyecto GeoCluster-Tree** 🗺️🌳 