# Plan de implementación: rstarLib

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Construir la librería `RStarTree2D<T>` header-only (arena + árbol + índice por id + grupos por hoja) según `rstarLib/DISENO.md`, con tests TDD, ejemplo y pipeline Python corregido.

**Architecture:** Header-only C++17. Tres headers componibles: `rstartree.hpp` (núcleo: arena `vector<T>` + R*-tree Beckmann con hojas `{x,y,idx}`), `indice_por_id.hpp` (hash id→idx opcional), `grupos_por_hoja.hpp` (precomputación por hoja con extractores). La maquinaria Beckmann se **porta** de `rstarLib/struct/GeoCluster.cpp` (ya testeada, commit bf01d4e), adaptada a template y M/m por constructor.

**Tech Stack:** C++17, STL pura (sin dependencias), g++, make. Python 3 + pandas/sklearn/numpy para el pipeline.

## Global Constraints

- C++17, header-only, cero dependencias externas.
- Nombres de API en español (consistente con el proyecto).
- M y m son parámetros de constructor; validar `2 <= m <= M/2` (lanzar `std::invalid_argument`).
- 2D fijo. Payload genérico `template <typename T>`.
- NO tocar nada fuera de `rstarLib/`. El proyecto original es intocable.
- Compilar siempre con `-std=c++17 -O2 -Wall`.
- Tests con M=8, m=3 vía constructor (sin macros).
- Fuente de la maquinaria a portar: `rstarLib/struct/GeoCluster.cpp` y `GeoCluster.h` (referirse por nombre de función, no por línea).
- Commit al final de cada tarea (mensajes en español, convención del repo).

---

### Task 1: Infraestructura + struct `Caja`

**Files:**
- Create: `rstarLib/rstartree.hpp`
- Create: `rstarLib/tests/test_rstarlib.cpp`
- Create: `rstarLib/Makefile`

**Interfaces:**
- Produces: `struct Caja` con `double lo[2], hi[2]`; métodos `reset()`, `estirar(const Caja&)`, `estirar(double x, double y)`, `area()`, `margen()`, `overlap(const Caja&)`, `interseca(const Caja&)`, `contiene(double x, double y)`, `dist2A(double x, double y)`. Constructor `Caja()` (reset) y `Caja(xmin, ymin, xmax, ymax)`.
- Produces: macro `CHECK(cond, msg)` y contador `fallos` en el test (mismo estilo que `rstarLib/struct/tests/test_rstar.cpp`).

- [ ] **Step 1: Makefile**

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

test: tests/test_rstarlib.cpp rstartree.hpp indice_por_id.hpp grupos_por_hoja.hpp
	$(CXX) $(CXXFLAGS) tests/test_rstarlib.cpp -o tests/test_rstarlib
	./tests/test_rstarlib

ejemplo: ejemplo/ejemplo_taxis.cpp rstartree.hpp indice_por_id.hpp grupos_por_hoja.hpp
	$(CXX) $(CXXFLAGS) ejemplo/ejemplo_taxis.cpp -o ejemplo/ejemplo_taxis
	./ejemplo/ejemplo_taxis

clean:
	rm -f tests/test_rstarlib ejemplo/ejemplo_taxis

.PHONY: test ejemplo clean
```

Nota: hasta que existan `indice_por_id.hpp` y `grupos_por_hoja.hpp` (Tasks 8-9), crear ambos archivos vacíos con solo `#pragma once` para que el Makefile funcione desde ya.

- [ ] **Step 2: Test que falla — geometría de Caja**

En `tests/test_rstarlib.cpp`:

```cpp
#include "../rstartree.hpp"
#include <iostream>
#include <string>
using namespace std;

static int fallos = 0;
#define CHECK(cond, msg) do { \
    if (cond) { cout << "  [OK]    " << msg << endl; } \
    else      { cout << "  [FALLA] " << msg << endl; fallos++; } \
} while (0)

static void test_caja() {
    cout << "\nT1: geometria de Caja" << endl;
    Caja a(0, 0, 10, 10), b(5, 5, 15, 15), c(20, 20, 30, 30);
    CHECK(a.area() == 100.0, "area 10x10 = 100");
    CHECK(a.margen() == 40.0, "margen 10x10 = 40");
    CHECK(a.overlap(b) == 25.0, "overlap esquinas 5x5 = 25");
    CHECK(a.interseca(b) && !a.interseca(c), "interseca b, no interseca c");
    CHECK(a.contiene(5, 5) && !a.contiene(11, 5), "contiene (5,5), no (11,5)");
    CHECK(a.dist2A(13, 14) == 3.0*3.0 + 4.0*4.0, "dist2 a punto externo = 25");
    CHECK(a.dist2A(5, 5) == 0.0, "dist2 a punto interno = 0");
    Caja d; d.estirar(3, 7); d.estirar(a);
    CHECK(d.lo[0] == 0 && d.hi[0] == 10 && d.lo[1] == 0 && d.hi[1] == 10, "estirar acumula");
}

int main() {
    cout << "=== Tests rstarLib ===" << endl;
    test_caja();
    cout << "\n=== Resultado: " << (fallos == 0 ? "TODOS PASAN" : to_string(fallos) + " FALLOS") << " ===" << endl;
    return fallos == 0 ? 0 : 1;
}
```

- [ ] **Step 3: Verificar que falla**: `cd rstarLib && make test` → error de compilación: `Caja` no existe.

- [ ] **Step 4: Implementar `Caja` en `rstartree.hpp`**

```cpp
#pragma once
// rstarLib — R*-tree 2D reutilizable (Beckmann et al. 1990)
// Ver DISENO.md. Header-only, C++17, sin dependencias.
#include <vector>
#include <queue>
#include <functional>
#include <algorithm>
#include <limits>
#include <cstdint>
#include <stdexcept>
#include <cmath>

struct Caja {
    double lo[2], hi[2];
    Caja() { reset(); }
    Caja(double xmin, double ymin, double xmax, double ymax) {
        lo[0] = xmin; lo[1] = ymin; hi[0] = xmax; hi[1] = ymax;
    }
    void reset() {
        lo[0] = lo[1] = std::numeric_limits<double>::max();
        hi[0] = hi[1] = std::numeric_limits<double>::lowest();
    }
    void estirar(const Caja& o) {
        for (int d = 0; d < 2; d++) {
            lo[d] = std::min(lo[d], o.lo[d]);
            hi[d] = std::max(hi[d], o.hi[d]);
        }
    }
    void estirar(double x, double y) {
        lo[0] = std::min(lo[0], x); hi[0] = std::max(hi[0], x);
        lo[1] = std::min(lo[1], y); hi[1] = std::max(hi[1], y);
    }
    double area() const { return (hi[0] - lo[0]) * (hi[1] - lo[1]); }
    double margen() const { return 2.0 * ((hi[0] - lo[0]) + (hi[1] - lo[1])); }
    double overlap(const Caja& o) const {
        double a = std::max(0.0, std::min(hi[0], o.hi[0]) - std::max(lo[0], o.lo[0]));
        double b = std::max(0.0, std::min(hi[1], o.hi[1]) - std::max(lo[1], o.lo[1]));
        return a * b;
    }
    bool interseca(const Caja& o) const {
        return !(hi[0] < o.lo[0] || o.hi[0] < lo[0] || hi[1] < o.lo[1] || o.hi[1] < lo[1]);
    }
    bool contiene(double x, double y) const {
        return x >= lo[0] && x <= hi[0] && y >= lo[1] && y <= hi[1];
    }
    // distancia al cuadrado del punto a la caja (0 si esta dentro)
    double dist2A(double x, double y) const {
        double dx = std::max({lo[0] - x, 0.0, x - hi[0]});
        double dy = std::max({lo[1] - y, 0.0, y - hi[1]});
        return dx * dx + dy * dy;
    }
};
```

- [ ] **Step 5: Verificar que pasa**: `make test` → T1 todo OK.
- [ ] **Step 6: Commit**: `git add rstarLib/ && git commit -m "rstarLib: infraestructura y struct Caja con tests"`

---

### Task 2: Esqueleto `RStarTree2D<T>` — arena e inserción en hoja única

**Files:**
- Modify: `rstarLib/rstartree.hpp`
- Modify: `rstarLib/tests/test_rstarlib.cpp`

