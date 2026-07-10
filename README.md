# Final_Project_EDA — GeoCluster-Tree y rstarLib

Repositorio con dos partes:

1. **El proyecto original (GeoCluster-Tree)**: un R*-tree modificado con
   clusterización geográfica y atributiva para búsquedas espaciales y por
   similitud sobre el dataset de NYC Taxi. Proyecto final del curso de
   Estructuras de Datos Avanzadas.
2. **rstarLib**: la evolución de ese proyecto convertida en una librería C++
   reutilizable (header-only), pensada para cualquier tipo de dato 2D.

## Estructura del repositorio

```
├── ideaStruct/        Idea y diseño original (PDF + PPT de GeoCluster-Tree)
├── database/          Datos (los .csv NO se versionan por tamaño)
├── preprocesameinto/  Pipeline Python original: limpieza, PCA,
│                      clusterización geográfica y atributiva
├── bin/               Conversión CSV -> binario (versión original)
├── struct/            Implementación C++ original: GeoCluster.h/.cpp,
│                      main.cpp, tests (make test) y Makefile
└── rstarLib/          LA LIBRERÍA — ver rstarLib/README.md
```

## El proyecto original (struct/ + preprocesameinto/)

Flujo: los datos crudos de NYC Taxi se limpian y clusterizan en Python
(cluster geográfico con k-means + subclusters atributivos por zona), se
convierten a binario, y el programa C++ los indexa en un R*-tree cuyas hojas
guardan referencias ligeras (`PointID`); los datos completos viven en una
hash table. Consultas del proyecto:

- Los **n puntos más parecidos** a un punto dado dentro de un bounding box.
- Los **grupos de puntos similares entre sí** dentro de un bounding box.

La inserción del árbol sigue el paper original del R*-tree (Beckmann et al.
1990, `rstar-tree.pdf`, no versionado): ChooseSubtree con costo de overlap,
split por suma de márgenes, forced reinsert (close reinsert) y propagación
de splits con tests de conformidad en `struct/tests/`.

Compilar y probar:

```bash
cd struct
make test        # tests de conformidad con el paper (M=8, m=3)
make geocluster  # binario principal (requiere los datos .bin)
```

## La librería (rstarLib/)

Reescritura generalizada del proyecto como librería header-only C++17 sin
dependencias, para reutilizarla con cualquier dato (`template <typename T>`):

- **`rstartree.hpp`** — `RStarTree2D<T>`: arena de datos + árbol R* completo
  (inserción Beckmann, `buscarRango`, `kVecinos`, `recorrer`, `eliminar` con
  condensación). M y m son parámetros del constructor.
- **`indice_por_id.hpp`** — `IndicePorId`: id externo → posición, O(1).
- **`grupos_por_hoja.hpp`** — `GruposPorHoja`: precomputación por hoja
  (grupos por etiqueta + centroides) que implementa las dos consultas del
  proyecto de forma genérica vía extractores.
- **`preprocesameinto/pipeline/`** — pipeline Python corregido: limpieza con
  clip, PCA + clustering atributivo global (k por silhouette), export binario
  vectorizado.

```bash
cd rstarLib
make test      # suite completa de la librería
make ejemplo   # demo con 100k puntos sintéticos estilo taxi
```

Documentación completa: [rstarLib/README.md](rstarLib/README.md) (API y uso),
[rstarLib/DISENO.md](rstarLib/DISENO.md) (decisiones de arquitectura).

## Datos

Los `.csv` y `.bin` no se versionan (ver `.gitignore`). El pipeline se corre
localmente: Python 3 con `pandas`, `scikit-learn` y `numpy`.
