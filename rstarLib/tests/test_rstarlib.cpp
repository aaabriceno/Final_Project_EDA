// Tests de rstarLib. Correr: make test
#include "../rstartree.hpp"
#include "../indice_por_id.hpp"
#include "../grupos_por_hoja.hpp"
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

struct Stats {
    int minProf = 1 << 30, maxProf = -1;
    int violMax = 0, violMin = 0;
};

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

static void test_knn() {
    cout << "\nT5: kVecinos vs fuerza bruta" << endl;
    RStarTree2D<int> arbol(8, 3);
    vector<pair<double,double>> pts;
    unsigned semilla = 12345;
    auto rnd = [&]() {
        semilla = semilla * 1103515245u + 12345u;
        return ((semilla >> 8) % 100000) / 100000.0;   // 5 decimales: sin empates practicos
    };
    for (int i = 0; i < 300; i++) {
        double x = rnd(), y = rnd();
        pts.push_back({x, y});
        arbol.insertar(x, y, i);
    }
    double qx = 0.5, qy = 0.5;
    auto knn = arbol.kVecinos(qx, qy, 10);
    vector<pair<double,int>> fb;   // fuerza bruta
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

static void test_eliminar() {
    cout << "\nT6: eliminar con condensacion" << endl;
    RStarTree2D<int> arbol(8, 3);
    int id = 0;
    for (int i = 0; i < 25; i++)
        for (int j = 0; j < 20; j++)
            arbol.insertar(i * 0.01, j * 0.01, id++);

    // borrar los 200 puntos de las primeras 10 filas (datos 0..199)
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

static void test_hojas_version() {
    cout << "\nT7: vistas de hojas y versiones" << endl;
    RStarTree2D<int> arbol(8, 3);
    for (int i = 0; i < 50; i++) arbol.insertar(i * 0.1, i * 0.1, i);
    size_t total = 0;
    int hojas = 0;
    arbol.visitarHojas([&](const RStarTree2D<int>::HojaVista& h) { hojas++; total += h.entradas.size(); });
    CHECK(total == 50, "las hojas contienen los 50 puntos");
    CHECK(hojas > 1, "hubo splits (mas de una hoja)");

    int enRango = 0;
    arbol.visitarHojasEnRango(Caja(0, 0, 1.0, 1.0), [&](const RStarTree2D<int>::HojaVista&) { enRango++; });
    CHECK(enRango >= 1 && enRango <= hojas, "visitarHojasEnRango filtra por bbox");

    uint64_t vAntes = 0;
    uintptr_t clave0 = 0;
    arbol.visitarHojas([&](const RStarTree2D<int>::HojaVista& h) {
        if (clave0 == 0) { clave0 = h.clave; vAntes = h.version; }
    });
    arbol.insertar(0.01, 0.01, 999);   // cae en la primera hoja
    uint64_t vDespues = vAntes;
    bool halloClave = false;
    arbol.visitarHojas([&](const RStarTree2D<int>::HojaVista& h) {
        if (h.clave == clave0) { vDespues = h.version; halloClave = true; }
    });
    CHECK(halloClave && vDespues != vAntes, "la version de la hoja mutada cambio");
}

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

struct Viaje { int id; int etiqueta; vector<double> pcs; };

static void test_grupos() {
    cout << "\nT9: GruposPorHoja e invalidacion" << endl;
    RStarTree2D<Viaje> arbol(8, 3);
    // 60 puntos en linea, etiqueta = i % 3
    for (int i = 0; i < 60; i++)
        arbol.insertar(i * 0.01, 0.0, Viaje{i, i % 3, {(double)(i % 3), 0.0}});

    GruposPorHoja<Viaje, int> grupos(arbol,
        [](const Viaje& v) { return v.etiqueta; },
        [](const Viaje& v) { return v.pcs; });
    grupos.construir();

    auto gs = grupos.gruposEnRango(Caja(-1, -1, 1, 1));
    CHECK(gs.size() == 3, "3 grupos fusionados por etiqueta en rango global");
    size_t total = 0;
    for (auto& g : gs) total += g.size();
    CHECK(total == 60, "los 60 puntos repartidos en los grupos");
    bool coherentes = true;
    for (auto& g : gs) {
        int et = arbol.dato(g[0]).etiqueta;
        for (uint32_t idx : g) if (arbol.dato(idx).etiqueta != et) coherentes = false;
    }
    CHECK(coherentes, "cada grupo tiene una sola etiqueta");

    // bbox parcial: solo los indices 0..9 (x <= 0.095)
    auto parcial = grupos.gruposEnRango(Caja(-0.01, -0.01, 0.095, 0.01));
    size_t enParcial = 0;
    for (auto& g : parcial) enParcial += g.size();
    CHECK(enParcial == 10, "bbox parcial filtra miembros fuera del rango");

    // invalidacion perezosa: insertar tras construir
    arbol.insertar(0.005, 0.0, Viaje{999, 0, {0.0, 0.0}});
    auto gs2 = grupos.gruposEnRango(Caja(-1, -1, 1, 1));
    size_t total2 = 0;
    for (auto& g : gs2) total2 += g.size();
    CHECK(total2 == 61, "grupo rearmado perezosamente tras insercion");
}

static void test_n_similares() {
    cout << "\nT10: nSimilares" << endl;
    RStarTree2D<Viaje> arbol(8, 3);
    // 30 puntos dentro del futuro bbox [0, 0.5]:
    //   10 etiqueta 0 (pcs {0}), 10 etiqueta 1 (pcs {5}), 10 etiqueta 2 (pcs {9})
    int id = 0;
    for (int e = 0; e < 3; e++)
        for (int i = 0; i < 10; i++)
            arbol.insertar(0.015 * (e * 10 + i), 0.0, Viaje{id++, e, {e == 2 ? 9.0 : e * 5.0}});
    // referente: etiqueta 1, pcs {5}, FUERA del bbox (x=2.0)
    uint32_t ref = arbol.insertar(2.0, 2.0, Viaje{999, 1, {5.0}});

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
    // centroides: etiqueta 0 -> {0} (dist 5 de {5}); etiqueta 2 -> {9} (dist 4)
    // => los 5 extra deben venir de etiqueta 2
    int deEt2 = 0;
    for (size_t i = 10; i < 15; i++) if (arbol.dato(sim15[i]).etiqueta == 2) deEt2++;
    CHECK(deEt2 == 5, "los extra vienen del grupo con centroide mas cercano (etiqueta 2)");
}

int main() {
    cout << "=== Tests rstarLib ===" << endl;
    test_caja();
    test_esqueleto();
    test_rango_y_recorrido();
    test_invariantes();
    test_knn();
    test_eliminar();
    test_hojas_version();
    test_indice_por_id();
    test_grupos();
    test_n_similares();
    cout << "\n=== Resultado: " << (fallos == 0 ? "TODOS PASAN" : to_string(fallos) + " FALLOS") << " ===" << endl;
    return fallos == 0 ? 0 : 1;
}