**Interfaces:**
- Produces: `template <typename T> class RStarTree2D` con:
  - `RStarTree2D(int M = 1200, int m = 480)` — valida `2 <= m <= M/2`, si no `throw std::invalid_argument("m debe cumplir 2 <= m <= M/2")`.
  - `struct Resultado { double x, y; uint32_t idx; };` (público, es también la entrada de hoja)
  - `uint32_t insertar(double x, double y, T dato)` — agrega a arena, inserta al árbol, devuelve idx.
  - `const T& dato(uint32_t idx) const` / `T& dato(uint32_t idx)`
  - `size_t tamano() const` — puntos indexados (los `eliminar` posteriores lo decrementan).
- Estructura interna `Nodo`: `bool esHoja; int nivel; Caja mbr; std::vector<Nodo*> hijos; std::vector<Resultado> entradas; Nodo* padre; uint64_t version;` — destructor recursivo, copia prohibida.

- [ ] **Step 1: Test que falla**

```cpp
static void test_esqueleto() {
    cout << "\nT2: arena e insercion basica" << endl;
    RStarTree2D<string> arbol(8, 3);
    uint32_t a = arbol.insertar(1.0, 2.0, "hola");
    uint32_t b = arbol.insertar(3.0, 4.0, "mundo");
    CHECK(a == 0 && b == 1, "idx consecutivos desde 0");
    CHECK(arbol.tamano() == 2, "tamano 2");
    CHECK(arbol.dato(a) == "hola" && arbol.dato(b) == "mundo", "dato() recupera payload");
    bool exploto = false;
    try { RStarTree2D<int> malo(8, 7); } catch (const std::invalid_argument&) { exploto = true; }
    CHECK(exploto, "ctor valida 2 <= m <= M/2");
}
```

Agregar `#include <string>` si falta y llamar `test_esqueleto();` en `main`.

- [ ] **Step 2: Verificar que falla** (no compila: `RStarTree2D` no existe).
- [ ] **Step 3: Implementar** en `rstartree.hpp` (después de `Caja`):

```cpp
template <typename T>
class RStarTree2D {
public:
    struct Resultado { double x, y; uint32_t idx; };

    explicit RStarTree2D(int M = 1200, int m = 480) : M_(M), m_(m) {
        if (m < 2 || m > M / 2) throw std::invalid_argument("m debe cumplir 2 <= m <= M/2");
    }
    ~RStarTree2D() { delete raiz_; }
    RStarTree2D(const RStarTree2D&) = delete;
    RStarTree2D& operator=(const RStarTree2D&) = delete;

    uint32_t insertar(double x, double y, T dato) {
        arena_.push_back(std::move(dato));
        uint32_t idx = (uint32_t)(arena_.size() - 1);
        insertarEntrada({x, y, idx});
        n_puntos_++;
        return idx;
    }
    const T& dato(uint32_t idx) const { return arena_[idx]; }
    T& dato(uint32_t idx) { return arena_[idx]; }
    size_t tamano() const { return n_puntos_; }

private:
    struct Nodo {
        bool esHoja;
        int nivel = 0;                 // 0 = hoja
        Caja mbr;
        std::vector<Nodo*> hijos;      // solo internos
        std::vector<Resultado> entradas; // solo hojas
        Nodo* padre = nullptr;
        uint64_t version = 0;          // para caches externos (grupos)
        explicit Nodo(bool hoja) : esHoja(hoja) {}
        ~Nodo() { for (Nodo* h : hijos) delete h; }
        Nodo(const Nodo&) = delete;
        Nodo& operator=(const Nodo&) = delete;
    };

    int M_, m_;
    std::vector<T> arena_;
    Nodo* raiz_ = nullptr;
    size_t n_puntos_ = 0;
    uint64_t contadorVersion_ = 0;
    std::vector<bool> nivelReinsertado_;   // OT1: un reinsert por nivel por operacion

    void tocar(Nodo* hoja) { hoja->version = ++contadorVersion_; }

    // En esta task: version minima sin split (se reemplaza en Task 4)
    void insertarEntrada(const Resultado& e) {
        if (raiz_ == nullptr) raiz_ = new Nodo(true);
        raiz_->entradas.push_back(e);
        raiz_->mbr.estirar(e.x, e.y);
        tocar(raiz_);
    }
};
```

- [ ] **Step 4: Verificar que pasa**: `make test`.
- [ ] **Step 5: Commit**: `git commit -m "rstarLib: esqueleto RStarTree2D con arena y ctor validado"`

---

### Task 3: `buscarRango` + `recorrer`

**Files:**
- Modify: `rstarLib/rstartree.hpp`
- Modify: `rstarLib/tests/test_rstarlib.cpp`

**Interfaces:**
- Produces: `std::vector<Resultado> buscarRango(const Caja& bbox) const` y `void recorrer(const std::function<void(const Resultado&)>& visita) const`.

- [ ] **Step 1: Test que falla**

```cpp
static void test_rango_y_recorrido() {
    cout << "\nT3: buscarRango y recorrer" << endl;
    RStarTree2D<int> arbol(8, 3);
    for (int i = 0; i < 6; i++) arbol.insertar(i * 1.0, i * 1.0, i * 10);
    auto res = arbol.buscarRango(Caja(1.5, 1.5, 4.5, 4.5));
    CHECK(res.size() == 3, "rango [1.5,4.5]^2 devuelve 3 (los puntos 2,3,4)");
    bool datosOk = true;
    for (const auto& r : res) if (arbol.dato(r.idx) != (int)(r.x) * 10) datosOk = false;
    CHECK(datosOk, "idx de resultados apunta al dato correcto");
    int visitados = 0;
    arbol.recorrer([&](const RStarTree2D<int>::Resultado&) { visitados++; });
    CHECK(visitados == 6, "recorrer visita los 6 puntos");
}
```

- [ ] **Step 2: Verificar que falla.**
- [ ] **Step 3: Implementar** (métodos públicos + auxiliares privados):

```cpp
public:
    std::vector<Resultado> buscarRango(const Caja& bbox) const {
        std::vector<Resultado> res;
        rangoRec(raiz_, bbox, res);
        return res;
    }
    void recorrer(const std::function<void(const Resultado&)>& visita) const {
        recorrerRec(raiz_, visita);
    }
private:
    void rangoRec(const Nodo* n, const Caja& bbox, std::vector<Resultado>& res) const {
        if (n == nullptr || !n->mbr.interseca(bbox)) return;
        if (n->esHoja) {
            for (const auto& e : n->entradas)
                if (bbox.contiene(e.x, e.y)) res.push_back(e);
        } else {
            for (const Nodo* h : n->hijos) rangoRec(h, bbox, res);
        }
    }
    void recorrerRec(const Nodo* n, const std::function<void(const Resultado&)>& v) const {
        if (n == nullptr) return;
        if (n->esHoja) { for (const auto& e : n->entradas) v(e); }
        else for (const Nodo* h : n->hijos) recorrerRec(h, v);
    }
```

- [ ] **Step 4: Verificar que pasa.**
- [ ] **Step 5: Commit**: `git commit -m "rstarLib: buscarRango y recorrer"`

---

### Task 4: Maquinaria de inserción completa (porte de Beckmann)

**Files:**
- Modify: `rstarLib/rstartree.hpp` (reemplaza `insertarEntrada` mínima)
- Modify: `rstarLib/tests/test_rstarlib.cpp`
- Referencia de porte: `rstarLib/struct/GeoCluster.cpp` (funciones: `chooseSubTree`, `hijoConMenorAmpliacionArea`, `hijoConMenorAmpliacionOverlap`, `overflowTreatment`, `reinsertar`, `elegirSplit`, `split`, `ajustarMBRHaciaArriba`, `updateMBR`) y `GeoCluster.h` (struct `MBR`).

**Interfaces:**
- Consumes: `Nodo`, `Caja`, `tocar()` de Tasks 1-2.
- Produces: `insertarEntrada(const Resultado&)` con splits/reinserts correctos; privados `insertarSubarbol(Nodo*)`, `chooseSubTree(const Caja&, int)`, `overflowTreatment(Nodo*)`, `reinsertar(Nodo*)`, `split(Nodo*)`, `ajustarHaciaArriba(Nodo*)`, `actualizarMBR(Nodo*)`, struct interno `SeleccionSplit { int eje; std::vector<int> orden; int tamGrupo1; }` y `SeleccionSplit elegirSplit(const std::vector<Caja>&) const`.

