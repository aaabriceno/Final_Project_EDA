// Tests de conformidad con el paper R*-tree (Beckmann et al. 1990, secciones 2 y 4)
// Compilar con nodos pequeños para ejercitar splits sin millones de puntos:
//   g++ -std=c++17 -DGEO_MAX_ENTRIES=8 -DGEO_MIN_ENTRIES=3 tests/test_rstar.cpp GeoCluster.cpp -o tests/test_rstar
#include "../GeoCluster.h"
#include <iostream>
#include <set>
#include <map>

static int fallos = 0;
#define CHECK(cond, msg) do { \
    if (cond) { cout << "  [OK]    " << msg << endl; } \
    else      { cout << "  [FALLA] " << msg << endl; fallos++; } \
} while (0)

static vector<double> attrs() { return {1.0, 2.0}; }

// Invariantes del R-tree (paper, seccion 2):
//  - todo nodo no-hoja tiene entre m y M hijos, salvo la raiz (>= 2 si es interna)
//  - toda hoja tiene entre m y M puntos, salvo si la raiz es hoja
//  - todas las hojas al mismo nivel
//  - el MBR de cada nodo contiene a sus hijos/puntos
struct Stats {
    int minProf = 1 << 30, maxProf = -1;
    int violacionesMax = 0, violacionesMin = 0, violacionesMBR = 0;
};

static void recorrer(Nodo* n, int prof, Stats& s, bool esRaiz) {
    if (n == nullptr) return;
    if (n->esHoja) {
        s.minProf = min(s.minProf, prof);
        s.maxProf = max(s.maxProf, prof);
        if ((int)n->puntos.size() > MAX_PUNTOS_POR_NODO) s.violacionesMax++;
        if (!esRaiz && (int)n->puntos.size() < MIN_PUNTOS_POR_NODO) s.violacionesMin++;
        for (const auto& p : n->puntos) {
            if (p.latitud  < n->mbr.m_minp[0] - 1e-12 || p.latitud  > n->mbr.m_maxp[0] + 1e-12 ||
                p.longitud < n->mbr.m_minp[1] - 1e-12 || p.longitud > n->mbr.m_maxp[1] + 1e-12)
                s.violacionesMBR++;
        }
    } else {
        if ((int)n->hijos.size() > MAX_PUNTOS_POR_NODO) s.violacionesMax++;
        if (!esRaiz && (int)n->hijos.size() < MIN_PUNTOS_POR_NODO) s.violacionesMin++;
        if (esRaiz && (int)n->hijos.size() < 2) s.violacionesMin++;
        for (Nodo* h : n->hijos) {
            if (h == nullptr) { s.violacionesMBR++; continue; }
            if (h->mbr.m_minp[0] < n->mbr.m_minp[0] - 1e-12 || h->mbr.m_minp[1] < n->mbr.m_minp[1] - 1e-12 ||
                h->mbr.m_maxp[0] > n->mbr.m_maxp[0] + 1e-12 || h->mbr.m_maxp[1] > n->mbr.m_maxp[1] + 1e-12)
                s.violacionesMBR++;
            recorrer(h, prof + 1, s, false);
        }
    }
}

// Busqueda global sin ruido de debug
static vector<Punto> buscarTodo(GeoCluster& geo, const MBR& rango) {
    vector<Punto> res;
    if (geo.obtenerRaiz() != nullptr) geo.searchRec(rango, geo.obtenerRaiz(), res);
    return res;
}

// T1 — Paper 4.2 (ChooseSplitAxis): puntos dispersos en latitud y constantes en
// longitud deben partirse por el eje latitud (suma S de margenes minima).
static void test_choose_split_axis() {
    cout << "\nT1: ChooseSplitAxis elige el eje correcto (paper 4.2 CSA)" << endl;
    GeoCluster geo;
    // gran dispersion en latitud, jitter minimo en longitud => partir por latitud
    vector<PointID> pts;
    for (int i = 0; i < 9; i++) pts.push_back(PointID(i + 1, (double)i, (i % 2) * 0.1, 0, 0));
    int eje = geo.chooseSplitAxis(pts);
    CHECK(eje == 0, "dispersion dominante en latitud => eje 0 (latitud), obtuvo eje " + to_string(eje));

    // caso espejo: dispersion dominante en longitud
    vector<PointID> pts2;
    for (int i = 0; i < 9; i++) pts2.push_back(PointID(i + 1, (i % 2) * 0.1, (double)i, 0, 0));
    int eje2 = geo.chooseSplitAxis(pts2);
    CHECK(eje2 == 1, "dispersion dominante en longitud => eje 1 (longitud), obtuvo eje " + to_string(eje2));
}

