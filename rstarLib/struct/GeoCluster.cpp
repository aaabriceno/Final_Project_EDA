#include "GeoCluster.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <array>
#include <unordered_map>
#include <iostream>
#include <limits>
#include <iomanip>
#include <functional>
#include <cassert>

using namespace std;

// Configurar precisión global para todos los números de punto flotante
void configurarPrecision() {
    cout.setf(ios::fixed, ios::floatfield);
    cout.precision(10);
}

GeoCluster::GeoCluster() {
    cout << "Constructor llamado con parámetros optimizados" << endl;
    cout << "MAX_PUNTOS_POR_NODO: " << MAX_PUNTOS_POR_NODO << endl;
    cout << "MIN_PUNTOS_POR_NODO: " << MIN_PUNTOS_POR_NODO << endl;
    raiz = nullptr;
    configurarPrecision();  // Configurar precisión para todos los números
}

GeoCluster::~GeoCluster() {
    if (raiz != nullptr) {
        delete raiz;
        raiz = nullptr;
    }
    std::cout << "Destructor llamado" << std::endl;
}

// NUEVOS MÉTODOS PARA MANEJAR LA HASH TABLE DE PUNTOS COMPLETOS
void GeoCluster::almacenarPuntoCompleto(const Punto& punto) {
    puntos_completos[punto.id] = punto;
}

Punto GeoCluster::obtenerPuntoCompleto(int id_punto) const {
    auto it = puntos_completos.find(id_punto);
    if (it != puntos_completos.end()) {
        return it->second;
    }
    // Retornar un punto vacío si no se encuentra
    return Punto();
}

bool GeoCluster::existePuntoCompleto(int id_punto) const {
    return puntos_completos.find(id_punto) != puntos_completos.end();
}

void GeoCluster::limpiarPuntosCompletos() {
    puntos_completos.clear();
}

int GeoCluster::obtenerNumeroPuntosCompletos() const {
    return puntos_completos.size();
}

// ============================================================================
// INSERCIÓN R*-TREE (Beckmann et al. 1990, sección 4)
// ============================================================================

// MBR degenerado de un punto
static MBR mbrDePunto(const PointID& p) {
    return MBR(p.latitud, p.longitud, p.latitud, p.longitud);
}

// Cuánto crece el área de 'mbr' al estirarse para incluir 'e'
static double ampliacionArea(const MBR& mbr, const MBR& e) {
    MBR ampliado = mbr;
    ampliado.stretch(e);
    return ampliado.area() - mbr.area();
}

// ID1: los datos entran siempre por el nivel hoja
void GeoCluster::inserData(const Punto& punto_a_insertar){
    Punto punto = punto_a_insertar;
    punto.optimizarAtributos();
    almacenarPuntoCompleto(punto);

    // OT1: la marca "primera vez en este nivel" vale por cada dato insertado
    niveles_reinsert.clear();
    insertar(punto, 0);
}

// I1-I4: inserta un punto de datos en el nivel hoja
void GeoCluster::insertar(const Punto& punto, int nivel) {
    (void)nivel; // los puntos de datos van siempre al nivel 0 (hojas)
    if (raiz == nullptr) {
        raiz = new Nodo(true);
        raiz->m_nivel = 0;
        raiz->padre = nullptr;
    }

    MBR entrada(punto.latitud, punto.longitud, punto.latitud, punto.longitud);
    Nodo* N = chooseSubTree(entrada, 0);               // I1
    N->puntos.push_back(PointID(punto));               // I2
    ajustarMBRHaciaArriba(N);                          // I4
    if ((int)N->puntos.size() > MAX_PUNTOS_POR_NODO)   // I2/I3: overflow con M+1 entradas
        overflowTreatment(N);
}

// Reinserción de una entrada de nodo interno: cuelga el subárbol completo en
// un nodo del nivel que le corresponde (paper 4.3, reinserción por niveles)
void GeoCluster::insertarSubarbol(Nodo* subarbol) {
    Nodo* N = chooseSubTree(subarbol->mbr, subarbol->m_nivel + 1);
    subarbol->padre = N;
    N->hijos.push_back(subarbol);
    ajustarMBRHaciaArriba(N);
    if ((int)N->hijos.size() > MAX_PUNTOS_POR_NODO)
        overflowTreatment(N);
}

// I4: tras insertar, todos los MBR del camino hasta la raíz deben cubrir la entrada
void GeoCluster::ajustarMBRHaciaArriba(Nodo* N) {
    while (N != nullptr) {
        updateMBR(N);
        N = N->padre;
    }
}

// 4.1 ChooseSubtree: desciende desde la raíz hasta un nodo del nivel destino.
// CS2: si los hijos son hojas, mínima ampliación de overlap (empates: menor
// ampliación de área, luego menor área); si son internos, mínima ampliación
// de área (empates: menor área).
Nodo* GeoCluster::chooseSubTree(const MBR& entrada, int nivel_destino) {
    Nodo* N = raiz;                                    // CS1
    while (N != nullptr && N->m_nivel > nivel_destino) {
        Nodo* mejor = (N->m_nivel == 1)
            ? hijoConMenorAmpliacionOverlap(N, entrada)
            : hijoConMenorAmpliacionArea(N, entrada);
        if (mejor == nullptr) {
            cerr << "Error: nodo interno sin hijos validos en chooseSubTree" << endl;
            return N;
        }
        N = mejor;                                     // CS3
    }
    return N;
}

Nodo* GeoCluster::hijoConMenorAmpliacionArea(Nodo* N, const MBR& entrada) {
    Nodo* mejor = nullptr;
    double mejorCosto = numeric_limits<double>::infinity();
    double mejorArea = numeric_limits<double>::infinity();
    for (Nodo* hijo : N->hijos) {
        if (hijo == nullptr) continue;
        double costo = ampliacionArea(hijo->mbr, entrada);
        double area = hijo->mbr.area();
        if (costo < mejorCosto || (costo == mejorCosto && area < mejorArea)) {
            mejorCosto = costo;
            mejorArea = area;
            mejor = hijo;
        }
    }
    return mejor;
}