**Tabla de porte** (misma lógica, estos renames):

| GeoCluster.cpp | rstartree.hpp |
|---|---|
| `MBR` / `m_minp[d]` / `m_maxp[d]` | `Caja` / `lo[d]` / `hi[d]` |
| `stretch` / `margin` / `area` / `overlap` | `estirar` / `margen` / `area` / `overlap` |
| `PointID {id, lat, lon, clusters}` | `Resultado {x, y, idx}` |
| `mbrDePunto(p)` | `Caja(e.x, e.y, e.x, e.y)` |
| `MAX_PUNTOS_POR_NODO` / `MIN_PUNTOS_POR_NODO` | `M_` / `m_` |
| `PORCENTAJE_PARA_HACER_REINSERT` (0.3) | constante local `0.3` (p = 30% de M, paper 4.3) |
| `niveles_reinsert` (unordered_map) | `nivelReinsertado_` (vector<bool>, índice = nivel) |
| `raiz`, `puntos`, `hijos`, `m_nivel` | `raiz_`, `entradas`, `hijos`, `nivel` |
| `updateMBR` | `actualizarMBR` |
| `ajustarMBRHaciaArriba` | `ajustarHaciaArriba` |
| couts de error (`cerr`) | eliminarlos (librería silenciosa; condiciones imposibles → nada) |

Reglas del porte que NO cambian: overlap real contra hermanos con optimización de 32 candidatos (hijos de nivel 1); CSA por suma S de márgenes de todas las distribuciones (ambos órdenes lower/upper); CSI por mínimo overlap, empate por área; distribuciones grupo1 = m..E-m; close reinsert (reinsertar en orden de distancia CRECIENTE al centro del MBR), p = 30% de M; OT1 un reinsert por nivel por operación; split de raíz crea nueva raíz con `nivel = viejo + 1`.

Cambios estructurales respecto al original:
1. `insertarEntrada(e)`: reemplaza `insertar(Punto,...)`. Al inicio de cada **operación externa** (`insertar` público y, en Task 6, `eliminar`) se limpia `nivelReinsertado_.assign(64, false)`. `insertarEntrada` NO la limpia (los reinserts internos la comparten).
2. Toda mutación de una hoja llama `tocar(hoja)`.
3. `split` de hoja: ambas mitades reciben `tocar()`.

- [ ] **Step 1: Tests que fallan — conformidad estructural** (adaptación de `rstarLib/struct/tests/test_rstar.cpp`):

```cpp
struct Stats {
    int minProf = 1 << 30, maxProf = -1;
    int violMax = 0, violMin = 0, violMBR = 0;
};
// recorre via API publica de test: se agrega a RStarTree2D un metodo de
// inspeccion (necesario tambien para grupos en Task 7):
//   void inspeccionar(const std::function<void(bool esHoja, int nivel, int prof,
//                     const Caja& mbr, size_t nEntradas, size_t nHijos, bool esRaiz)>&) const;
static void test_invariantes() {
    cout << "\nT4: invariantes estructurales (500 puntos, M=8, m=3)" << endl;
    RStarTree2D<int> arbol(8, 3);
    int id = 0;
    for (int i = 0; i < 25; i++)
        for (int j = 0; j < 20; j++)
            arbol.insertar(i * 0.01, j * 0.01, id++);

    Stats s;
    arbol.inspeccionar([&](bool esHoja, int, int prof, const Caja&, size_t nE, size_t nH, bool esRaiz) {
        size_t cuenta = esHoja ? nE : nH;
        if (cuenta > 8) s.violMax++;
        if (!esRaiz && cuenta < 3) s.violMin++;
        if (esRaiz && !esHoja && nH < 2) s.violMin++;
        if (esHoja) { s.minProf = min(s.minProf, prof); s.maxProf = max(s.maxProf, prof); }
    });
    CHECK(s.violMax == 0, "ningun nodo supera M");
    CHECK(s.violMin == 0, "todo nodo no-raiz >= m");
    CHECK(s.minProf == s.maxProf, "hojas al mismo nivel");

    auto todo = arbol.buscarRango(Caja(-1, -1, 1, 1));
    vector<int> conteo(500, 0);
    for (auto& r : todo) conteo[arbol.dato(r.idx)]++;
    int perdidos = 0, duplicados = 0;
    for (int c : conteo) { if (c == 0) perdidos++; if (c > 1) duplicados++; }
    CHECK(perdidos == 0 && duplicados == 0, "sin perdida ni duplicados tras splits/reinserts");
    CHECK(arbol.tamano() == 500, "tamano correcto");
}
```

`inspeccionar` (público, para tests y depuración — recorre con profundidad):

```cpp
public:
    void inspeccionar(const std::function<void(bool, int, int, const Caja&, size_t, size_t, bool)>& f) const {
        inspeccionarRec(raiz_, 0, true, f);
    }
private:
    void inspeccionarRec(const Nodo* n, int prof, bool esRaiz,
                         const std::function<void(bool, int, int, const Caja&, size_t, size_t, bool)>& f) const {
        if (n == nullptr) return;
        f(n->esHoja, n->nivel, prof, n->mbr, n->entradas.size(), n->hijos.size(), esRaiz);
        for (const Nodo* h : n->hijos) inspeccionarRec(h, prof + 1, false, f);
    }
```

- [ ] **Step 2: Verificar que falla** (con la `insertarEntrada` mínima de Task 2, la raíz acumula 500 entradas → viola M).
- [ ] **Step 3: Portar la maquinaria** siguiendo la tabla. Además `insertar` público queda:

```cpp
    uint32_t insertar(double x, double y, T dato) {
        arena_.push_back(std::move(dato));
        uint32_t idx = (uint32_t)(arena_.size() - 1);
        nivelReinsertado_.assign(64, false);   // OT1: por operacion externa
        insertarEntrada({x, y, idx});
        n_puntos_++;
        return idx;
    }
```

y `insertarEntrada` / `insertarSubarbol`:

```cpp
    void insertarEntrada(const Resultado& e) {
        if (raiz_ == nullptr) raiz_ = new Nodo(true);
        Caja ce(e.x, e.y, e.x, e.y);
        Nodo* hoja = chooseSubTree(ce, 0);
        hoja->entradas.push_back(e);
        tocar(hoja);
        ajustarHaciaArriba(hoja);
        if ((int)hoja->entradas.size() > M_) overflowTreatment(hoja);
    }
    void insertarSubarbol(Nodo* sub) {
        Nodo* n = chooseSubTree(sub->mbr, sub->nivel + 1);
        sub->padre = n;
        n->hijos.push_back(sub);
        ajustarHaciaArriba(n);
        if ((int)n->hijos.size() > M_) overflowTreatment(n);
    }
```

El resto (`chooseSubTree`, `hijoConMenorAmpliacionArea/Overlap`, `overflowTreatment`, `reinsertar`, `elegirSplit`, `split`, `ajustarHaciaArriba`, `actualizarMBR`) se porta 1:1 desde `rstarLib/struct/GeoCluster.cpp` aplicando la tabla. En `reinsertar` para hojas, las entradas quitadas se reinsertan con `insertarEntrada({e.x, e.y, e.idx})` (no hay arena que tocar). En `split` de hoja: `tocar(N); tocar(nuevo);`.

- [ ] **Step 4: Verificar que pasa** (`make test`, todo verde incluido T1-T3).
- [ ] **Step 5: Commit**: `git commit -m "rstarLib: maquinaria de insercion Beckmann portada y genericizada"`

---

### Task 5: `kVecinos` (branch-and-bound)

**Files:**
- Modify: `rstarLib/rstartree.hpp`, `rstarLib/tests/test_rstarlib.cpp`

**Interfaces:**
- Produces: `std::vector<Resultado> kVecinos(double x, double y, int k) const` — ordenados de más cercano a más lejano.

- [ ] **Step 1: Test que falla — comparación contra fuerza bruta**

