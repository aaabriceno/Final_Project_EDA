// Tests de rstarLib. Correr: make test
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

int main() {
    cout << "=== Tests rstarLib ===" << endl;
    test_caja();
    test_esqueleto();
    test_rango_y_recorrido();
    cout << "\n=== Resultado: " << (fallos == 0 ? "TODOS PASAN" : to_string(fallos) + " FALLOS") << " ===" << endl;
    return fallos == 0 ? 0 : 1;
}