// Overlap del paper: overlap(E_k) = sum_{i!=k} area(E_k ∩ E_i). Se elige la
// entrada cuya ampliación de overlap al incluir la nueva entrada es mínima.
// Optimización del paper (4.1): evaluar solo los 32 hijos con menor
// ampliación de área — el costo exacto es cuadrático en M.
Nodo* GeoCluster::hijoConMenorAmpliacionOverlap(Nodo* N, const MBR& entrada) {
    const int CANDIDATOS = 32;
    int n = (int)N->hijos.size();

    vector<int> orden(n);
    for (int i = 0; i < n; i++) orden[i] = i;
    sort(orden.begin(), orden.end(), [&](int a, int b) {
        return ampliacionArea(N->hijos[a]->mbr, entrada) <
               ampliacionArea(N->hijos[b]->mbr, entrada);
    });

    int evaluar = min(n, CANDIDATOS);
    Nodo* mejor = nullptr;
    double mejorOverlap = numeric_limits<double>::infinity();
    double mejorCostoArea = numeric_limits<double>::infinity();
    double mejorArea = numeric_limits<double>::infinity();

    for (int c = 0; c < evaluar; c++) {
        int k = orden[c];
        Nodo* hijoK = N->hijos[k];
        if (hijoK == nullptr) continue;

        MBR ampliado = hijoK->mbr;
        ampliado.stretch(entrada);

        double antes = 0.0, despues = 0.0;
        for (int i = 0; i < n; i++) {
            if (i == k || N->hijos[i] == nullptr) continue;
            antes   += hijoK->mbr.overlap(N->hijos[i]->mbr);
            despues += ampliado.overlap(N->hijos[i]->mbr);
        }
        double costoOverlap = despues - antes;
        double costoArea = ampliacionArea(hijoK->mbr, entrada);
        double area = hijoK->mbr.area();

        if (costoOverlap < mejorOverlap ||
            (costoOverlap == mejorOverlap &&
             (costoArea < mejorCostoArea ||
              (costoArea == mejorCostoArea && area < mejorArea)))) {
            mejorOverlap = costoOverlap;
            mejorCostoArea = costoArea;
            mejorArea = area;
            mejor = hijoK;
        }
    }
    return mejor;
}

// OT1: primera vez en el nivel (y no raíz) => reinsertar; si no => split
void GeoCluster::overflowTreatment(Nodo* N) {
    if (N == nullptr) return;
    if (N != raiz && niveles_reinsert.find(N->m_nivel) == niveles_reinsert.end()) {
        niveles_reinsert[N->m_nivel] = true;
        reinsertar(N);
    } else {
        split(N);
    }
}

// 4.3 ReInsert (RI1-RI4). Variante "close reinsert": el paper reporta que
// reinsertar empezando por la distancia MÍNIMA rinde mejor que far reinsert.
// Funciona en hojas (puntos) y en nodos internos (subárboles).
void GeoCluster::reinsertar(Nodo* N) {
    if (N == nullptr) return;

    // RI1: distancias de los centros de las entradas al centro del MBR de N
    double centro_lat = (N->mbr.m_minp[0] + N->mbr.m_maxp[0]) / 2.0;
    double centro_lon = (N->mbr.m_minp[1] + N->mbr.m_maxp[1]) / 2.0;

    int total = N->esHoja ? (int)N->puntos.size() : (int)N->hijos.size();

    // p = 30% de M (mejor valor experimental del paper, hojas e internos)
    int p = (int)(MAX_PUNTOS_POR_NODO * PORCENTAJE_PARA_HACER_REINSERT);
    if (p < 1) p = 1;
    if (p > total - 1) p = total - 1;
    if (p < 1) return;

    vector<pair<double, int>> dist(total);   // {distancia^2, índice}
    for (int i = 0; i < total; i++) {
        double la, lo;
        if (N->esHoja) {
            la = N->puntos[i].latitud;
            lo = N->puntos[i].longitud;
        } else {
            la = (N->hijos[i]->mbr.m_minp[0] + N->hijos[i]->mbr.m_maxp[0]) / 2.0;
            lo = (N->hijos[i]->mbr.m_minp[1] + N->hijos[i]->mbr.m_maxp[1]) / 2.0;
        }
        double dla = la - centro_lat, dlo = lo - centro_lon;
        dist[i] = { dla * dla + dlo * dlo, i };
    }

    // RI2: orden decreciente de distancia
    sort(dist.begin(), dist.end(),
         [](const pair<double,int>& a, const pair<double,int>& b) { return a.first > b.first; });

    // RI3: quitar las p entradas más lejanas
    vector<bool> quitar(total, false);
    for (int c = 0; c < p; c++) quitar[dist[c].second] = true;

    vector<PointID> puntosQuitados;
    vector<Nodo*> hijosQuitados;
    // guardados en orden de distancia CRECIENTE (close reinsert)
    for (int c = p - 1; c >= 0; c--) {
        int i = dist[c].second;
        if (N->esHoja) puntosQuitados.push_back(N->puntos[i]);
        else           hijosQuitados.push_back(N->hijos[i]);
    }

    if (N->esHoja) {
        vector<PointID> resto;
        resto.reserve(total - p);
        for (int i = 0; i < total; i++) if (!quitar[i]) resto.push_back(N->puntos[i]);
        N->puntos = std::move(resto);
    } else {
        vector<Nodo*> resto;
        resto.reserve(total - p);
        for (int i = 0; i < total; i++) if (!quitar[i]) resto.push_back(N->hijos[i]);
        N->hijos = std::move(resto);
    }
    ajustarMBRHaciaArriba(N);

    // RI4: reinsertar empezando por la distancia mínima
    if (N->esHoja) {
        for (const PointID& pid : puntosQuitados) {
            // el PointID trae todo lo que la hoja necesita; los atributos
            // completos siguen en la hash table puntos_completos
            Punto pr(pid.id, pid.latitud, pid.longitud, vector<double>());
            pr.id_cluster_geografico = pid.id_cluster_geografico;
            pr.id_subcluster_atributivo = pid.id_subcluster_atributivo;
            insertar(pr, 0);
        }
    } else {
        for (Nodo* h : hijosQuitados) insertarSubarbol(h);
    }
}