```cpp
static void test_knn() {
    cout << "\nT5: kVecinos vs fuerza bruta" << endl;
    RStarTree2D<int> arbol(8, 3);
    vector<pair<double,double>> pts;
    unsigned semilla = 12345;
    auto rnd = [&]() { semilla = semilla * 1103515245 + 12345; return (semilla >> 8 % 1000) % 1000 / 1000.0; };
    for (int i = 0; i < 300; i++) {
        double x = rnd(), y = rnd();
        pts.push_back({x, y});
        arbol.insertar(x, y, i);
    }
    double qx = 0.5, qy = 0.5;
    auto knn = arbol.kVecinos(qx, qy, 10);
    // fuerza bruta
    vector<pair<double,int>> fb;
    for (int i = 0; i < 300; i++) {
        double dx = pts[i].first - qx, dy = pts[i].second - qy;
        fb.push_back({dx*dx + dy*dy, i});
    }
    sort(fb.begin(), fb.end());
    CHECK(knn.size() == 10, "devuelve k resultados");
    bool iguales = true;
    for (int i = 0; i < 10; i++)
        if (arbol.dato(knn[i].idx) != fb[i].second) iguales = false;
    CHECK(iguales, "los 10 vecinos coinciden con fuerza bruta, en orden");
    CHECK(arbol.kVecinos(qx, qy, 0).empty(), "k=0 devuelve vacio");
    auto todos = arbol.kVecinos(qx, qy, 999);
    CHECK(todos.size() == 300, "k > n devuelve todos");
}
```

- [ ] **Step 2: Verificar que falla.**
- [ ] **Step 3: Implementar**

```cpp
public:
    std::vector<Resultado> kVecinos(double x, double y, int k) const {
        std::vector<Resultado> res;
        if (raiz_ == nullptr || k <= 0) return res;
        // cola de nodos por distancia minima (best-first)
        using ItemN = std::pair<double, const Nodo*>;
        auto cmpN = [](const ItemN& a, const ItemN& b) { return a.first > b.first; };
        std::priority_queue<ItemN, std::vector<ItemN>, decltype(cmpN)> nodos(cmpN);
        nodos.push({raiz_->mbr.dist2A(x, y), raiz_});
        // mejores k hallados: max-heap por distancia
        using ItemP = std::pair<double, Resultado>;
        auto cmpP = [](const ItemP& a, const ItemP& b) { return a.first < b.first; };
        std::priority_queue<ItemP, std::vector<ItemP>, decltype(cmpP)> mejores(cmpP);

        while (!nodos.empty()) {
            auto [d2, n] = nodos.top(); nodos.pop();
            if ((int)mejores.size() == k && d2 > mejores.top().first) break; // poda
            if (n->esHoja) {
                for (const auto& e : n->entradas) {
                    double dx = e.x - x, dy = e.y - y, dd = dx * dx + dy * dy;
                    if ((int)mejores.size() < k) mejores.push({dd, e});
                    else if (dd < mejores.top().first) { mejores.pop(); mejores.push({dd, e}); }
                }
            } else {
                for (const Nodo* h : n->hijos) nodos.push({h->mbr.dist2A(x, y), h});
            }
        }
        res.resize(mejores.size());
        for (int i = (int)mejores.size() - 1; i >= 0; i--) { res[i] = mejores.top().second; mejores.pop(); }
        return res;
    }
```

- [ ] **Step 4: Verificar que pasa.**
- [ ] **Step 5: Commit**: `git commit -m "rstarLib: kVecinos best-first con poda"`

---

### Task 6: `eliminar` con condensación

**Files:**
- Modify: `rstarLib/rstartree.hpp`, `rstarLib/tests/test_rstarlib.cpp`

**Interfaces:**
- Produces: `bool eliminar(double x, double y, const std::function<bool(const T&)>& coincide)` — quita la PRIMERA entrada en (x,y) cuyo dato cumple el predicado. La arena conserva el dato (tombstone documentado: el idx deja de estar en el índice espacial pero `dato(idx)` sigue siendo válido).

- [ ] **Step 1: Tests que fallan**

```cpp
static void test_eliminar() {
    cout << "\nT6: eliminar con condensacion" << endl;
    RStarTree2D<int> arbol(8, 3);
    int id = 0;
    for (int i = 0; i < 25; i++)
        for (int j = 0; j < 20; j++)
            arbol.insertar(i * 0.01, j * 0.01, id++);

    // borrar 200 puntos (columnas j pares de las primeras 10 filas... usar datos conocidos)
    int borrados = 0;
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 20; j++)
            if (arbol.eliminar(i * 0.01, j * 0.01, [](const int&) { return true; })) borrados++;
    CHECK(borrados == 200, "borro exactamente 200");
    CHECK(arbol.tamano() == 300, "tamano 300 tras borrar");
    CHECK(!arbol.eliminar(9.9, 9.9, [](const int&) { return true; }), "punto inexistente devuelve false");

    auto resto = arbol.buscarRango(Caja(-1, -1, 1, 1));
    CHECK(resto.size() == 300, "quedan 300 en el indice");
    bool ningunBorrado = true;
    for (auto& r : resto) if (arbol.dato(r.idx) < 200) ningunBorrado = false;
    CHECK(ningunBorrado, "ninguno de los borrados aparece");

    // invariantes tras condensacion
    Stats s;
    arbol.inspeccionar([&](bool esHoja, int, int prof, const Caja&, size_t nE, size_t nH, bool esRaiz) {
        size_t cuenta = esHoja ? nE : nH;
        if (cuenta > 8) s.violMax++;
        if (!esRaiz && cuenta < 3) s.violMin++;
        if (esHoja) { s.minProf = min(s.minProf, prof); s.maxProf = max(s.maxProf, prof); }
    });
    CHECK(s.violMax == 0 && s.violMin == 0, "invariantes m/M se mantienen");
    CHECK(s.minProf == s.maxProf, "hojas al mismo nivel tras borrar");
}
```

- [ ] **Step 2: Verificar que falla.**
- [ ] **Step 3: Implementar**

```cpp
public:
    bool eliminar(double x, double y, const std::function<bool(const T&)>& coincide) {
        if (raiz_ == nullptr) return false;
        Nodo* hoja = nullptr;
        int pos = -1;
        buscarEntrada(raiz_, x, y, coincide, hoja, pos);
        if (hoja == nullptr) return false;
        hoja->entradas.erase(hoja->entradas.begin() + pos);
        tocar(hoja);
        n_puntos_--;
        nivelReinsertado_.assign(64, false);
        condensar(hoja);
        // raiz interna con un solo hijo: acortar el arbol
        while (raiz_ != nullptr && !raiz_->esHoja && raiz_->hijos.size() == 1) {
            Nodo* h = raiz_->hijos[0];
            raiz_->hijos.clear();
            delete raiz_;
            raiz_ = h;
            h->padre = nullptr;
        }
        return true;
    }
private:
    void buscarEntrada(Nodo* n, double x, double y,
                       const std::function<bool(const T&)>& coincide,
                       Nodo*& hoja, int& pos) {
        if (n == nullptr || hoja != nullptr || !n->mbr.contiene(x, y)) return;
        if (n->esHoja) {
            for (size_t i = 0; i < n->entradas.size(); i++) {
                const auto& e = n->entradas[i];
                if (e.x == x && e.y == y && coincide(arena_[e.idx])) {
                    hoja = n; pos = (int)i; return;
                }
            }
        } else {
            for (Nodo* h : n->hijos) buscarEntrada(h, x, y, coincide, hoja, pos);
        }
    }
    void condensar(Nodo* n) {
        std::vector<Resultado> huerfanas;
        std::vector<Nodo*> huerfanos;
        Nodo* actual = n;
        while (actual != raiz_) {
            Nodo* padre = actual->padre;
            size_t cuenta = actual->esHoja ? actual->entradas.size() : actual->hijos.size();
            if (cuenta < (size_t)m_) {
                padre->hijos.erase(std::find(padre->hijos.begin(), padre->hijos.end(), actual));
                if (actual->esHoja) {
                    huerfanas.insert(huerfanas.end(), actual->entradas.begin(), actual->entradas.end());
                    actual->entradas.clear();
                } else {
                    huerfanos.insert(huerfanos.end(), actual->hijos.begin(), actual->hijos.end());
                    actual->hijos.clear();
                }
                delete actual;
            } else {
                actualizarMBR(actual);
            }
            actual = padre;
        }
        actualizarMBR(raiz_);
        for (const auto& e : huerfanas) insertarEntrada(e);
        for (Nodo* s : huerfanos) insertarSubarbol(s);
    }
```

- [ ] **Step 4: Verificar que pasa** (todos los tests, T1-T6).
- [ ] **Step 5: Commit**: `git commit -m "rstarLib: eliminar con condensacion y reinsercion de huerfanos"`

