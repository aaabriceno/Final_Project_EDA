# Diseño: rstarLib — R*-tree reutilizable con grupos precalculados

**Fecha:** 2026-07-08
**Estado:** aprobado (diseño validado en conversación)
**Regla de oro:** el proyecto original (`struct/`, `preprocesameinto/`, etc.) NO se toca. Todo el trabajo nuevo vive en `rstarLib/`.

## 1. Objetivo

Convertir la combinación única del proyecto GeoCluster (R*-tree + clusterización por
etiquetas + consultas priorizadas por grupo) en una librería C++ reutilizable para
cualquier tipo de dato 2D (lat/long u otras coordenadas), pensada para reuso futuro
(tesis de vulnerabilidades). Payload genérico vía template `T`; dimensión fija 2D
(decisión explícita del usuario: generalizar a N-D queda para después).

## 2. Arquitectura — 1 almacén + 3 aceleradores

Principio: un índice = una forma de ordenar = un tipo de pregunta. Ninguna estructura
sola responde preguntas por zona, por id y por similitud a la vez.

| Pieza | Contenido | Pregunta que responde | Tamaño (5M pts) |
|---|---|---|---|
| **Arena** `vector<T>` | dato completo, UNA vez | "dame el dato de la posición i" | ~440MB |
| **R*-tree** | hojas `{x, y, idx}` (20 B/entrada) | "¿quiénes están en esta zona?" | ~100MB |
| **Hash** `unordered_map<Id, uint32>` (opcional) | id externo → posición en arena | "¿dónde está el id 45023?" O(1) | ~60MB |
| **Grupos por hoja** (opcional) | cajones por etiqueta + centroide | respuestas pre-armadas (precomputación) | ~40MB |

Decisiones debatidas y cerradas:
- **Coordenadas viven dos veces a propósito**: en la hoja (hot path del árbol: splits,
  descensos) y dentro de `T` en la arena (las consultas por id necesitan saber dónde
  está el referente). Todo lo demás vive una sola vez. Es el mismo trade-off que hace
  cualquier motor de BD con columnas indexadas.
- **Índice (uint32) y no puntero**: sobrevive a reubicaciones del vector y pesa la mitad.
- **La hoja NO guarda id ni etiqueta de cluster**: el id está en `arena[idx]`; la
  pertenencia a grupos la materializa la capa de grupos. Menos bytes, cero redundancia.
- **La hash NO almacena datos**: solo int→int. Su única función es el punto de entrada
  por id externo (la consulta 1 del proyecto empieza con un id que puede estar FUERA
  del bbox de consulta — sin hash sería un escaneo O(n); el main.cpp original relee el
  archivo binario completo del disco para esto).

## 3. Maquinaria del árbol

Se **copia** de `struct/GeoCluster.cpp` la maquinaria Beckmann 1990 ya reescrita y
testeada (commit bf01d4e): chooseSubTree con overlap real (optimización 32 candidatos),
split con CSA por suma de márgenes + CSI, overflowTreatment por nivel, close reinsert
p=30%, ajuste de MBRs del camino, split de internos con propagación y nueva raíz.

Cambios respecto a la copia:
- Template `template <typename T>` — clase `RStarTree2D<T>`.
- M y m son **parámetros del constructor** (default 1200/480), no macros. Los tests
  usan M=8, m=3 sin flags de compilación.
- Entradas de hoja `{double x, double y, uint32_t idx}`.
- Sin `Punto`, `PointID`, `MicroCluster`, clusters ids, ni hash de puntos completos.

## 4. API pública

```cpp
template <typename T>
class RStarTree2D {
public:
    RStarTree2D(int M = 1200, int m = 480);

    // núcleo
    uint32_t insertar(double x, double y, T dato);          // devuelve idx en arena
    const T& dato(uint32_t idx) const;                       // arena[idx]
    size_t tamano() const;

    // consultas espaciales
    struct Resultado { double x, y; uint32_t idx; };
    vector<Resultado> buscarRango(const Caja& bbox) const;
    vector<Resultado> kVecinos(double x, double y, int k) const; // branch-and-bound
    void recorrer(function<void(const Resultado&)> visita) const;

    // eliminación (Guttman/Beckmann: condensación + reinserción de huérfanos)
    bool eliminar(double x, double y, function<bool(const T&)> coincide);
};
```

