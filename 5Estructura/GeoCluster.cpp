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

using namespace std;

// Configurar precisión global para todos los números de punto flotante
void configurarPrecision() {
    cout.setf(ios::fixed, ios::floatfield);
    cout.precision(14);
}

GeoCluster::GeoCluster() {
    cout << "Constructor llamado" << endl;
    raiz = nullptr;
    configurarPrecision();  // Configurar precisión para todos los números
}

GeoCluster::~GeoCluster() {
    delete raiz;
    std::cout << "Destructor llamado" << std::endl;
}

void GeoCluster::inserData(const Punto& punto_a_insertar){
    niveles_reinsert.clear();
    insertar(punto_a_insertar,0);
}

void GeoCluster::insertar(const Punto& punto, int nivel) {
    //I1: Llamar a chooseSubTree para encontrar el mejor nodo hoja
    if(raiz == nullptr){
        raiz = new Nodo(true);
        raiz->m_nivel = 0;
    }
    Nodo* N = chooseSubTree(raiz,punto,nivel);

    // Debug: Imprimir información de inserción
    cout << "  Debug: Insertando punto " << punto.id << " en nodo nivel " << N->m_nivel 
         << " (esHoja: " << N->esHoja << ", puntos actuales: " << N->puntos.size() << ")" << endl;

    //I2: Si N tiene menos entradas que M, insertar el punto en N
    if (N->puntos.size() < MAX_PUNTOS_POR_NODO){
        insertIntoLeafNode(N,punto);
        updateMBR(N);
        cout << "  Debug: Punto " << punto.id << " insertado directamente. Total en nodo: " << N->puntos.size() << endl;
    } 
    //S1: Si N tiene M entradas, llamar a OverFlowTreatment
    else{
        cout << "  Debug: Nodo lleno, llamando OverFlowTreatment para punto " << punto.id << endl;
        bool splite_ocurrido = OverFlowTreatment(N,punto,nivel);
        if(splite_ocurrido){
            updateMBR(N);
            cout << "  Debug: Split ocurrido para punto " << punto.id << endl;
        } 
    }
}

Nodo* GeoCluster::chooseSubTree(Nodo* N, const Punto punto, int nivel){
    // CS1: Ponemos a N como la raiz (IMPLICITAMENTE)
    // CS2: Si N es hoja, retorna N
    if (N->esHoja) {
        return N;
    }
    Nodo* mejor_nodo = nullptr;

    // Si los punteros hijos apuntan a hojas
    if(all_of(N->hijos.begin(), N->hijos.end(), [](Nodo* hijo){return hijo->esHoja;})){
        double minOverlap = numeric_limits<double>::infinity();
        for (auto& hijo: N->hijos){
            double costoOverlap = calcularCostoOverlap(hijo->mbr, punto);
            if (costoOverlap < minOverlap){
                minOverlap = costoOverlap;
                mejor_nodo = hijo;
            }
            else if(costoOverlap == minOverlap){
                double CostoAreaAmpliacion = calcularCostoDeAmpliacionArea(hijo->mbr, punto);
                if(CostoAreaAmpliacion < calcularCostoDeAmpliacionArea(mejor_nodo->mbr, punto)){
                    mejor_nodo = hijo;
                }
            }
        }
    }
    // Si los punteros hijos apuntan a nodos internos
    else{
        double minArea = numeric_limits<double>::infinity();
        for (auto& hijo : N->hijos){
            double CostoMinArea = calcularCostoDeAmpliacionArea(hijo->mbr, punto);
            if(CostoMinArea < minArea){
                minArea = CostoMinArea;
                mejor_nodo = hijo;
            }
            else if(CostoMinArea == minArea){
                if(calcularMBRArea(hijo->mbr) < calcularMBRArea(mejor_nodo->mbr)){
                    mejor_nodo = hijo;
                }
            }
        }  
    }
    
    // CS3: Seleccionar N como el nodo hijo y repetir desde CS2
    if (mejor_nodo != nullptr) {
        return chooseSubTree(mejor_nodo, punto, nivel);
    } 
    // Fallback: si no se encontró mejor nodo, retornar el primer hijo
    return N->hijos.empty() ? N : N->hijos[0];
}

