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
    RStarTree2D<Taxi> arbol;   // M=1200, m=480 por defecto
    mt19937 gen(42);
    uniform_real_distribution<double> dLat(40.55, 40.95), dLon(-74.10, -73.70);
    uniform_int_distribution<int> dEt(0, 9);
    normal_distribution<double> dPc(0.0, 1.0);

    // FASE CARGA (en un caso real: leidos del .bin ya ordenados por lat/long)
    const int N = 100000;
    for (int i = 0; i < N; i++) {
        double lat = dLat(gen), lon = dLon(gen);
        int et = dEt(gen);
        arbol.insertar(lat, lon, Taxi{i, et, {et + dPc(gen) * 0.1, dPc(gen)}, lat, lon});
    }
    cout << "Cargados " << arbol.tamano() << " puntos" << endl;

    // FASE PRECOMPUTO (una vez, tras la carga)
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
        cout << "\nConsulta 1: " << sim.size() << " similares al tripID 45020"
             << " (etiqueta " << arbol.dato(*idxRef).etiqueta << "):" << endl;
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

    // EXTRAS: kNN geografico
    auto vecinos = arbol.kVecinos(40.7528, -73.9765, 5);
    cout << "\n5 taxis mas cercanos a Grand Central: ";
    for (auto& v : vecinos) cout << arbol.dato(v.idx).tripID << " ";
    cout << endl;

    return 0;
}