// Resultado de la selección de split (4.2): eje, permutación de las entradas
// según el orden ganador y tamaño del primer grupo
struct SeleccionSplit {
    int eje;
    vector<int> orden;
    int tamGrupo1;
};

// S1 ChooseSplitAxis (CSA1-CSA2) + S2 ChooseSplitIndex (CSI1) sobre MBRs.
// Por cada eje se ordena por límite inferior y superior; cada orden genera
// las distribuciones con grupo1 = m..E-m. S(eje) = suma de márgenes de todas
// sus distribuciones; gana el eje de menor S. En ese eje, gana la
// distribución de menor overlap (empate: menor área). MBRs prefijo/sufijo
// permiten evaluar cada orden en O(E).
static SeleccionSplit elegirSplit(const vector<MBR>& ent) {
    int E = (int)ent.size();
    int m = MIN_PUNTOS_POR_NODO;
    if (m > E / 2) m = E / 2;   // salvaguarda: garantiza >= 1 distribución
    if (m < 1) m = 1;
    int numDistribuciones = E - 2 * m + 1;

    vector<int> ordenes[2][2];  // [eje][clave: 0=inferior, 1=superior]
    double S[2] = {0.0, 0.0};

    auto calcularPrefSuf = [&](const vector<int>& orden, vector<MBR>& pref, vector<MBR>& suf) {
        pref.resize(E); suf.resize(E);
        pref[0] = ent[orden[0]];
        for (int i = 1; i < E; i++) { pref[i] = pref[i-1]; pref[i].stretch(ent[orden[i]]); }
        suf[E-1] = ent[orden[E-1]];
        for (int i = E - 2; i >= 0; i--) { suf[i] = suf[i+1]; suf[i].stretch(ent[orden[i]]); }
    };

    for (int eje = 0; eje < 2; eje++) {
        for (int clave = 0; clave < 2; clave++) {
            vector<int>& orden = ordenes[eje][clave];
            orden.resize(E);
            for (int i = 0; i < E; i++) orden[i] = i;
            sort(orden.begin(), orden.end(), [&](int a, int b) {
                double ka = (clave == 0) ? ent[a].m_minp[eje] : ent[a].m_maxp[eje];
                double kb = (clave == 0) ? ent[b].m_minp[eje] : ent[b].m_maxp[eje];
                return ka < kb;
            });

            vector<MBR> pref, suf;
            calcularPrefSuf(orden, pref, suf);
            for (int k = 0; k < numDistribuciones; k++) {
                int g1 = m + k;
                S[eje] += pref[g1 - 1].margin() + suf[g1].margin();
            }
        }
    }

    SeleccionSplit sel;
    sel.eje = (S[0] <= S[1]) ? 0 : 1;   // CSA2

    // CSI1: mínima suma de overlap; empate por mínima suma de áreas
    double mejorOverlap = numeric_limits<double>::infinity();
    double mejorArea = numeric_limits<double>::infinity();
    for (int clave = 0; clave < 2; clave++) {
        const vector<int>& orden = ordenes[sel.eje][clave];
        vector<MBR> pref, suf;
        calcularPrefSuf(orden, pref, suf);
        for (int k = 0; k < numDistribuciones; k++) {
            int g1 = m + k;
            double ov = pref[g1 - 1].overlap(suf[g1]);
            double ar = pref[g1 - 1].area() + suf[g1].area();
            if (ov < mejorOverlap || (ov == mejorOverlap && ar < mejorArea)) {
                mejorOverlap = ov;
                mejorArea = ar;
                sel.orden = orden;
                sel.tamGrupo1 = g1;
            }
        }
    }
    return sel;
}

// 4.2 Split (S1-S3) + I3 (propagación hacia arriba). Genérico: parte tanto
// hojas (puntos) como nodos internos (subárboles).
void GeoCluster::split(Nodo* N) {
    if (N == nullptr) return;

    vector<MBR> entradas;
    if (N->esHoja) {
        entradas.reserve(N->puntos.size());
        for (const auto& p : N->puntos) entradas.push_back(mbrDePunto(p));
    } else {
        entradas.reserve(N->hijos.size());
        for (Nodo* h : N->hijos) entradas.push_back(h->mbr);
    }
    if (entradas.size() < 2) {
        cerr << "Error: nodo con menos de 2 entradas en split" << endl;
        return;
    }

    SeleccionSplit sel = elegirSplit(entradas);

    Nodo* nuevo = new Nodo(N->esHoja);
    nuevo->m_nivel = N->m_nivel;

    // S3: distribuir las entradas en los dos grupos
    if (N->esHoja) {
        vector<PointID> g1, g2;
        g1.reserve(sel.tamGrupo1);
        g2.reserve(entradas.size() - sel.tamGrupo1);
        for (int i = 0; i < (int)sel.orden.size(); i++)
            (i < sel.tamGrupo1 ? g1 : g2).push_back(N->puntos[sel.orden[i]]);
        N->puntos = std::move(g1);
        nuevo->puntos = std::move(g2);
    } else {
        vector<Nodo*> g1, g2;
        g1.reserve(sel.tamGrupo1);
        g2.reserve(entradas.size() - sel.tamGrupo1);
        for (int i = 0; i < (int)sel.orden.size(); i++)
            (i < sel.tamGrupo1 ? g1 : g2).push_back(N->hijos[sel.orden[i]]);
        N->hijos = std::move(g1);
        nuevo->hijos = std::move(g2);
        for (Nodo* h : nuevo->hijos) h->padre = nuevo;
    }

    updateMBR(N);
    updateMBR(nuevo);

    // I3: propagar el split hacia arriba
    if (N == raiz) {
        Nodo* nuevaRaiz = new Nodo(false);
        nuevaRaiz->m_nivel = N->m_nivel + 1;
        nuevaRaiz->padre = nullptr;
        nuevaRaiz->hijos.push_back(N);
        nuevaRaiz->hijos.push_back(nuevo);
        N->padre = nuevaRaiz;
        nuevo->padre = nuevaRaiz;
        updateMBR(nuevaRaiz);
        raiz = nuevaRaiz;
    } else {
        Nodo* padre = N->padre;
        nuevo->padre = padre;
        padre->hijos.push_back(nuevo);
        ajustarMBRHaciaArriba(padre);
        if ((int)padre->hijos.size() > MAX_PUNTOS_POR_NODO)
            overflowTreatment(padre);   // el overflow puede cascadear hacia arriba
    }
}