---

### Task 7: Vistas de hojas + versiones (infraestructura para grupos)

**Files:**
- Modify: `rstarLib/rstartree.hpp`, `rstarLib/tests/test_rstarlib.cpp`

**Interfaces:**
- Produces:
```cpp
struct HojaVista {
    uintptr_t clave;                        // identidad de la hoja
    uint64_t version;                       // cambia con CADA mutacion de la hoja
    const Caja& mbr;
    const std::vector<Resultado>& entradas;
};
void visitarHojas(const std::function<void(const HojaVista&)>& f) const;
void visitarHojasEnRango(const Caja& bbox, const std::function<void(const HojaVista&)>& f) const;
```
(`HojaVista` es struct público anidado en `RStarTree2D<T>`.)

- [ ] **Step 1: Test que falla**

```cpp
static void test_hojas_version() {
    cout << "\nT7: vistas de hojas y versiones" << endl;
    RStarTree2D<int> arbol(8, 3);
    for (int i = 0; i < 50; i++) arbol.insertar(i * 0.1, i * 0.1, i);
    size_t total = 0; int hojas = 0;
    arbol.visitarHojas([&](const RStarTree2D<int>::HojaVista& h) { hojas++; total += h.entradas.size(); });
    CHECK(total == 50, "las hojas contienen los 50 puntos");
    CHECK(hojas > 1, "hubo splits (mas de una hoja)");

    int enRango = 0;
    arbol.visitarHojasEnRango(Caja(0, 0, 1.0, 1.0), [&](const auto& h) { enRango++; });
    CHECK(enRango >= 1 && enRango <= hojas, "visitarHojasEnRango filtra por bbox");

    // version cambia al mutar
    uint64_t vAntes = 0; uintptr_t clave0 = 0;
    arbol.visitarHojas([&](const auto& h) { if (clave0 == 0) { clave0 = h.clave; vAntes = h.version; } });
    arbol.insertar(0.01, 0.01, 999);   // cae en la primera hoja
    uint64_t vDespues = vAntes;
    arbol.visitarHojas([&](const auto& h) { if (h.clave == clave0) vDespues = h.version; });
    CHECK(vDespues != vAntes, "la version de la hoja mutada cambio");
}
```

- [ ] **Step 2: Verificar que falla.**
- [ ] **Step 3: Implementar**

```cpp
public:
    struct HojaVista {
        uintptr_t clave;
        uint64_t version;
        const Caja& mbr;
        const std::vector<Resultado>& entradas;
    };
    void visitarHojas(const std::function<void(const HojaVista&)>& f) const {
        visitarHojasRec(raiz_, nullptr, f);
    }
    void visitarHojasEnRango(const Caja& bbox, const std::function<void(const HojaVista&)>& f) const {
        visitarHojasRec(raiz_, &bbox, f);
    }
private:
    void visitarHojasRec(const Nodo* n, const Caja* filtro,
                         const std::function<void(const HojaVista&)>& f) const {
        if (n == nullptr) return;
        if (filtro != nullptr && !n->mbr.interseca(*filtro)) return;
        if (n->esHoja) {
            f(HojaVista{(uintptr_t)n, n->version, n->mbr, n->entradas});
        } else {
            for (const Nodo* h : n->hijos) visitarHojasRec(h, filtro, f);
        }
    }
```

Verificar también que `tocar()` se llama en TODAS las mutaciones de hoja: inserción, split (ambas mitades), reinsert (hoja origen), eliminar. Si falta alguna, agregarla.

- [ ] **Step 4: Verificar que pasa.**
- [ ] **Step 5: Commit**: `git commit -m "rstarLib: vistas de hojas con clave y version para caches externos"`

---

### Task 8: `IndicePorId`

**Files:**
- Create: `rstarLib/indice_por_id.hpp` (reemplaza el placeholder)
- Modify: `rstarLib/tests/test_rstarlib.cpp` (agregar `#include "../indice_por_id.hpp"`)

**Interfaces:**
- Produces:
```cpp
template <typename T, typename Id>
class IndicePorId {
public:
    IndicePorId(const RStarTree2D<T>& arbol, std::function<Id(const T&)> idDe);
    std::optional<uint32_t> buscar(const Id& id) const;
    void agregar(const T& dato, uint32_t idx);   // para inserciones posteriores
    size_t tamano() const;
};
```

- [ ] **Step 1: Test que falla**

```cpp
struct Registro { int id; double valor; };

static void test_indice_por_id() {
    cout << "\nT8: IndicePorId" << endl;
    RStarTree2D<Registro> arbol(8, 3);
    for (int i = 0; i < 100; i++)
        arbol.insertar(i * 0.01, i * 0.02, Registro{1000 + i, i * 1.5});

    IndicePorId<Registro, int> indice(arbol, [](const Registro& r) { return r.id; });
    CHECK(indice.tamano() == 100, "indexo los 100");
    auto idx = indice.buscar(1042);
    CHECK(idx.has_value() && arbol.dato(*idx).id == 1042, "buscar(1042) llega al registro correcto");
    CHECK(!indice.buscar(9999).has_value(), "id inexistente devuelve nullopt");

    uint32_t nuevo = arbol.insertar(0.5, 0.5, Registro{2000, 7.0});
    indice.agregar(arbol.dato(nuevo), nuevo);
    CHECK(indice.buscar(2000).has_value(), "agregar() indexa insercion posterior");
}
```

- [ ] **Step 2: Verificar que falla.**
- [ ] **Step 3: Implementar** `indice_por_id.hpp`:

```cpp
#pragma once
// Indice opcional id externo -> posicion en la arena. Ver DISENO.md seccion 2.
#include "rstartree.hpp"
#include <unordered_map>
#include <optional>

template <typename T, typename Id>
class IndicePorId {
public:
    IndicePorId(const RStarTree2D<T>& arbol, std::function<Id(const T&)> idDe)
        : idDe_(std::move(idDe)) {
        arbol.recorrer([&](const typename RStarTree2D<T>::Resultado& r) {
            mapa_[idDe_(arbol.dato(r.idx))] = r.idx;
        });
    }
    std::optional<uint32_t> buscar(const Id& id) const {
        auto it = mapa_.find(id);
        if (it == mapa_.end()) return std::nullopt;
        return it->second;
    }
    void agregar(const T& dato, uint32_t idx) { mapa_[idDe_(dato)] = idx; }
    size_t tamano() const { return mapa_.size(); }

private:
    std::function<Id(const T&)> idDe_;
    std::unordered_map<Id, uint32_t> mapa_;
};
```

- [ ] **Step 4: Verificar que pasa.**
- [ ] **Step 5: Commit**: `git commit -m "rstarLib: IndicePorId opcional (id externo -> idx de arena)"`

---

### Task 9: `GruposPorHoja` — construir, `gruposEnRango`, invalidación perezosa

**Files:**
- Create: `rstarLib/grupos_por_hoja.hpp` (reemplaza el placeholder)
- Modify: `rstarLib/tests/test_rstarlib.cpp`

**Interfaces:**
- Consumes: `HojaVista`, `visitarHojas`, `visitarHojasEnRango` (Task 7).
- Produces:
```cpp
template <typename T, typename Etiqueta>   // Etiqueta necesita operator< (se usa std::map)
class GruposPorHoja {
public:
    GruposPorHoja(const RStarTree2D<T>& arbol,
                  std::function<Etiqueta(const T&)> etiquetaDe,
                  std::function<std::vector<double>(const T&)> caracteristicasDe);
    void construir();   // pasada completa post-carga
    // grupos con >= 2 miembros dentro del bbox, fusionados por etiqueta entre hojas
    std::vector<std::vector<uint32_t>> gruposEnRango(const Caja& bbox);
    std::vector<uint32_t> nSimilares(const Caja& bbox, uint32_t idxReferencia, int n); // Task 10
};
```
- Cache interno: `struct Grupo { Etiqueta etiqueta; std::vector<typename RStarTree2D<T>::Resultado> miembros; std::vector<double> centroide; };` y `std::unordered_map<uintptr_t, CacheHoja>` con `CacheHoja { uint64_t version; std::vector<Grupo> grupos; }`. Si la versión de la hoja difiere de la cacheada (o no existe), se rearma SOLO esa hoja (invalidación perezosa). El centroide promedia `caracteristicasDe` de los miembros.