// T2 — Correctitud: un punto con id=0 es un punto valido y debe aparecer en
// los resultados de busqueda (no usarse id 0 como centinela de 'no encontrado').
static void test_id_cero() {
    cout << "\nT2: punto con id=0 aparece en busquedas" << endl;
    GeoCluster geo;
    Punto p0(0, 1.0, 1.0, attrs());
    geo.inserData(p0);
    for (int i = 1; i <= 5; i++) geo.inserData(Punto(i, 1.0 + i * 0.001, 1.0, attrs()));
    MBR rango(0.9, 0.9, 1.1, 1.1);
    vector<Punto> res = buscarTodo(geo, rango);
    bool encontrado = false;
    for (const auto& p : res) if (p.id == 0 && p.latitud == 1.0) encontrado = true;
    CHECK(encontrado, "id=0 presente en resultado de rango que lo contiene");
}

// T3 — Paper I4: los MBR de TODO el camino de insercion se ajustan. Si tras un
// split se inserta un punto lejano, el MBR de la raiz debe cubrirlo y la
// busqueda debe encontrarlo.
static void test_mbr_camino_insercion() {
    cout << "\nT3: MBR de ancestros se ajusta en cada insercion (paper I4)" << endl;
    GeoCluster geo;
    // M=8: 9 inserciones fuerzan split de la raiz-hoja => raiz interna con 2 hojas
    for (int i = 1; i <= 9; i++) geo.inserData(Punto(i, i * 0.001, i * 0.001, attrs()));
    // punto lejos del MBR actual de la raiz
    geo.inserData(Punto(100, 5.0, 5.0, attrs()));
    MBR rango(4.9, 4.9, 5.1, 5.1);
    vector<Punto> res = buscarTodo(geo, rango);
    bool encontrado = false;
    for (const auto& p : res) if (p.id == 100) encontrado = true;
    CHECK(encontrado, "punto lejano encontrado tras extenderse el MBR de la raiz");
    Nodo* raiz = geo.obtenerRaiz();
    bool cubre = raiz != nullptr &&
                 5.0 >= raiz->mbr.m_minp[0] && 5.0 <= raiz->mbr.m_maxp[0] &&
                 5.0 >= raiz->mbr.m_minp[1] && 5.0 <= raiz->mbr.m_maxp[1];
    CHECK(cubre, "MBR de la raiz cubre el punto (5,5)");
}

// T4 — Propiedades estructurales del R-tree (paper seccion 2) con 500 puntos:
// requiere split de nodos internos y propagacion (paper I3).
static void test_invariantes_estructura() {
    cout << "\nT4: invariantes estructurales con 500 puntos (paper sec. 2 + I3)" << endl;
    GeoCluster geo;
    int id = 1;
    for (int i = 0; i < 25; i++)
        for (int j = 0; j < 20; j++)
            geo.inserData(Punto(id++, i * 0.01, j * 0.01, attrs()));

    Stats s;
    recorrer(geo.obtenerRaiz(), 0, s, true);
    CHECK(s.violacionesMax == 0, "ningun nodo supera M entradas (violaciones: " + to_string(s.violacionesMax) + ")");
    CHECK(s.violacionesMin == 0, "todo nodo no-raiz tiene >= m entradas (violaciones: " + to_string(s.violacionesMin) + ")");
    CHECK(s.minProf == s.maxProf, "todas las hojas al mismo nivel (min " + to_string(s.minProf) + ", max " + to_string(s.maxProf) + ")");
    CHECK(s.violacionesMBR == 0, "MBR de cada nodo contiene a sus hijos/puntos (violaciones: " + to_string(s.violacionesMBR) + ")");
}

// T5 — Sin perdida ni duplicados: la busqueda global devuelve exactamente los
// 500 ids insertados, una sola vez cada uno (reinserts/splits no pierden datos).
static void test_sin_perdida_ni_duplicados() {
    cout << "\nT5: sin perdida ni duplicados tras splits y reinserts" << endl;
    GeoCluster geo;
    int id = 1;
    for (int i = 0; i < 25; i++)
        for (int j = 0; j < 20; j++)
            geo.inserData(Punto(id++, i * 0.01, j * 0.01, attrs()));

    MBR global(-1.0, -1.0, 1.0, 1.0);
    vector<Punto> res = buscarTodo(geo, global);
    map<int, int> conteo;
    for (const auto& p : res) conteo[p.id]++;
    int perdidos = 0, duplicados = 0;
    for (int k = 1; k <= 500; k++) {
        auto it = conteo.find(k);
        if (it == conteo.end()) perdidos++;
        else if (it->second > 1) duplicados++;
    }
    CHECK(perdidos == 0, "0 puntos perdidos (perdidos: " + to_string(perdidos) + ")");
    CHECK(duplicados == 0, "0 puntos duplicados (duplicados: " + to_string(duplicados) + ")");
}

int main() {
    cout << "=== Tests de conformidad R*-tree (paper Beckmann 1990) ===" << endl;
    cout << "M=" << MAX_PUNTOS_POR_NODO << ", m=" << MIN_PUNTOS_POR_NODO << endl;

    test_choose_split_axis();
    test_id_cero();
    test_mbr_camino_insercion();
    test_invariantes_estructura();
    test_sin_perdida_ni_duplicados();

    cout << "\n=== Resultado: " << (fallos == 0 ? "TODOS PASAN" : to_string(fallos) + " FALLOS") << " ===" << endl;
    return fallos == 0 ? 0 : 1;
}
