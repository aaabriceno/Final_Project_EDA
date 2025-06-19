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

//probando
using namespace std;

//Primera funcion que se usa para insertar un punto
void GeoCluster::inserData(const Punto& punto_a_insertar){
    niveles_reinsert.clear();
    insertar(punto_a_insertar,0);
}

void GeoCluster::insertar(const Punto& punto, int nivel) {
    //I1: Llamar a chooseSubTree para encontrar el mejor nodo hoja
    Nodo* N = raiz;
    if (raiz == nullptr) {
        raiz = new Nodo(true); //es hoja
        raiz->m_nivel = 0;
    }

    Nodo* nodo_hoja = chooseSubTree(raiz, punto, nivel);

    //I2: Si N tiene menos entradas que M, insertar el punto en N
    if (nodo_hoja->puntos.size() < MAX_PUNTOS_POR_NODO){
        insertIntoLeafNode(N,punto);
        updateMBR(N);
    } 
    //S1: Si N tiene M entradas, llamar a OverFlowTreatment
    else{
        Nodo* nuevo_nodo = nullptr;
        bool splite = OverFlowTreatment(N,punto,nivel);
        
    }
}

Nodo* GeoCluster::chooseSubTree(Nodo* N,const Punto punto, int nivel){
    Nodo* N = raiz; //iniciamos con la raiz
    Nodo* mejor_nodo = nullptr;
    //si N es hoja
    if (N->esHoja) {
        return N;
    }
    //si nodo no es hoja (nodo interno)
    else {
        // si los punteros hijos apuntan a hojas
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
            return chooseSubTree(N,punto, nivel);
        }
        //si los punteros hijos a puntan a nodos internos
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
    }  
    //CS3 : Seleccionar N como el nodo hijo y repetir desde CS2
    return chooseSubTree(N,punto, nivel);
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
        return true;  // Hubo split
    }
}

void GeoCluster::reinsert(Nodo* N, const Punto& punto) {
    vector<pair<double, Punto>> distancias;
    
    /*
    RI1: Calcular distancias al centro del nodo N, con los mbr
         de cada MBR que tiene el nodo N
    */ 
    Punto centro = calcularCentroNodo(N);
    for (const auto& p : N->puntos) {
        double dist = calcularDistancia(p, centro);
        distancias.push_back({dist, p});
    }
    
    // RI2: Ordenar por distancia
    sort(distancias.begin(), distancias.end());
    
    // RI3: Remover 30% más lejanos
    int p = static_cast<int>(N->puntos.size() * PORCENTAJE_PARA_HACER_REINSERT);
    vector<Punto> puntos_a_reinsertar;
    
    for (int i = 0; i < p; i++) {
        puntos_a_reinsertar.push_back(distancias[i].second);
    }
    
    // Actualizar puntos en N
    vector<Punto> puntos_restantes;
    for (int i = p; i < distancias.size(); i++) {
        puntos_restantes.push_back(distancias[i].second);
    }
    N->puntos = puntos_restantes;
    
    // Actualizar MBR
    updateMBR(N);
    
    // RI4: Reinsertar puntos
    for (const auto& p : puntos_a_reinsertar) {
        insertar(p, N->m_nivel);
    }
}

void GeoCluster::Split(Nodo* nodo, Nodo*& nuevo_nodo) {
    // S1: Elegir eje de división
    int eje = chooseSplitAxis(nodo->puntos);
    
    // S2: Elegir índice de división
    int split_index = chooseSplitIndex(nodo->puntos, eje);
    
    // S3: Distribuir entradas
    nuevo_nodo = new Nodo(nodo->esHoja);
    nuevo_nodo->m_nivel = nodo->m_nivel;
    
    vector<Punto> grupo1(nodo->puntos.begin(), nodo->puntos.begin() + split_index);
    vector<Punto> grupo2(nodo->puntos.begin() + split_index, nodo->puntos.end());
    
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

GeoCluster::GeoCluster() {
    raiz = nullptr;
    std::cout << "Constructor llamado" << std::endl;
}

GeoCluster::~GeoCluster() {
    delete raiz;
    std::cout << "Destructor llamado" << std::endl;
}

//Funciones que ayudan a chooseSubTree
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


//Funciones que ayudan a Split
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

//Funcion Split 





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