- [ ] **Step 1: Tests que fallan**

```cpp
struct Viaje { int id; int etiqueta; vector<double> pcs; };

static void test_grupos() {
    cout << "\nT9: GruposPorHoja y invalidacion" << endl;
    RStarTree2D<Viaje> arbol(8, 3);
    // 3 etiquetas, 60 puntos: etiqueta = i % 3
    for (int i = 0; i < 60; i++)
        arbol.insertar(i * 0.01, 0.0, Viaje{i, i % 3, {(double)(i % 3), 0.0}});

    GruposPorHoja<Viaje, int> grupos(arbol,
        [](const Viaje& v) { return v.etiqueta; },
        [](const Viaje& v) { return v.pcs; });
    grupos.construir();

    auto gs = arbol.tamano() ? grupos.gruposEnRango(Caja(-1, -1, 1, 1)) : vector<vector<uint32_t>>{};
    CHECK(gs.size() == 3, "3 grupos fusionados por etiqueta en rango global");
    size_t total = 0; for (auto& g : gs) total += g.size();
    CHECK(total == 60, "los 60 puntos repartidos en los grupos");
    bool coherentes = true;
    for (auto& g : gs) {
        int et = arbol.dato(g[0]).etiqueta;
        for (uint32_t idx : g) if (arbol.dato(idx).etiqueta != et) coherentes = false;
    }
    CHECK(coherentes, "cada grupo tiene una sola etiqueta");

    // bbox parcial: solo puntos con x <= 0.095 (indices 0..9)
    auto parcial = grupos.gruposEnRango(Caja(-0.01, -0.01, 0.095, 0.01));
    size_t enParcial = 0; for (auto& g : parcial) enParcial += g.size();
    CHECK(enParcial == 10, "bbox parcial filtra miembros fuera del rango");

    // invalidacion: insertar tras construir y consultar de nuevo
    arbol.insertar(0.005, 0.0, Viaje{999, 0, {0.0, 0.0}});
    auto gs2 = grupos.gruposEnRango(Caja(-1, -1, 1, 1));
    size_t total2 = 0; for (auto& g : gs2) total2 += g.size();
    CHECK(total2 == 61, "grupo rearmado perezosamente tras insercion");
}
```

- [ ] **Step 2: Verificar que falla.**
- [ ] **Step 3: Implementar** `grupos_por_hoja.hpp`:

```cpp
#pragma once
// Precomputacion por hoja: cajones por etiqueta + centroides. Ver DISENO.md 2 y 4.
#include "rstartree.hpp"
#include <map>
#include <unordered_map>

template <typename T, typename Etiqueta>
class GruposPorHoja {
public:
    using Res = typename RStarTree2D<T>::Resultado;
    struct Grupo {
        Etiqueta etiqueta;
        std::vector<Res> miembros;
        std::vector<double> centroide;
    };

    GruposPorHoja(const RStarTree2D<T>& arbol,
                  std::function<Etiqueta(const T&)> etiquetaDe,
                  std::function<std::vector<double>(const T&)> caracteristicasDe)
        : arbol_(arbol), etiquetaDe_(std::move(etiquetaDe)),
          caracteristicasDe_(std::move(caracteristicasDe)) {}

    void construir() {
        cache_.clear();
        arbol_.visitarHojas([&](const typename RStarTree2D<T>::HojaVista& h) {
            cache_[h.clave] = armarHoja(h);
        });
    }

    std::vector<std::vector<uint32_t>> gruposEnRango(const Caja& bbox) {
        std::map<Etiqueta, std::vector<uint32_t>> fusion;
        arbol_.visitarHojasEnRango(bbox, [&](const auto& h) {
            const CacheHoja& c = obtener(h);
            for (const Grupo& g : c.grupos)
                for (const Res& m : g.miembros)
                    if (bbox.contiene(m.x, m.y))
                        fusion[g.etiqueta].push_back(m.idx);
        });
        std::vector<std::vector<uint32_t>> res;
        for (auto& [et, v] : fusion)
            if (v.size() >= 2) res.push_back(std::move(v));
        return res;
    }

private:
    struct CacheHoja { uint64_t version; std::vector<Grupo> grupos; };

    CacheHoja armarHoja(const typename RStarTree2D<T>::HojaVista& h) {
        std::map<Etiqueta, Grupo> cajones;
        for (const Res& e : h.entradas) {
            const T& d = arbol_.dato(e.idx);
            Grupo& g = cajones[etiquetaDe_(d)];
            g.etiqueta = etiquetaDe_(d);
            g.miembros.push_back(e);
        }
        CacheHoja c;
        c.version = h.version;
        for (auto& [et, g] : cajones) {
            // centroide de caracteristicas
            std::vector<double> suma;
            for (const Res& m : g.miembros) {
                auto v = caracteristicasDe_(arbol_.dato(m.idx));
                if (suma.empty()) suma.assign(v.size(), 0.0);
                for (size_t i = 0; i < v.size() && i < suma.size(); i++) suma[i] += v[i];
            }
            for (double& s : suma) s /= (double)g.miembros.size();
            g.centroide = std::move(suma);
            c.grupos.push_back(std::move(g));
        }
        return c;
    }

    // invalidacion perezosa: version distinta => rearmar esa hoja
    const CacheHoja& obtener(const typename RStarTree2D<T>::HojaVista& h) {
        auto it = cache_.find(h.clave);
        if (it == cache_.end() || it->second.version != h.version)
            it = cache_.insert_or_assign(h.clave, armarHoja(h)).first;
        return it->second;
    }

    const RStarTree2D<T>& arbol_;
    std::function<Etiqueta(const T&)> etiquetaDe_;
    std::function<std::vector<double>(const T&)> caracteristicasDe_;
    std::unordered_map<uintptr_t, CacheHoja> cache_;
};
```

- [ ] **Step 4: Verificar que pasa.**
- [ ] **Step 5: Commit**: `git commit -m "rstarLib: GruposPorHoja con construccion post-carga e invalidacion perezosa"`

---

### Task 10: `nSimilares` (consulta estrella 1)

**Files:**
- Modify: `rstarLib/grupos_por_hoja.hpp`, `rstarLib/tests/test_rstarlib.cpp`

**Interfaces:**
- Produces: `std::vector<uint32_t> nSimilares(const Caja& bbox, uint32_t idxReferencia, int n)` en `GruposPorHoja`. Nota de diseño: recibe el **idx** del referente (obtenido típicamente vía `IndicePorId`) — permite excluirlo de los resultados y funciona aunque esté FUERA del bbox. Prioridad: (1) miembros del grupo con la misma etiqueta del referente, ordenados por distancia de características al referente; (2) demás grupos ordenados por distancia de su centroide a las características del referente, sus miembros también ordenados por distancia.

- [ ] **Step 1: Tests que fallan**

```cpp
static void test_n_similares() {
    cout << "\nT10: nSimilares" << endl;
    RStarTree2D<Viaje> arbol(8, 3);
    // bbox de consulta cubrira x en [0, 0.5]; el referente vive en x=2.0 (FUERA)
    // 30 puntos dentro: 10 etiqueta 0 (pcs {0}), 10 etiqueta 1 (pcs {5}), 10 etiqueta 2 (pcs {9})
    int id = 0;
    for (int e = 0; e < 3; e++)
        for (int i = 0; i < 10; i++)
            arbol.insertar(0.05 * (e * 10 + i), 0.0, Viaje{id++, e, {e * 5.0 - (e == 2 ? 1.0 : 0.0)}});
    uint32_t ref = arbol.insertar(2.0, 2.0, Viaje{999, 1, {5.0}});  // etiqueta 1, fuera del bbox

    GruposPorHoja<Viaje, int> grupos(arbol,
        [](const Viaje& v) { return v.etiqueta; },
        [](const Viaje& v) { return v.pcs; });
    grupos.construir();

    Caja bbox(-0.01, -0.01, 0.51, 0.01);
    auto sim = grupos.nSimilares(bbox, ref, 10);
    CHECK(sim.size() == 10, "devuelve n=10");
    bool todosEt1 = true;
    for (uint32_t idx : sim) if (arbol.dato(idx).etiqueta != 1) todosEt1 = false;
    CHECK(todosEt1, "prioridad 1: los 10 de la misma etiqueta del referente");
    bool sinReferente = true;
    for (uint32_t idx : sim) if (idx == ref) sinReferente = false;
    CHECK(sinReferente, "el referente no aparece en resultados");

    auto sim15 = grupos.nSimilares(bbox, ref, 15);
    CHECK(sim15.size() == 15, "n=15 completa con otros grupos");
    // los 5 extra deben ser etiqueta 0 (centroide {0} esta a dist 5 de {5};
    // etiqueta 2 con centroide {9} esta a dist 4) -> mas cercano es etiqueta 2
    int deEt2 = 0;
    for (size_t i = 10; i < 15; i++) if (arbol.dato(sim15[i]).etiqueta == 2) deEt2++;
    CHECK(deEt2 == 5, "los extra vienen del grupo con centroide mas cercano (etiqueta 2)");
}
```