// 4.2 CSA: expuesto para tests — devuelve el eje que elegiría el split
int GeoCluster::chooseSplitAxis(const vector<PointID>& puntos) {
    if (puntos.size() < 2) return 0;
    vector<MBR> entradas;
    entradas.reserve(puntos.size());
    for (const auto& p : puntos) entradas.push_back(mbrDePunto(p));
    return elegirSplit(entradas).eje;
}

void GeoCluster::updateMBR(Nodo* nodo) {
    // Verificar que el nodo sea válido
    if (nodo == nullptr) {
        cerr << "Error: Nodo nullptr en updateMBR" << endl;
        return;
    }
    
    if (nodo->esHoja) {
        if (nodo->puntos.empty()) {
            nodo->mbr.reset();
        } else {
            // NUEVO: Usar la función que trabaja con PointID
            nodo->mbr = calcularMBR(nodo->puntos);
        }
    } else {
        if (nodo->hijos.empty()) {
            nodo->mbr.reset();
        } else {
            // Encontrar el primer hijo válido
            Nodo* primerHijo = nullptr;
            for (auto& hijo : nodo->hijos) {
                if (hijo != nullptr) {
                    primerHijo = hijo;
                    break;
                }
            }
            
            if (primerHijo != nullptr) {
                nodo->mbr = primerHijo->mbr;
                for (size_t i = 1; i < nodo->hijos.size(); i++) {
                    if (nodo->hijos[i] != nullptr) {
                        nodo->mbr.stretch(nodo->hijos[i]->mbr);
                    }
                }
            } else {
                nodo->mbr.reset();
            }
        }
    }
}

bool GeoCluster::estaDentroDelMBR(const Punto& punto, const MBR& mbr) {
    bool dentroLatitud = (punto.latitud >= mbr.m_minp[0] && punto.latitud <= mbr.m_maxp[0]);
    bool dentroLongitud = (punto.longitud >= mbr.m_minp[1] && punto.longitud <= mbr.m_maxp[1]);

    return dentroLatitud && dentroLongitud;
}

// NUEVA: Función para verificar si un PointID está dentro del MBR
bool GeoCluster::estaDentroDelMBR(const PointID& point_id, const MBR& mbr) {
    bool dentroLatitud = (point_id.latitud >= mbr.m_minp[0] && point_id.latitud <= mbr.m_maxp[0]);
    bool dentroLongitud = (point_id.longitud >= mbr.m_minp[1] && point_id.longitud <= mbr.m_maxp[1]);

    return dentroLatitud && dentroLongitud;
}

bool GeoCluster::interseccionMBR(const MBR& mbr1, const MBR& mbr2) {
    return mbr1.is_intersected(mbr2);
}

vector<Punto> GeoCluster::buscarPuntosDentroInterseccion(const MBR& rango, Nodo* nodo) {
    vector<Punto> puntos_encontrados;
    if (nodo == nullptr) {
        return puntos_encontrados;
    }

    // Si no hay intersección con el MBR del nodo, retornar vacío
    if (!interseccionMBR(nodo->mbr, rango)) {
        return puntos_encontrados;
    }

    // Si es hoja, verificar cada punto
    if (nodo->esHoja) {
        for (const auto& point_id : nodo->puntos) {
            if (point_id.latitud >= rango.m_minp[0] && point_id.latitud <= rango.m_maxp[0] &&
                point_id.longitud >= rango.m_minp[1] && point_id.longitud <= rango.m_maxp[1]) {
                // Recuperar el punto completo; id=0 es un id válido, la
                // existencia se comprueba con find (no con un centinela)
                auto it = puntos_completos.find(point_id.id);
                if (it != puntos_completos.end()) {
                    puntos_encontrados.push_back(it->second);
                }
            }
        }
    }
    // Si no es hoja, buscar recursivamente en los hijos
    else {
        for (auto hijo : nodo->hijos) {
            vector<Punto> puntos_hijo = buscarPuntosDentroInterseccion(rango, hijo);
            puntos_encontrados.insert(puntos_encontrados.end(), puntos_hijo.begin(), puntos_hijo.end());
        }
    }

    return puntos_encontrados;
}

void GeoCluster::searchRec(const MBR& rango, Nodo* nodo, vector<Punto>& puntos_similares) {
    if (interseccionMBR(nodo->mbr, rango)) {
        if (nodo->esHoja) {
            for (const auto& point_id : nodo->puntos) {
                if (estaDentroDelMBR(point_id, rango)) {
                    // id=0 es válido: comprobar existencia con find
                    auto it = puntos_completos.find(point_id.id);
                    if (it != puntos_completos.end()) {
                        puntos_similares.push_back(it->second);
                    }
                }
            }
        } else {
            for (Nodo* hijo : nodo->hijos) {
                searchRec(rango, hijo, puntos_similares);
            }
        }
    }
}