bool GeoCluster::OverFlowTreatment(Nodo* N, const Punto& punto, int nivel) {
    // OVT1: Si no es raíz y es primera vez en este nivel, hacer reinsert
    if (N != raiz && niveles_reinsert.find(nivel) == niveles_reinsert.end()) {
        niveles_reinsert[nivel] = true;
        reinsert(N, punto);
        return false;  // No hubo split
    } else {
        // Si ya se hizo reinsert en este nivel o es raíz, hacer split
        Nodo* nuevo_nodo = nullptr;
        Split(N, nuevo_nodo);

        // Después del split, necesitamos insertar el punto que causó el overflow
        // Primero, agregar el punto al nodo original si hay espacio
        if (N->puntos.size() < MAX_PUNTOS_POR_NODO) {
            insertIntoLeafNode(N, punto);
            updateMBR(N);
            cout << "  Debug: Punto " << punto.id << " insertado en nodo original después del split" << endl;
        } else {
            // Si el nodo original está lleno, insertar en el nuevo nodo
            insertIntoLeafNode(nuevo_nodo, punto);
            updateMBR(nuevo_nodo);
            cout << "  Debug: Punto " << punto.id << " insertado en nuevo nodo después del split" << endl;
        }

        propagateSplit(N, nuevo_nodo);
        return true;  // Hubo split
    }
}

void GeoCluster::reinsert(Nodo* N, const Punto& punto) {
    cout << "  Debug: Iniciando reinsert para nodo con " << N->puntos.size() << " puntos" << endl;
    
    vector<pair<double, Punto>> distancias;
    
    /*
    RI1: Calcular distancias al centro del nodo N, con los mbr
         de cada MBR que tiene el nodo N
    */ 
    Punto centro = calcularCentroNodo(N);
    for (const auto& p : N->puntos) {
        double dist = calcularDistancia(p, centro);
        distancias.push_back({dist, p});
        cout << "  Debug: Punto " << p.id << " a distancia " << dist << " del centro" << endl;
    }
    
    // RI2: Ordenar por distancia
    sort(distancias.begin(), distancias.end(),greater<pair<double, Punto>>());
    
    // RI3: Remover 30% más lejanos
    int p = static_cast<int>(N->puntos.size() * PORCENTAJE_PARA_HACER_REINSERT);
    vector<Punto> puntos_a_reinsertar;
    
    cout << "  Debug: Removiendo " << p << " puntos más lejanos para reinsertar" << endl;
    
    for (int i = 0; i < p; i++) {
        puntos_a_reinsertar.push_back(distancias[i].second);
        cout << "  Debug: Punto " << distancias[i].second.id << " marcado para reinsertar" << endl;
    }
    
    // Actualizar puntos en N (quedarse con los más cercanos)
    vector<Punto> puntos_restantes;
    for (int i = p; i < distancias.size(); i++) {
        puntos_restantes.push_back(distancias[i].second);
        cout << "  Debug: Punto " << distancias[i].second.id << " se queda en el nodo" << endl;
    }
    N->puntos = puntos_restantes;
    
    // Actualizar MBR
    updateMBR(N);
    
    cout << "  Debug: Reinsertando " << puntos_a_reinsertar.size() << " puntos" << endl;
    
    // RI4: Reinsertar puntos (empezando por los más lejanos - reinserción lejana)
    for (const auto& p : puntos_a_reinsertar) {
        cout << "  Debug: Reinsertando punto " << p.id << endl;
        insertar(p, N->m_nivel);
    }
    
    // IMPORTANTE: Después del reinsert, necesitamos insertar el punto original que causó el overflow
    cout << "  Debug: Insertando punto original " << punto.id << " después del reinsert" << endl;
    insertar(punto, N->m_nivel);
}

Punto GeoCluster::calcularCentroNodo(Nodo* nodo) {
    // Calcula el centro del MBR del nodo
    double centroLat = (nodo->mbr.m_minp[0] + nodo->mbr.m_maxp[0]) / 2.0;
    double centroLon = (nodo->mbr.m_minp[1] + nodo->mbr.m_maxp[1]) / 2.0;
    
    // Crear un punto con el centro (id = -1 para indicar que es centro)
    Punto centro;
    centro.id = -1;
    centro.latitud = centroLat;
    centro.longitud = centroLon;
    centro.atributos = {}; // Centro no tiene atributos específicos
    
    return centro;
}

double GeoCluster::calcularDistancia(const Punto& p1, const Punto& p2) {
    // Distancia euclidiana en el espacio lat/lon
    double deltaLat = p1.latitud - p2.latitud;
    double deltaLon = p1.longitud - p2.longitud;
    
    return sqrt(deltaLat * deltaLat + deltaLon * deltaLon);
}