- [ ] **Step 2: Verificar que falla.**
- [ ] **Step 3: Implementar** en `grupos_por_hoja.hpp` (dentro de la clase):

```cpp
public:
    std::vector<uint32_t> nSimilares(const Caja& bbox, uint32_t idxReferencia, int n) {
        std::vector<uint32_t> res;
        if (n <= 0) return res;
        const T& refDato = arbol_.dato(idxReferencia);
        Etiqueta refEt = etiquetaDe_(refDato);
        std::vector<double> refCar = caracteristicasDe_(refDato);

        // recolectar grupos fusionados por etiqueta dentro del bbox
        std::map<Etiqueta, std::vector<Res>> fusion;
        arbol_.visitarHojasEnRango(bbox, [&](const auto& h) {
            const CacheHoja& c = obtener(h);
            for (const Grupo& g : c.grupos)
                for (const Res& m : g.miembros)
                    if (bbox.contiene(m.x, m.y) && m.idx != idxReferencia)
                        fusion[g.etiqueta].push_back(m);
        });

        auto dist2Car = [&](const std::vector<double>& a, const std::vector<double>& b) {
            double s = 0; size_t k = std::min(a.size(), b.size());
            for (size_t i = 0; i < k; i++) { double d = a[i] - b[i]; s += d * d; }
            return s;
        };
        auto agregarOrdenado = [&](std::vector<Res>& miembros) {
            std::sort(miembros.begin(), miembros.end(), [&](const Res& p, const Res& q) {
                return dist2Car(caracteristicasDe_(arbol_.dato(p.idx)), refCar) <
                       dist2Car(caracteristicasDe_(arbol_.dato(q.idx)), refCar);
            });
            for (const Res& m : miembros) {
                if ((int)res.size() >= n) return;
                res.push_back(m.idx);
            }
        };

        // prioridad 1: misma etiqueta
        auto mismo = fusion.find(refEt);
        if (mismo != fusion.end()) { agregarOrdenado(mismo->second); fusion.erase(mismo); }

        // prioridad 2: demas grupos por distancia de centroide
        std::vector<std::pair<double, Etiqueta>> orden;
        for (auto& [et, miembros] : fusion) {
            std::vector<double> suma;
            for (const Res& m : miembros) {
                auto v = caracteristicasDe_(arbol_.dato(m.idx));
                if (suma.empty()) suma.assign(v.size(), 0.0);
                for (size_t i = 0; i < v.size() && i < suma.size(); i++) suma[i] += v[i];
            }
            for (double& s : suma) s /= (double)miembros.size();
            orden.push_back({dist2Car(suma, refCar), et});
        }
        std::sort(orden.begin(), orden.end());
        for (auto& [d, et] : orden) {
            if ((int)res.size() >= n) break;
            agregarOrdenado(fusion[et]);
        }
        return res;
    }
```

- [ ] **Step 4: Verificar que pasa.**
- [ ] **Step 5: Commit**: `git commit -m "rstarLib: nSimilares con prioridad por etiqueta y ranking por centroides"`

---

### Task 11: Ejemplo taxis + README

**Files:**
- Create: `rstarLib/ejemplo/ejemplo_taxis.cpp`
- Create: `rstarLib/README.md`

**Interfaces:**
- Consumes: toda la API pública de Tasks 1-10.

- [ ] **Step 1: Escribir el ejemplo** (replica el flujo del proyecto con datos sintéticos, sin dependencia de archivos):

```cpp
// Demo de rstarLib con datos estilo NYC-taxi sinteticos.
// Compilar y correr: make ejemplo
#include "../rstartree.hpp"
#include "../indice_por_id.hpp"
#include "../grupos_por_hoja.hpp"
#include <iostream>
#include <random>
using namespace std;

struct Taxi {
    int tripID;
    int etiqueta;              // cluster atributivo global (viene del pipeline Python)
    vector<double> pcs;        // componentes PCA
    double lat, lon;           // tambien en T: las consultas por id las necesitan
};

int main() {
    RStarTree2D<Taxi> arbol;             // M=1200, m=480 por defecto
    mt19937 gen(42);
    uniform_real_distribution<double> dLat(40.55, 40.95), dLon(-74.10, -73.70);
    uniform_int_distribution<int> dEt(0, 9);
    normal_distribution<double> dPc(0.0, 1.0);

    // FASE CARGA: insertar (en un caso real, ordenados por lat/long desde el .bin)
    const int N = 100000;
    for (int i = 0; i < N; i++) {
        double lat = dLat(gen), lon = dLon(gen);
        int et = dEt(gen);
        arbol.insertar(lat, lon, Taxi{i, et, {et + dPc(gen) * 0.1, dPc(gen)}, lat, lon});
    }
    cout << "Cargados " << arbol.tamano() << " puntos" << endl;

    // FASE PRECOMPUTO (lo que pedia el profesor):
    IndicePorId<Taxi, int> porId(arbol, [](const Taxi& t) { return t.tripID; });
    GruposPorHoja<Taxi, int> grupos(arbol,
        [](const Taxi& t) { return t.etiqueta; },
        [](const Taxi& t) { return t.pcs; });
    grupos.construir();
    cout << "Indice por id y grupos por hoja construidos" << endl;

    // CONSULTA 1: los 20 mas parecidos al tripID 45020 dentro de un bbox
    Caja bbox(40.70, -74.02, 40.80, -73.93);
    auto idxRef = porId.buscar(45020);
    if (idxRef) {
        auto sim = grupos.nSimilares(bbox, *idxRef, 20);
        cout << "\nConsulta 1: " << sim.size() << " similares al tripID 45020 "
             << "(etiqueta " << arbol.dato(*idxRef).etiqueta << "):" << endl;
        for (size_t i = 0; i < 5 && i < sim.size(); i++) {
            const Taxi& t = arbol.dato(sim[i]);
            cout << "  tripID " << t.tripID << " etiqueta " << t.etiqueta << endl;
        }
        cout << "  ..." << endl;
    }

    // CONSULTA 2: grupos similares entre si dentro del bbox
    auto gs = grupos.gruposEnRango(bbox);
    cout << "\nConsulta 2: " << gs.size() << " grupos en el bbox" << endl;
    for (size_t i = 0; i < 3 && i < gs.size(); i++)
        cout << "  grupo de etiqueta " << arbol.dato(gs[i][0]).etiqueta
             << ": " << gs[i].size() << " puntos" << endl;

    // EXTRAS: kNN geografico y eliminacion
    auto vecinos = arbol.kVecinos(40.7528, -73.9765, 5);
    cout << "\n5 taxis mas cercanos a Grand Central: ";
    for (auto& v : vecinos) cout << arbol.dato(v.idx).tripID << " ";
    cout << endl;

    return 0;
}
```

- [ ] **Step 2: `make ejemplo`** → corre sin errores, imprime resultados coherentes (consulta 1 devuelve 20 con la etiqueta del referente).
- [ ] **Step 3: README.md** — secciones: qué es (2 párrafos, arquitectura de DISENO.md resumida), instalación (copiar los 3 .hpp), ejemplo mínimo de 20 líneas (subset del ejemplo_taxis), API de referencia (tabla método → qué hace → complejidad), pipeline de datos recomendado (el diagrama de DISENO.md sección 5), extensiones futuras (N-D, ball tree, bulk STR). Máximo ~120 líneas.
- [ ] **Step 4: Commit**: `git commit -m "rstarLib: ejemplo taxis completo y README"`

---

### Task 12: Pipeline Python corregido

**Files:**
- Create: `rstarLib/preprocesameinto/pipeline/1_limpiar.py`
- Create: `rstarLib/preprocesameinto/pipeline/2_pca_clustering.py`
- Create: `rstarLib/preprocesameinto/pipeline/3_ordenar_a_binario.py`