Módulos opcionales (archivos/clases aparte, componibles):

```cpp
// indice por id externo
template <typename T, typename Id>
class IndicePorId {  // envuelve unordered_map<Id, uint32_t>
    IndicePorId(const RStarTree2D<T>& arbol, function<Id(const T&)> idDe);
    optional<uint32_t> buscar(Id id) const;
};

// grupos precalculados por hoja (la precomputación pedida por el profesor)
template <typename T, typename Etiqueta>
class GruposPorHoja {
    GruposPorHoja(RStarTree2D<T>& arbol, function<Etiqueta(const T&)> etiquetaDe,
                  function<vector<double>(const T&)> caracteristicasDe);
    void construir();          // post-carga, una pasada por hojas: cajones + centroides
    // invalidación perezosa: inserciones posteriores marcan la hoja sucia;
    // el cajón se rearma en la próxima consulta que lo toque

    // las dos consultas estrella del proyecto, generalizadas:
    // 1) n más parecidos a un dato de referencia dentro de un bbox
    //    (prioridad: misma etiqueta → grupos rankeados por distancia de centroides)
    vector<uint32_t> nSimilares(const Caja& bbox, const T& referencia, int n) const;
    // 2) grupos similares entre sí dentro del bbox (se devuelven pre-armados)
    vector<vector<uint32_t>> gruposEnRango(const Caja& bbox) const;
};
```

**Extractores** (`idDe`, `etiquetaDe`, `caracteristicasDe`): funciones que el usuario
provee para que la librería lea campos de un `T` que no conoce. Así los datos viejos
del proyecto (par `(cluster_geo, subcluster)` como etiqueta compuesta) y los futuros
(etiqueta global única) usan la misma librería sin cambios.

## 5. Pipeline de datos recomendado (documentado en README)

```
PYTHON (offline, 1 vez):
  1. limpiar (clip de negativos — NO desplazar la columna; filtro geográfico)
  2. PCA (k por varianza acumulada)
  3. clustering atributivo GLOBAL (MiniBatchKMeans; k por silhouette)
     — el cluster geográfico desaparece: el árbol hace el trabajo espacial
  4. ordenar por (x, y) + exportar .bin  (escritura vectorizada)

C++ (carga): insertar ordenado → construirGrupos()   [segundos]
C++ (consulta): hash O(1) por id + hojas del bbox + grupos pre-armados
```

Nota: ordenar antes o después del clustering da resultados idénticos (el clustering
es independiente del orden de filas); se ordena al final por higiene — pegado al único
consumidor (la carga del árbol).

## 6. Tests (TDD, mismo estilo que struct/tests)

- Se porta la suite de conformidad Beckmann (invariantes m/M, hojas al mismo nivel,
  contención de MBRs, eje de split, MBRs de ancestros, sin pérdida/duplicados).
- Nuevos: kNN contra fuerza bruta; eliminar mantiene invariantes y borra de verdad;
  recorrer visita todo exactamente una vez; nSimilares con referente FUERA del bbox;
  gruposEnRango devuelve los grupos correctos; invalidación de grupos tras insertar.
- `make test` con M=8, m=3 por constructor (sin macros).

## 7. Estructura de carpeta

```
rstarLib/
├── DISENO.md            (este documento)
├── rstartree.hpp        (RStarTree2D<T> — header-only)
├── indice_por_id.hpp    (módulo opcional)
├── grupos_por_hoja.hpp  (módulo opcional)
├── tests/test_rstarlib.cpp
├── ejemplo/ejemplo_taxis.cpp   (replica las 2 consultas del proyecto con datos taxi)
├── Makefile             (make test / make ejemplo)
└── README.md            (API + pipeline + cómo llevarla a otro proyecto)
```

## 8. Fuera de alcance (documentado como futuro)

- **N dimensiones** (template DIM) — cuando la tesis lo pida.
- **Ball tree por hoja** — índice métrico para kNN exacto en espacio de atributos;
  reemplazaría el ranking por centroides. Solo se justifica con hojas grandes.
- **Bulk loading STR** — mejor que inserción ordenada para cargas masivas.
- **Cualquier cambio en `struct/`** — el proyecto original queda como está.
  (Única excepción acordada: 3 líneas del `.gitignore` raíz que apuntan a la ruta
  vieja `5Estructura/` y dejan pasar binarios compilados.)