Nodo* GeoCluster::encontrarPadre(Nodo* raiz, Nodo* hijo) {
    if (raiz == nullptr || hijo == nullptr) {
        return nullptr;
    }
    
    // Si la raíz es el padre directo
    for (auto& hijo_raiz : raiz->hijos) {
        if (hijo_raiz == hijo) {
            return raiz;
        }
    }
    
    // Buscar recursivamente en los hijos
    for (auto& hijo_raiz : raiz->hijos) {
        Nodo* padre = encontrarPadre(hijo_raiz, hijo);
        if (padre != nullptr) {
            return padre;
        }
    }
    
    return nullptr;
}

void GeoCluster::propagateSplit(Nodo* nodo_original, Nodo* nuevo_nodo) {
    cout << "  Debug: === PROPAGANDO SPLIT ===" << endl;
    cout << "    Nodo original: " << nodo_original->puntos.size() << " puntos" << endl;
    cout << "    Nuevo nodo: " << nuevo_nodo->puntos.size() << " puntos" << endl;
    
    if (nodo_original == raiz) {
        // Crear nueva raíz
        Nodo* nueva_raiz = new Nodo(false);
        nueva_raiz->hijos.push_back(nodo_original);
        nueva_raiz->hijos.push_back(nuevo_nodo);
        nueva_raiz->m_nivel = nodo_original->m_nivel + 1;
        
        cout << "    Creando nueva raíz con 2 hijos:" << endl;
        cout << "      Hijo 0: " << nodo_original->puntos.size() << " puntos" << endl;
        cout << "      Hijo 1: " << nuevo_nodo->puntos.size() << " puntos" << endl;
        
        // Actualizar MBR de la nueva raíz
        nueva_raiz->mbr = nodo_original->mbr;
        nueva_raiz->mbr.stretch(nuevo_nodo->mbr);
        
        raiz = nueva_raiz;
        cout << "    Nueva raíz creada en nivel " << nueva_raiz->m_nivel << endl;
    } else {
        // Encontrar padre y actualizar
        Nodo* padre = encontrarPadre(raiz, nodo_original);
        if (padre != nullptr) {
            int indice_hijo = padre->hijos.size();
            padre->hijos.push_back(nuevo_nodo);
            updateMBR(padre);
            
            cout << "    Agregando hijo " << indice_hijo << " con " 
                 << nuevo_nodo->puntos.size() << " puntos al padre (nivel " << padre->m_nivel << ")" << endl;
            cout << "    Padre ahora tiene " << padre->hijos.size() << " hijos" << endl;
            
            // Si el padre se desborda, aplicar overflow treatment
            if (padre->hijos.size() > MAX_PUNTOS_POR_NODO) {
                cout << "    ¡Padre se desborda! Aplicando overflow treatment..." << endl;
                // Crear un punto dummy para la llamada
                Punto punto_dummy;
                punto_dummy.id = -1;
                punto_dummy.latitud = 0.0;
                punto_dummy.longitud = 0.0;
                OverFlowTreatment(padre, punto_dummy, padre->m_nivel);
            }
        } else {
            cout << "    ERROR: No se encontró padre para el nodo" << endl;
        }
    }
    cout << "  Debug: === FIN PROPAGACIÓN SPLIT ===" << endl;
}

void GeoCluster::Split(Nodo* nodo, Nodo*& nuevo_nodo) {
    // S1: Elegir eje de división
    int eje = chooseSplitAxis(nodo->puntos);
    
    // S2: Elegir índice de división
    int split_index = chooseSplitIndex(nodo->puntos, eje);
    
    // S3: Distribuir entradas
    nuevo_nodo = new Nodo(true); // Siempre es hoja para puntos
    nuevo_nodo->m_nivel = nodo->m_nivel;
    
    vector<Punto> grupo1(nodo->puntos.begin(), nodo->puntos.begin() + split_index);
    vector<Punto> grupo2(nodo->puntos.begin() + split_index, nodo->puntos.end());
    
    // Verificar que no se pierdan puntos
    if (grupo1.size() + grupo2.size() != nodo->puntos.size()) {
        cout << "ERROR: Se perdieron puntos en el split!" << endl;
        cout << "Original: " << nodo->puntos.size() << ", Grupo1: " << grupo1.size() 
             << ", Grupo2: " << grupo2.size() << endl;
    }
    
    nodo->puntos = grupo1;
    nuevo_nodo->puntos = grupo2;
    
    // Actualizar MBRs
    updateMBR(nodo);
    updateMBR(nuevo_nodo);
}