void GeoCluster::imprimirArbol(Nodo* nodo, int nivel) {
    if (nodo == nullptr) {
        nodo = raiz;
    }
    
    if (nodo == nullptr) {
        cout << "Árbol vacío" << endl;
        return;
    }
    
    string indentacion(nivel * 2, ' ');
    cout << indentacion << "Nivel " << nodo->m_nivel << " (";
    cout << (nodo->esHoja ? "Hoja" : "Interno") << "): ";
    
    if (nodo->esHoja) {
        cout << nodo->puntos.size() << " puntos";
    } else {
        cout << nodo->hijos.size() << " hijos";
    }
    
    cout << " MBR: [" << nodo->mbr.m_minp[0] << "," << nodo->mbr.m_maxp[0] << "] x ["
         << nodo->mbr.m_minp[1] << "," << nodo->mbr.m_maxp[1] << "]" << endl;
    
    if (!nodo->esHoja) {
        for (Nodo* hijo : nodo->hijos) {
            if (hijo != nullptr) {
                imprimirArbol(hijo, nivel + 1);
            }
        }
    }
}


void GeoCluster::imprimirArbol() {
    cout << "\n=== ESTRUCTURA DEL R*-TREE ===" << endl;
    imprimirArbol(raiz, 0);
    cout << "================================\n" << endl;
}

// ============================================================================
// FUNCIONES DE CONSULTA OPTIMIZADAS PARA TIEMPO REAL
// ============================================================================