Los scripts viejos copiados quedan como referencia hasta la Task 13. Los nuevos aplican los fixes conocidos: clip de negativos (NO desplazar columna), clustering GLOBAL (sin cluster geográfico), k por silhouette, orden al final, escritura binaria vectorizada.

- [ ] **Step 1: `1_limpiar.py`**

```python
"""Limpieza: coordenadas validas NYC + clip de negativos (no desplazar)."""
import sys
import pandas as pd

entrada = sys.argv[1] if len(sys.argv) > 1 else 'database/2Mpuntos.csv'
salida = sys.argv[2] if len(sys.argv) > 2 else 'database/pipeline_1_limpio.csv'

df = pd.read_csv(entrada)
print(f"Registros originales: {len(df):,}")

NYC = dict(min_lat=40.5774, max_lat=40.9176, min_lon=-74.15, max_lon=-73.7004)
mask = (
    df['pickup_latitude'].between(NYC['min_lat'], NYC['max_lat']) &
    df['pickup_longitude'].between(NYC['min_lon'], NYC['max_lon'])
)
df = df[mask].copy()
print(f"Tras filtro geografico: {len(df):,}")

tarifas = ['fare_amount', 'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge']
for col in tarifas:
    negativos = (df[col] < 0).sum()
    if negativos:
        print(f"  {col}: {negativos} negativos -> clip a 0")
    df[col] = df[col].clip(lower=0)
df['total_amount'] = df[tarifas].sum(axis=1).round(2)

df = df.dropna()
df.to_csv(salida, index=False)
print(f"Guardado {salida}: {len(df):,} registros")
```

- [ ] **Step 2: `2_pca_clustering.py`**

```python
"""PCA (k por varianza >= 90%) + clustering atributivo GLOBAL (k por silhouette)."""
import sys
import numpy as np
import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.decomposition import PCA
from sklearn.cluster import MiniBatchKMeans
from sklearn.metrics import silhouette_score

entrada = sys.argv[1] if len(sys.argv) > 1 else 'database/pipeline_1_limpio.csv'
salida = sys.argv[2] if len(sys.argv) > 2 else 'database/pipeline_2_etiquetado.csv'

df = pd.read_csv(entrada)
atributos = ['passenger_count', 'trip_distance', 'payment_type', 'fare_amount',
             'extra', 'mta_tax', 'tip_amount', 'tolls_amount', 'improvement_surcharge']
X = StandardScaler().fit_transform(df[atributos].values)

pca = PCA(n_components=0.90)          # componentes hasta 90% de varianza
Xp = pca.fit_transform(X)
print(f"PCA: {Xp.shape[1]} componentes ({pca.explained_variance_ratio_.sum()*100:.1f}% varianza)")

mejor_k, mejor_s = 2, -1.0
for k in range(2, 21):
    km = MiniBatchKMeans(n_clusters=k, random_state=42, n_init=3)
    labels = km.fit_predict(Xp)
    s = silhouette_score(Xp, labels, sample_size=min(10000, len(Xp)), random_state=42)
    print(f"  k={k}: silhouette={s:.4f}")
    if s > mejor_s:
        mejor_k, mejor_s = k, s
print(f"k elegido: {mejor_k} (silhouette {mejor_s:.4f})")

km = MiniBatchKMeans(n_clusters=mejor_k, random_state=42, n_init=3)
df_out = df[['tripID', 'pickup_latitude', 'pickup_longitude']].copy()
df_out['etiqueta'] = km.fit_predict(Xp)
for i in range(Xp.shape[1]):
    df_out[f'PC{i+1}'] = Xp[:, i].round(10)
df_out.to_csv(salida, index=False)
print(f"Guardado {salida}")
```

- [ ] **Step 3: `3_ordenar_a_binario.py`**

```python
"""Orden por (lat, lon) + exportar .bin vectorizado.
Formato: numPuntos(int32) + por punto: tripID(int32), lat(f64), lon(f64),
etiqueta(int32), numPCs(int32), PCs(f64 x numPCs)."""
import sys
import numpy as np
import pandas as pd

entrada = sys.argv[1] if len(sys.argv) > 1 else 'database/pipeline_2_etiquetado.csv'
salida = sys.argv[2] if len(sys.argv) > 2 else 'database/pipeline_3_datos.bin'

df = pd.read_csv(entrada).sort_values(['pickup_latitude', 'pickup_longitude'])
pcs = [c for c in df.columns if c.startswith('PC')]
n, npc = len(df), len(pcs)

registro = np.dtype([('id', '<i4'), ('lat', '<f8'), ('lon', '<f8'),
                     ('etiqueta', '<i4'), ('npc', '<i4'), ('pcs', '<f8', (npc,))])
arr = np.empty(n, dtype=registro)
arr['id'] = df['tripID'].to_numpy(np.int32)
arr['lat'] = df['pickup_latitude'].to_numpy()
arr['lon'] = df['pickup_longitude'].to_numpy()
arr['etiqueta'] = df['etiqueta'].to_numpy(np.int32)
arr['npc'] = npc
arr['pcs'] = df[pcs].to_numpy()

with open(salida, 'wb') as f:
    np.int32(n).tofile(f)
    arr.tofile(f)
print(f"Guardado {salida}: {n:,} registros, {npc} PCs, orden (lat, lon)")
```

- [ ] **Step 4: Prueba de humo** (con un subset para no esperar): crear `database/mini.csv` con las primeras 5000 filas de `2Mpuntos.csv` (`head -5001 database/2Mpuntos.csv > database/mini.csv`), correr los 3 scripts encadenados desde `rstarLib/`, verificar que el `.bin` resultante existe y su tamaño = `4 + n*(4+8+8+4+4+8*npc)` bytes. Borrar `mini.csv` y los intermedios tras la prueba.
- [ ] **Step 5: Commit**: `git commit -m "rstarLib: pipeline Python corregido (clip, clustering global, bin vectorizado)"`

---

### Task 13: Limpieza final de `rstarLib/`

**Files:**
- Delete: `rstarLib/struct/Rstar_ayuda/` (borradores viejos), `rstarLib/struct/geocluster` (binario), `rstarLib/bin/csv_to_binary.py` (formato incompatible, pipeline muerto), `rstarLib/preprocesameinto/5clusterizacion_geografica.py`, `6.0ordenacion_clusteres_geograficos.py`, `6.1subclusterizacion_geografica.py`, `7clusterizacion_atributiva.py`, `8analisis_optimo_subclusters.py`, `3limpiarDatos.py`, `4PCA.py`, `python2.py` (reemplazados por `pipeline/`)
- Keep: `rstarLib/struct/` (GeoCluster.h/.cpp/main.cpp/tests — referencia del porte hasta que el usuario decida), `rstarLib/preprocesameinto/AnalisisPython/` (análisis exploratorio del usuario), `rstarLib/database/`, `rstarLib/bin/README.md`

- [ ] **Step 1: Borrar los archivos listados** con `git rm` (los no trackeados como `geocluster`, con `rm`).
- [ ] **Step 2: Verificar** que `make test` y `make ejemplo` siguen verdes tras el borrado.
- [ ] **Step 3: Commit**: `git commit -m "rstarLib: limpiar copias obsoletas (borradores, pipeline viejo)"`

---

## Autorevisión del plan (hecha)

- **Cobertura del spec**: Caja/árbol/arena (Tasks 1-4) ✓, kVecinos ✓ (T5), recorrer ✓ (T3), eliminar ✓ (T6), IndicePorId ✓ (T8), GruposPorHoja + invalidación ✓ (T9), nSimilares con referente fuera del bbox ✓ (T10), gruposEnRango ✓ (T9), ejemplo + README con pipeline documentado ✓ (T11), pipeline Python ✓ (T12), limpieza ✓ (T13). Fuera de alcance (N-D, ball tree, STR) documentado en README (T11).
- **Desviación consciente del spec**: `nSimilares` recibe `uint32_t idxReferencia` en vez de `const T&` — permite excluir al referente y encaja con el flujo id→hash→idx. Anotado en Task 10.
- **Tipos consistentes**: `Resultado {x, y, idx}` usado como entrada de hoja y retorno en todas las tasks; `HojaVista` (T7) consumido por T9-T10; extractores `std::function` uniformes.