bool interseccion_mbr_microcluster(const MicroCluster& mbr1, const MBR& rect) {
    double lat = mbr1.centroId[0];
    double lon = mbr1.centroId[1];

    return !(lon < rect.m_minp[1] || lon > rect.m_maxp[1] ||
             lat < rect.m_minp[0]  || lat > rect.m_maxp[0]);
}

double GeoCluster::calcularCostoOverlap(const MBR& mbr, const Punto& punto){
    double latMin = min(mbr.m_minp[0], punto.latitud);
    double longMin = min(mbr.m_minp[1], punto.longitud);
    double latMax = max(mbr.m_maxp[0], punto.latitud);
    double longMax = max(mbr.m_maxp[1], punto.longitud);

    double nuevaArea = (latMax-latMin) * (longMax-longMin);
    double areaOriginal = (mbr.m_maxp[0]- mbr.m_minp[0]) * (mbr.m_maxp[1]- mbr.m_minp[1]);
    return nuevaArea - areaOriginal;
}

double GeoCluster::calcularCostoDeAmpliacionArea(const MBR& mbr, const Punto& punto){
    double latMin = min(mbr.m_minp[0], punto.latitud);
    double longMin = min(mbr.m_minp[1], punto.longitud);
    double latMax = max(mbr.m_maxp[0], punto.latitud);
    double longMax = max(mbr.m_maxp[1], punto.longitud);

    return (latMax - latMin) * (longMax - longMin);
}

double GeoCluster::calcularMBRArea(const MBR& mbr) {
    return (mbr.m_maxp[0] - mbr.m_minp[0]) * (mbr.m_maxp[1] - mbr.m_minp[1]);
}

double GeoCluster::calcularOverlap(const MBR& mbr1, const MBR& mbr2) {
    double lat_overlap = std::max(0.0, std::min(mbr1.m_maxp[0], mbr2.m_maxp[0]) - std::max(mbr1.m_minp[0], mbr2.m_minp[0]));
    double lon_overlap = std::max(0.0, std::min(mbr1.m_maxp[1], mbr2.m_maxp[1]) - std::max(mbr1.m_minp[1], mbr2.m_minp[1]));

    return lat_overlap * lon_overlap;
}

double GeoCluster::calcularMargen(const MBR& mbr) {
    double latitudLongitud = mbr.m_maxp[0] - mbr.m_minp[0];
    double longitudLongitud = mbr.m_maxp[1] - mbr.m_minp[1];

    return 2 * (latitudLongitud + longitudLongitud);
}