ResultadoBusqueda GeoCluster::n_puntos_similiares_a_punto(const Punto& punto_de_busqueda, MBR& rango, int numero_de_puntos_similares) {
    ResultadoBusqueda resultado;
    
    cout << "[ULTRA-OPTIMIZADO] Búsqueda usando clusters y hash tables..." << endl;
    
    // 1. Obtener puntos en el rango de forma eficiente
    vector<Punto> candidatos = buscarPuntosDentroInterseccion(rango, raiz);
    cout << "[INFO] Candidatos en rango: " << candidatos.size() << endl;
    
    if (candidatos.empty()) return resultado;
    
    // 2. Crear estructura optimizada con unordered_map
    unordered_map<string, vector<Punto>> puntos_por_cluster;
    unordered_map<int, Punto> mapa_puntos;
    
    // 3. Agrupar candidatos por cluster y subcluster O(1) por punto
    for (const auto& punto : candidatos) {
        string clave = to_string(punto.id_cluster_geografico) + "_" + to_string(punto.id_subcluster_atributivo);
        puntos_por_cluster[clave].push_back(punto);
        mapa_puntos[punto.id] = punto;
    }
    
    // 4. Calcular similitudes con priorización por clusters
    vector<pair<double, Punto>> puntos_ordenados;
    
    // 4.1. PRIORIDAD 1: Mismo cluster y mismo subcluster (máxima similitud)
    string clave_mismo = to_string(punto_de_busqueda.id_cluster_geografico) + "_" + 
                        to_string(punto_de_busqueda.id_subcluster_atributivo);
    
    auto it_mismo = puntos_por_cluster.find(clave_mismo);
    if (it_mismo != puntos_por_cluster.end()) {
        for (const auto& punto : it_mismo->second) {
            if (punto.id != punto_de_busqueda.id) {
                double similitud = calcularSimilitudOptimizada(punto_de_busqueda, punto);
                puntos_ordenados.push_back({similitud, punto});
            }
        }
    }
    
    // 4.2. PRIORIDAD 2: Mismo cluster, diferente subcluster
    for (const auto& [clave, puntos] : puntos_por_cluster) {
        if (clave != clave_mismo && clave.substr(0, clave.find('_')) == 
            to_string(punto_de_busqueda.id_cluster_geografico)) {
            for (const auto& punto : puntos) {
                double similitud = calcularSimilitudOptimizada(punto_de_busqueda, punto);
                puntos_ordenados.push_back({similitud, punto});
            }
        }
    }
    
    // 4.3. PRIORIDAD 3: Diferente cluster
    for (const auto& [clave, puntos] : puntos_por_cluster) {
        if (clave != clave_mismo && clave.substr(0, clave.find('_')) != 
            to_string(punto_de_busqueda.id_cluster_geografico)) {
            for (const auto& punto : puntos) {
                double similitud = calcularSimilitudOptimizada(punto_de_busqueda, punto);
                puntos_ordenados.push_back({similitud, punto});
            }
        }
    }
    
    // 5. Ordenar por similitud (mayor primero)
    sort(puntos_ordenados.begin(), puntos_ordenados.end(),
         [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // 6. Agregar al resultado
    for (const auto& [similitud, punto] : puntos_ordenados) {
        resultado.agregar(punto, similitud);
        if (resultado.puntos.size() >= numero_de_puntos_similares) break;
    }
    
    cout << "[ULTRA-OPTIMIZADO] Búsqueda completada. Puntos encontrados: " 
         << resultado.puntos.size() << endl;
    
    return resultado;
}

// Función auxiliar para calcular similitud optimizada
double GeoCluster::calcularSimilitudOptimizada(const Punto& p1, const Punto& p2) {
    // 1. Similitud por atributos directos (50% del peso)
    double similitud_atributos = 0.0;
    double distancia = 0.0;
    
    int num_atributos = min((int)p1.atributos.size(), (int)p2.atributos.size());
    for (int i = 0; i < num_atributos; i++) {
        double diff = p1.atributos[i] - p2.atributos[i];
        distancia += diff * diff;
    }
    similitud_atributos = exp(-sqrt(distancia));
    
    // 2. Similitud por cluster geográfico (30% del peso)
    double similitud_cluster = (p1.id_cluster_geografico == p2.id_cluster_geografico) ? 1.0 : 0.0;
    
    // 3. Similitud por subcluster atributivo (20% del peso)
    double similitud_subcluster = (p1.id_subcluster_atributivo == p2.id_subcluster_atributivo) ? 1.0 : 0.0;
    
    // 4. Combinar similitudes
    return (0.5 * similitud_atributos) + (0.3 * similitud_cluster) + (0.2 * similitud_subcluster);
}

vector<vector<Punto>> GeoCluster::grupos_similares_de_puntos(MBR& rango) {
    vector<vector<Punto>> grupos_resultado;
    
    // Buscar todos los nodos hojas que intersecten con el rango
    vector<Nodo*> nodos_en_rango;
    buscarNodosHojasEnRango(raiz, rango, nodos_en_rango);
    
    // cout << "  Nodos hojas en rango: " << nodos_en_rango.size() << endl;
    
    // Para cada nodo, crear grupos basados en subclusters
    for (Nodo* nodo : nodos_en_rango) {
        if (nodo->puntos.empty()) continue;
        
        // Agrupar por subclusters
        unordered_map<int, vector<Punto>> grupos_por_subcluster;
        for (const auto& point_id : nodo->puntos) {
            auto it = puntos_completos.find(point_id.id);
            if (it != puntos_completos.end()) { // id=0 es válido
                grupos_por_subcluster[it->second.id_subcluster_atributivo].push_back(it->second);
            }
        }
        
        // Convertir a vector de vectores
        for (const auto& [subcluster_id, puntos_grupo] : grupos_por_subcluster) {
            if (puntos_grupo.size() >= 2) { // Solo grupos con al menos 2 puntos
                grupos_resultado.push_back(puntos_grupo);
            }
        }
    }
    
    cout << "Grupos similares encontrados: " << grupos_resultado.size() << endl;
    
    // Mostrar TODOS los grupos con TODOS sus puntos
    for (size_t i = 0; i < grupos_resultado.size(); i++) {
        cout << "\n=== GRUPO " << (i+1) << " ===" << endl;
        cout << "Cantidad de puntos: " << grupos_resultado[i].size() << endl;
        
        if (!grupos_resultado[i].empty()) {
            cout << "Cluster geográfico: " << grupos_resultado[i][0].id_cluster_geografico 
                 << ", Subcluster atributivo: " << grupos_resultado[i][0].id_subcluster_atributivo << endl;
            //cout << "Puntos del grupo:" << endl;
            
            // Imprimir TODOS los puntos del grupo
            /*
            for (size_t j = 0; j < grupos_resultado[i].size(); j++) {
                const auto& punto = grupos_resultado[i][j];
                cout << "  " << (j+1) << ". ID: " << punto.id 
                     << " (Lat: " << punto.latitud 
                     << ", Lon: " << punto.longitud << ")" << endl;
            }
            */
        }
    }
    
    return grupos_resultado;
}

// Función auxiliar para buscar nodos hojas en un rango
void GeoCluster::buscarNodosHojasEnRango(Nodo* nodo, const MBR& rango, vector<Nodo*>& nodos_encontrados) {
    if (nodo == nullptr) return;
    
    if (!interseccionMBR(nodo->mbr, rango)) {
        return; // Nodo no intersecta con el rango
    }
    
    if (nodo->esHoja) {
        nodos_encontrados.push_back(nodo);
    } else {
        for (Nodo* hijo : nodo->hijos) {
            buscarNodosHojasEnRango(hijo, rango, nodos_encontrados);
        }
    }
}

// ============================================================================
// FUNCIONES AUXILIARES PARA CONSULTAS RÁPIDAS
// ============================================================================

// Consulta rápida por ID de subcluster
vector<Punto> GeoCluster::buscarPorSubcluster(int id_subcluster, MBR& rango) {
    vector<Punto> puntos_en_rango;
    searchRec(rango, raiz, puntos_en_rango);
    
    vector<Punto> resultado;
    for (const auto& punto : puntos_en_rango) {
        if (punto.id_subcluster_atributivo == id_subcluster) {
            resultado.push_back(punto);
        }
    }
    
    return resultado;
}

// Consulta rápida por rango de similitud
vector<Punto> GeoCluster::buscarPorSimilitud(const Punto& referencia, double umbral_similitud, MBR& rango) {
    vector<Punto> puntos_en_rango;
    searchRec(rango, raiz, puntos_en_rango);
    
    vector<Punto> resultado;
    for (const auto& punto : puntos_en_rango) {
        double similitud = calcularSimilitudAtributos(referencia, punto);
        if (similitud >= umbral_similitud) {
            resultado.push_back(punto);
        }
    }
    
    return resultado;
}

// ============================================================================
// FUNCIONES DE CONSULTA PRINCIPALES
// ============================================================================

// ============================================================================
// FUNCIONES AUXILIARES PARA CONSULTAS
// ============================================================================

double GeoCluster::calcularSimilitudAtributos(const Punto& p1, const Punto& p2) {
    // Calcular similitud basada en atributos PCA usando distancia euclidiana
    double distancia = 0.0;
    
    // Usar solo los primeros 8 componentes PCA (si están disponibles)
    int num_atributos = min(8, (int)min(p1.atributos.size(), p2.atributos.size()));
    
    for (int i = 0; i < num_atributos; i++) {
        double diff = p1.atributos[i] - p2.atributos[i];
        distancia += diff * diff;
    }
    
    // Convertir distancia a similitud (e^(-distancia))
    // Esto dará valores entre 0 y 1, donde:
    // - Puntos idénticos tendrán similitud = 1
    // - Puntos muy diferentes tendrán similitud cercana a 0
    return exp(-sqrt(distancia));
}

// ============================================================================
// FUNCIONES PARA MICROCLUSTERS
// ============================================================================

void GeoCluster::crearMicroclustersEnHojas() {
    cout << "\n🔬 Creando microclusters en hojas del R*-Tree..." << endl;
    crearMicroclustersRec(raiz);
    cout << "✅ Microclusters creados en todas las hojas" << endl;
}

void GeoCluster::mostrarEstadisticasMicroclusters() {
    cout << "\nESTADÍSTICAS DE MICROCLUSTERS" << endl;
    cout << "=================================" << endl;
    
    int total_microclusters = 0;
    int total_puntos_en_microclusters = 0;
    unordered_map<string, int> combinaciones_clusters;
    
    // Función recursiva para contar microclusters
    function<void(Nodo*)> contarMicroclusters = [&](Nodo* nodo) {
        if (nodo == nullptr) return;
        
        if (nodo->esHoja) {
            total_microclusters += nodo->microclusters_en_Hoja.size();
            
            for (const auto& mc : nodo->microclusters_en_Hoja) {
                total_puntos_en_microclusters += mc.puntos.size();
                
                // Contar combinaciones únicas de cluster_geo + subcluster_attr
                string combinacion = to_string(mc.id_cluster_geografico) + "_" + 
                                   to_string(mc.id_subcluster_atributivo);
                combinaciones_clusters[combinacion]++;
            }
        } else {
            for (Nodo* hijo : nodo->hijos) {
                contarMicroclusters(hijo);
            }
        }
    };
    
    contarMicroclusters(raiz);
    
    cout << "Total de microclusters creados: " << total_microclusters << endl;
    cout << "Total de puntos en microclusters: " << total_puntos_en_microclusters << endl;
    cout << "Combinaciones únicas (cluster_geo_subcluster_attr): " << combinaciones_clusters.size() << endl;
    
    // Mostrar las combinaciones más frecuentes
    vector<pair<string, int>> combinaciones_ordenadas;
    for (const auto& par : combinaciones_clusters) {
        combinaciones_ordenadas.push_back(par);
    }
    
    sort(combinaciones_ordenadas.begin(), combinaciones_ordenadas.end(),
         [](const pair<string, int>& a, const pair<string, int>& b) {
             return a.second > b.second;
         });
    
    cout << "\nTop 10 combinaciones más frecuentes:" << endl;
    for (int i = 0; i < min(10, (int)combinaciones_ordenadas.size()); i++) {
        cout << "  " << i+1 << ". " << combinaciones_ordenadas[i].first 
             << " (aparece en " << combinaciones_ordenadas[i].second << " nodos hoja)" << endl;
    }
}

void GeoCluster::crearMicroclustersRec(Nodo* nodo) {
    if (nodo == nullptr) return;
    
    if (nodo->esHoja) {
        // Crear microclusters basados en subclusters atributivos
        crearMicroclustersEnNodoHoja(nodo);
    } else {
        // Recursión para nodos internos
        for (Nodo* hijo : nodo->hijos) {
            crearMicroclustersRec(hijo);
        }
    }
}

void GeoCluster::crearMicroclustersEnNodoHoja(Nodo* nodo_hoja) {
    if (nodo_hoja->puntos.empty()) return;
    
    // Agrupar puntos por combinación de cluster geográfico y subcluster atributivo
    // Usar una clave compuesta: "cluster_geo_subcluster_attr"
    unordered_map<string, vector<Punto>> puntos_por_microcluster;
    
    for (const auto& point_id : nodo_hoja->puntos) {
        // Recuperar el punto completo (id=0 es válido: comprobar con find)
        auto it = puntos_completos.find(point_id.id);
        if (it == puntos_completos.end()) continue;
        const Punto& punto = it->second;
        // Crear clave combinada: cluster_geografico + "_" + subcluster_atributivo
        string clave_microcluster = to_string(punto.id_cluster_geografico) + "_" +
                                   to_string(punto.id_subcluster_atributivo);
        puntos_por_microcluster[clave_microcluster].push_back(punto);
    }
    
    // Crear microclusters para cada combinación única
    for (const auto& par : puntos_por_microcluster) {
        const string& clave_microcluster = par.first;
        const vector<Punto>& puntos_grupo = par.second;
        
        if (puntos_grupo.size() < 2) continue;  // Microclusters con al menos 2 puntos
        
        // Extraer IDs de la clave
        size_t pos_guion = clave_microcluster.find('_');
        int id_cluster_geo = stoi(clave_microcluster.substr(0, pos_guion));
        int id_subcluster_attr = stoi(clave_microcluster.substr(pos_guion + 1));
        
        // Calcular centro del microcluster
        vector<double> centro_atributos;
        if (!puntos_grupo.empty() && !puntos_grupo[0].atributos.empty()) {
            int num_atributos = puntos_grupo[0].atributos.size();
            centro_atributos.resize(num_atributos, 0.0);
            
            for (const auto& punto : puntos_grupo) {
                for (int i = 0; i < num_atributos; i++) {
                    centro_atributos[i] += punto.atributos[i];
                }
            }
            
            // Promedio
            for (int i = 0; i < num_atributos; i++) {
                centro_atributos[i] /= puntos_grupo.size();
            }
        }
        
        // Calcular radio del microcluster
        double radio = 0.0;
        for (const auto& punto : puntos_grupo) {
            double dist = calcularDistanciaAtributos(centro_atributos, punto.atributos);
            radio = max(radio, dist);
        }
        
        // Crear microcluster con ambos IDs (cluster geográfico y subcluster atributivo)
        MicroCluster microcluster(centro_atributos, radio, id_cluster_geo, id_subcluster_attr);
        microcluster.puntos = puntos_grupo;
        
        nodo_hoja->microclusters_en_Hoja.push_back(microcluster);
    }
    
    cout << "Nodo hoja: " << nodo_hoja->microclusters_en_Hoja.size() 
         << " microclusters creados de " << nodo_hoja->puntos.size() << " puntos" << endl;
    
    // Mostrar estadísticas de los microclusters creados
    for (size_t i = 0; i < nodo_hoja->microclusters_en_Hoja.size(); i++) {
        const auto& mc = nodo_hoja->microclusters_en_Hoja[i];
        cout << "    Microcluster " << i+1 << ": " << mc.puntos.size() 
             << " puntos, cluster_geo=" << mc.id_cluster_geografico 
             << ", subcluster_attr=" << mc.id_subcluster_atributivo 
             << ", radio=" << fixed << setprecision(4) << mc.radio << endl;
    }
}

double GeoCluster::calcularDistanciaAtributos(const vector<double>& centro, const vector<double>& atributos) {
    double distancia = 0.0;
    int num_atributos = min(centro.size(), atributos.size());
    
    for (int i = 0; i < num_atributos; i++) {
        double diff = centro[i] - atributos[i];
        distancia += diff * diff;
    }
    
    return sqrt(distancia);
}

// ============================================================================
// FUNCIONES DE VERIFICACIÓN Y DEBUG
// ============================================================================

// ============================================================================
// FUNCIONES AUXILIARES OPTIMIZADAS
// ============================================================================

// ============================================================================
// FUNCIONES AUXILIARES OPTIMIZADAS
// ============================================================================

Nodo* GeoCluster::encontrarNodoHoja(const Punto& punto) {
    if (raiz == nullptr) return nullptr;
    
    Nodo* nodo_actual = raiz;
    
    // Navegar hasta encontrar un nodo hoja
    while (!nodo_actual->esHoja) {
        Nodo* mejor_hijo = nullptr;
        double min_distancia = numeric_limits<double>::infinity();
        
        // Encontrar el hijo más cercano al punto
        for (Nodo* hijo : nodo_actual->hijos) {
            // Calcular distancia del punto al centro del MBR del hijo
            double centro_lat = (hijo->mbr.m_minp[0] + hijo->mbr.m_maxp[0]) / 2.0;
            double centro_lon = (hijo->mbr.m_minp[1] + hijo->mbr.m_maxp[1]) / 2.0;
            
            double distancia = sqrt(pow(punto.latitud - centro_lat, 2) + pow(punto.longitud - centro_lon, 2));
            
            if (distancia < min_distancia) {
                min_distancia = distancia;
                mejor_hijo = hijo;
            }
        }
        
        if (mejor_hijo == nullptr) break;
        nodo_actual = mejor_hijo;
    }
    
    return nodo_actual;
}

vector<Nodo*> GeoCluster::obtenerNodosHermanosEnRango(Nodo* nodo_actual, const MBR& rango) {
    vector<Nodo*> nodos_hermanos;
    
    if (nodo_actual == nullptr || nodo_actual->padre == nullptr) {
        return nodos_hermanos;
    }
    
    Nodo* padre = nodo_actual->padre;
    
    // Buscar hermanos que intersecten con el rango
    for (Nodo* hermano : padre->hijos) {
        if (hermano != nodo_actual && hermano->esHoja) {
            // Verificar si el MBR del hermano intersecta con el rango
            if (interseccionMBR(hermano->mbr, rango)) {
                nodos_hermanos.push_back(hermano);
            }
        }
    }
    
    return nodos_hermanos;
}

MBR GeoCluster::calcularMBR(const vector<Punto>& puntos) {
    if (puntos.empty()) {
        return MBR();
    }
    
    double minLat = puntos[0].latitud, maxLat = puntos[0].latitud;
    double minLon = puntos[0].longitud, maxLon = puntos[0].longitud;
    
    for (const auto& punto : puntos) {
        minLat = std::min(minLat, punto.latitud);
        minLon = std::min(minLon, punto.longitud);
        maxLat = std::max(maxLat, punto.latitud);
        maxLon = std::max(maxLon, punto.longitud);
    }

    return MBR(minLat, minLon, maxLat, maxLon);
}

// NUEVA: Función para calcular MBR con PointID
MBR GeoCluster::calcularMBR(const vector<PointID>& puntos) {
    if (puntos.empty()) {
        return MBR();
    }
    
    double minLat = puntos[0].latitud, maxLat = puntos[0].latitud;
    double minLon = puntos[0].longitud, maxLon = puntos[0].longitud;
    
    for (const auto& punto : puntos) {
        minLat = std::min(minLat, punto.latitud);
        minLon = std::min(minLon, punto.longitud);
        maxLat = std::max(maxLat, punto.latitud);
        maxLon = std::max(maxLon, punto.longitud);
    }

    return MBR(minLat, minLon, maxLat, maxLon);
}

// ============================================================================
// NUEVAS FUNCIONES OPTIMIZADAS POR SIMILITUD DE CLUSTER
// ============================================================================

vector<Punto> GeoCluster::buscarPuntosPorSimilitudCluster(const Punto& punto_referencia, const MBR& rango, int n_puntos) {
    vector<Punto> resultado;
    
    // Buscar todos los puntos en el rango
    vector<Punto> puntos_en_rango = buscarPuntosDentroInterseccion(rango, raiz);
    if (puntos_en_rango.empty()) {
        return resultado;
    }
    
    // Listas por prioridad
    vector<Punto> lista1, lista2, lista3;
    for (const auto& punto : puntos_en_rango) {
        if (punto.id == punto_referencia.id) continue; // Excluir el punto de referencia
        if (punto.id_cluster_geografico == punto_referencia.id_cluster_geografico &&
            punto.id_subcluster_atributivo == punto_referencia.id_subcluster_atributivo) {
            lista1.push_back(punto);
        } else if (punto.id_cluster_geografico == punto_referencia.id_cluster_geografico) {
            lista2.push_back(punto);
        } else {
            lista3.push_back(punto);
        }
    }
    // Concatenar listas en orden de prioridad
    vector<Punto> candidatos;
    for (const auto& p : lista1) candidatos.push_back(p);
    for (const auto& p : lista2) candidatos.push_back(p);
    for (const auto& p : lista3) candidatos.push_back(p);
    // Limitar a N candidatos
    if ((int)candidatos.size() > n_puntos) {
        candidatos.resize(n_puntos);
    }
    // Calcular distancia euclidiana solo para los N candidatos
    struct PuntoDist {
        Punto punto;
        double dist;
        PuntoDist(const Punto& p, double d) : punto(p), dist(d) {}
        bool operator<(const PuntoDist& o) const { return dist < o.dist; }
    };
    vector<PuntoDist> ordenados;
    for (const auto& p : candidatos) {
        double d = sqrt(pow(p.latitud - punto_referencia.latitud, 2) + pow(p.longitud - punto_referencia.longitud, 2));
        ordenados.emplace_back(p, d);
    }
    sort(ordenados.begin(), ordenados.end());
    // Retornar los N más cercanos
    for (const auto& pd : ordenados) {
        resultado.push_back(pd.punto);
    }
    return resultado;
}
