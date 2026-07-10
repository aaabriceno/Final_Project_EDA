# rstarLib

R*-tree 2D reutilizable (Beckmann et al. 1990) con **grupos precalculados**:
la combinación árbol espacial + clusterización por etiquetas del proyecto
GeoCluster, generalizada para cualquier tipo de dato.

## Arquitectura — 1 almacén + 3 aceleradores

Cada estructura responde UNA forma de pregunta (ver `DISENO.md`):

| Pieza | Header | Pregunta que responde |
|---|---|---|
| Arena `vector<T>` | `rstartree.hpp` | "dame el dato de la posición idx" — acceso directo |
| R*-tree (hojas `{x, y, idx}`) | `rstartree.hpp` | "¿quiénes están en esta zona?" |
| `IndicePorId` (opcional) | `indice_por_id.hpp` | "¿dónde está el id 45020?" — O(1) |
| `GruposPorHoja` (opcional) | `grupos_por_hoja.hpp` | respuestas pre-armadas por etiqueta |

El dato completo vive UNA sola vez en la arena. Las hojas del árbol guardan
20 bytes por punto. Las coordenadas se duplican a propósito (hot path del
árbol + consultas por id), igual que hace cualquier BD con columnas indexadas.

## Instalación

Copiar los tres `.hpp` a tu proyecto. C++17, sin dependencias.

```cpp
#include "rstartree.hpp"
#include "indice_por_id.hpp"    // solo si consultas por id
#include "grupos_por_hoja.hpp"  // solo si usas grupos precalculados
```

## Uso mínimo

```cpp
struct MiDato { int id; int etiqueta; std::vector<double> caracteristicas; };

RStarTree2D<MiDato> arbol;                        // M=1200, m=480 (paper)
arbol.insertar(x, y, MiDato{...});                // carga (idealmente ordenada por x,y)

IndicePorId<MiDato, int> porId(arbol, [](const MiDato& d) { return d.id; });
GruposPorHoja<MiDato, int> grupos(arbol,
    [](const MiDato& d) { return d.etiqueta; },
    [](const MiDato& d) { return d.caracteristicas; });
grupos.construir();                               // precomputación (una vez)

// consulta 1: n más parecidos a un id dentro de un bbox (id puede estar fuera)
auto idx = porId.buscar(45020);
auto similares = grupos.nSimilares(Caja(x1, y1, x2, y2), *idx, 50);

// consulta 2: grupos similares entre sí dentro del bbox
auto gs = grupos.gruposEnRango(Caja(x1, y1, x2, y2));
```

Demo completa: `make ejemplo` (`ejemplo/ejemplo_taxis.cpp`, 100k puntos sintéticos).

## API de referencia

| Método | Qué hace | Costo |
|---|---|---|
| `insertar(x, y, dato)` → idx | inserta; devuelve posición en arena | O(log n) amortizado |
| `dato(idx)` | payload por posición | O(1) |
| `buscarRango(caja)` | puntos dentro del bbox | O(log n + resultados) |
| `kVecinos(x, y, k)` | k más cercanos, ordenados | best-first con poda |
| `recorrer(f)` | visita todos los puntos | O(n) |
| `eliminar(x, y, pred)` | quita del índice (condensación del paper) | O(log n) + reinserts |
| `IndicePorId::buscar(id)` | id externo → idx | O(1) |
| `GruposPorHoja::construir()` | arma cajones por etiqueta + centroides | O(n), una vez |
| `GruposPorHoja::nSimilares(bbox, idx, n)` | consulta 1 (prioridad por etiqueta) | grupos pre-armados |
| `GruposPorHoja::gruposEnRango(bbox)` | consulta 2 (grupos ≥ 2 miembros) | grupos pre-armados |

Notas:
- `eliminar` es *tombstone*: el dato sigue en la arena (`dato(idx)` válido), solo
  desaparece del índice espacial.
- Tras insertar después de `construir()`, las hojas mutadas se rearman solas en
  la siguiente consulta (invalidación perezosa por versión de hoja).
- M/m se validan en el constructor: `2 <= m <= M/2`. El paper recomienda m = 40% de M.

## Pipeline de datos recomendado

```
PYTHON (offline, 1 vez):
  1. limpiar        — filtro geográfico + clip de negativos (preprocesameinto/pipeline/1_limpiar.py)
  2. PCA + k-means  — clustering atributivo GLOBAL, k por silhouette (2_pca_clustering.py)
  3. ordenar + .bin — orden (lat, lon) y binario vectorizado (3_ordenar_a_binario.py)

C++ (carga):    insertar ordenado → grupos.construir()      [segundos]
C++ (consulta): porId O(1) + hojas del bbox + grupos pre-armados
```

El cluster geográfico del pipeline viejo ya no existe: el árbol hace el trabajo
espacial (mejor: es adaptativo). Datos viejos con etiquetas por zona siguen
funcionando usando la etiqueta compuesta `(cluster_geo, subcluster)` en el extractor.

## Extensiones futuras (documentadas, no implementadas)

- **N dimensiones**: generalizar `Caja` y las hojas a `DIM` (template).
- **Ball tree por hoja**: kNN exacto en el espacio de atributos, reemplazando el
  ranking por centroides. Solo se justifica con hojas grandes.
- **Bulk loading STR**: mejor que inserción ordenada para cargas masivas.