MBR GeoCluster::calcularMBR(const vector<Punto>& puntos) {
    if (puntos.empty()) {
        return MBR(0, 0, 0, 0);
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

int GeoCluster::chooseSplitAxis(const vector<Punto>& puntos){
    vector<Punto> puntosLatitud = puntos;
    vector<Punto> puntosLongitud = puntos;

    sort(puntosLatitud.begin(), puntosLatitud.end(), [](const Punto& p1, const Punto& p2) {
        return p1.latitud < p2.latitud;
    });

    sort(puntosLongitud.begin(), puntosLongitud.end(), [](const Punto& p1, const Punto& p2) {
        return p1.longitud < p2.longitud;
    });

    MBR mbrLatitud = calcularMBR(puntosLatitud);
    MBR mbrLongitud = calcularMBR(puntosLongitud);

    double margenLat = calcularMargen(mbrLatitud);
    double margenLong = calcularMargen(mbrLongitud);

    return (margenLat < margenLong) ? 0 : 1;
}

int GeoCluster::chooseSplitIndex(vector<Punto>& puntos, int eje) {
    int M = puntos.size();
    int m = MIN_PUNTOS_POR_NODO;

    if (eje == 0) {
        sort(puntos.begin(), puntos.end(), [](const Punto& p1, const Punto& p2) {
            return p1.latitud < p2.latitud;
        });
    } else {
        sort(puntos.begin(), puntos.end(), [](const Punto& p1, const Punto& p2) {
            return p1.longitud < p2.longitud;
        });
    }

    int numero_de_distribuciones = M - 2 * m + 2;
    double minOverlap = numeric_limits<double>::infinity();
    double minMargen = numeric_limits<double>::infinity();
    double minArea = numeric_limits<double>::infinity();
    int bestSplitIndex = m;

    for (int i = 0; i < numero_de_distribuciones; ++i) {
        int grupo1_size = m + i;
        
        vector<Punto> grupo1(puntos.begin(), puntos.begin() + grupo1_size);
        vector<Punto> grupo2(puntos.begin() + grupo1_size, puntos.end());

        MBR mbrGrupo1 = calcularMBR(grupo1);
        MBR mbrGrupo2 = calcularMBR(grupo2);

        double overlap = calcularOverlap(mbrGrupo1, mbrGrupo2);
        double margen = calcularMargen(mbrGrupo1) + calcularMargen(mbrGrupo2);
        double area = calcularMBRArea(mbrGrupo1) + calcularMBRArea(mbrGrupo2);

        if (overlap < minOverlap) {
            minOverlap = overlap;
            bestSplitIndex = grupo1_size;
            minMargen = margen;
            minArea = area;
        } else if (overlap == minOverlap) {
            if (margen < minMargen) {
                minMargen = margen;
                bestSplitIndex = grupo1_size;
                minArea = area;
            } else if (margen == minMargen) {
                if (area < minArea) {
                    minArea = area;
                    bestSplitIndex = grupo1_size;
                }
            }
        }
    }

    return bestSplitIndex;
}

void GeoCluster::insertIntoLeafNode(Nodo* nodo_hoja, const Punto& punto) {
    nodo_hoja->puntos.push_back(punto);
    nodo_hoja->mbr = calcularMBR(nodo_hoja->puntos);
}

void GeoCluster::insertIntoParent(Nodo* padre, Nodo* nuevo_nodo) {
    if (padre == nullptr) {
        // Crear nueva raíz
        Nodo* nueva_raiz = new Nodo(false);
        nueva_raiz->hijos.push_back(raiz);
        nueva_raiz->hijos.push_back(nuevo_nodo);
        nueva_raiz->m_nivel = raiz->m_nivel + 1;
        
        // Calcular MBR de la nueva raíz
        nueva_raiz->mbr = raiz->mbr;
        nueva_raiz->mbr.stretch(nuevo_nodo->mbr);
        
        raiz = nueva_raiz;
        return;
    }
    
    padre->hijos.push_back(nuevo_nodo);
    updateMBR(padre);
    
    if (padre->hijos.size() > MAX_PUNTOS_POR_NODO) {
        // Necesitaríamos implementar split para nodos internos
        cout << "Warning: Nodo interno excede capacidad - split no implementado" << endl;
    }
}

void GeoCluster::updateMBR(Nodo* nodo) {
    if (nodo->esHoja) {
        nodo->mbr = calcularMBR(nodo->puntos);
    } else {
        if (!nodo->hijos.empty()) {
            nodo->mbr = nodo->hijos[0]->mbr;
            for (size_t i = 1; i < nodo->hijos.size(); i++) {
                nodo->mbr.stretch(nodo->hijos[i]->mbr);
            }
        }
    }
}

double GeoCluster::computeArea(double ax1, double ay1, double ax2, double ay2,
                              double bx1, double by1, double bx2, double by2) {
    double interseccion_minLat = max(ax1, bx1);
    double interseccion_minLon = max(ay1, by1);
    double interseccion_maxLat = min(ax2, bx2);
    double interseccion_maxLon = min(ay2, by2);

    if (interseccion_minLat >= interseccion_maxLat || interseccion_minLon >= interseccion_maxLon) {
        return 0.0;
    }

    return (interseccion_maxLat - interseccion_minLat) * (interseccion_maxLon - interseccion_minLon);
}

MBR GeoCluster::computeMBR(double ax1, double ay1, double ax2, double ay2,
                          double bx1, double by1, double bx2, double by2){
    double interseccion_minLat = max(ax1, bx1);
    double interseccion_minLon = max(ay1, by1);
    double interseccion_maxLat = min(ax2, bx2);
    double interseccion_maxLon = min(ay2, by2);

    if (interseccion_minLat >= interseccion_maxLat || interseccion_minLon >= interseccion_maxLon) {
        return MBR();
    }

    return MBR(interseccion_minLat, interseccion_minLon, interseccion_maxLat, interseccion_maxLon);
}

bool GeoCluster::estaDentroDelMBR(const Punto& punto, const MBR& mbr) {
    bool dentroLatitud = (punto.latitud >= mbr.m_minp[0] && punto.latitud <= mbr.m_maxp[0]);
    bool dentroLongitud = (punto.longitud >= mbr.m_minp[1] && punto.longitud <= mbr.m_maxp[1]);

    return dentroLatitud && dentroLongitud;
}

bool GeoCluster::interseccionMBR(const MBR& mbr1, const MBR& mbr2) {
    return mbr1.is_intersected(mbr2);
}

vector<Punto> GeoCluster::buscarPuntosDentroInterseccion(const MBR& rango, Nodo* nodo) {
    vector<Punto> puntosEncontrados;

    if (!interseccionMBR(nodo->mbr, rango)) {
        return puntosEncontrados;
    }

    if (nodo->esHoja) {
        for (const auto& punto : nodo->puntos) {
            if (estaDentroDelMBR(punto, rango)) {
                puntosEncontrados.push_back(punto);
            }
        }
    } else {
        for (Nodo* hijo : nodo->hijos) {
            vector<Punto> puntosHijo = buscarPuntosDentroInterseccion(rango, hijo);
            puntosEncontrados.insert(puntosEncontrados.end(), puntosHijo.begin(), puntosHijo.end());
        }
    }

    return puntosEncontrados;
}

void GeoCluster::searchRec(const MBR& rango, Nodo* nodo, vector<Punto>& puntos_similares) {
    if (interseccionMBR(nodo->mbr, rango)) {
        if (nodo->esHoja) {
            for (const auto& punto : nodo->puntos) {
                if (estaDentroDelMBR(punto, rango)) {
                    puntos_similares.push_back(punto);
                }
            }
        } else {
            for (Nodo* hijo : nodo->hijos) {
                searchRec(rango, hijo, puntos_similares);
            }
        }
    }
}

int GeoCluster::contarPuntosEnArbol(Nodo* nodo) {
    if (nodo == nullptr) {
        nodo = raiz;
    }
    
    if (nodo == nullptr) {
        return 0;
    }
    
    if (nodo->esHoja) {
        return nodo->puntos.size();
    } else {
        int total = 0;
        for (Nodo* hijo : nodo->hijos) {
            total += contarPuntosEnArbol(hijo);
        }
        return total;
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
    
    string indent(nivel * 2, ' ');
    cout << indent << "Nivel " << nivel << " - ";
    
    if (nodo->esHoja) {
        cout << "HOJA - " << nodo->puntos.size() << " puntos" << endl;
        cout << indent << "  MBR: (" << nodo->mbr.m_minp[0] << "," << nodo->mbr.m_minp[1] << ") a ("
             << nodo->mbr.m_maxp[0] << "," << nodo->mbr.m_maxp[1] << ")" << endl;
        for (const auto& punto : nodo->puntos) {
            cout << indent << "    Punto " << punto.id << ": (" << punto.latitud << "," << punto.longitud << ")" << endl;
        }
    } else {
        cout << "INTERNO - " << nodo->hijos.size() << " hijos" << endl;
        cout << indent << "  MBR: (" << nodo->mbr.m_minp[0] << "," << nodo->mbr.m_minp[1] << ") a ("
             << nodo->mbr.m_maxp[0] << "," << nodo->mbr.m_maxp[1] << ")" << endl;
        for (size_t i = 0; i < nodo->hijos.size(); i++) {
            cout << indent << "  Hijo " << i << ":" << endl;
            imprimirArbol(nodo->hijos[i], nivel + 1);
        }
    }
}


/*
void GeoCluster::verificarDuplicados() {
    vector<int> ids_encontrados;
    verificarDuplicadosRec(raiz, ids_encontrados);
    
    cout << "\n=== VERIFICACIÓN DE DUPLICADOS ===" << endl;
    cout << "Total de IDs únicos encontrados: " << ids_encontrados.size() << endl;
    
    // Ordenar y verificar duplicados
    sort(ids_encontrados.begin(), ids_encontrados.end());
    for (size_t i = 1; i < ids_encontrados.size(); i++) {
        if (ids_encontrados[i] == ids_encontrados[i-1]) {
            cout << "¡DUPLICADO ENCONTRADO! ID: " << ids_encontrados[i] << endl;
        }
    }
}

void GeoCluster::verificarDuplicadosRec(Nodo* nodo, vector<int>& ids) {
    if (nodo == nullptr) return;
    
    if (nodo->esHoja) {
        for (const auto& punto : nodo->puntos) {
            ids.push_back(punto.id);
        }
    } else {
        for (Nodo* hijo : nodo->hijos) {
            verificarDuplicadosRec(hijo, ids);
        }
    }
}
*/