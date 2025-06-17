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

using namespace std;

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

double GeoCluster::calcularOverlapCosto(const MBR& mbr, const Punto& punto){
    double latMin = min(mbr.m_minp[0], punto.latitud);
    double longMin = min(mbr.m_minp[1], punto.longitud);
    double latMax = max(mbr.m_maxp[0], punto.latitud);
    double longMax = max(mbr.m_maxp[1], punto.longitud);

    double nuevaArea = (latMax-latMin) * (longMax-longMin);
    double areaOriginal = (mbr.m_maxp[0]- mbr.m_minp[0]) * (mbr.m_maxp[1]- mbr.m_minp[1]);
    return nuevaArea - areaOriginal;
}

double GeoCluster::calcularAreaCosto(const MBR& mbr, const Punto& punto){
    double latMin = min(mbr.m_minp[0], punto.latitud);
    double longMin = min(mbr.m_minp[1], punto.longitud);
    double latMax = max(mbr.m_maxp[0], punto.latitud);
    double longMax = max(mbr.m_maxp[1], punto.longitud);

    return (latMax - latMin) * (longMax - longMin);
}

double GeoCluster::calcularMBRArea(const MBR& mbr) {
    return (mbr.m_maxp[0] - mbr.m_minp[0]) * (mbr.m_maxp[1] - mbr.m_minp[1]);
}

Nodo* GeoCluster::chooseSubTree(Nodo* nodo, const Punto& punto_ingresado){
    if (nodo->esHoja) {
        return nodo;
    }

    if(all_of(nodo->hijos.begin(), nodo->hijos.end(), [](Nodo* hijo){return hijo->esHoja;})){
        Nodo* mejor_nodo = nullptr;
        double minOverlap = numeric_limits<double>::infinity();
        
        for (auto& hijo: nodo->hijos){
            double overLapCosto = calcularOverlapCosto(hijo->mbr, punto_ingresado);
            if (overLapCosto < minOverlap){
                minOverlap = overLapCosto;
                mejor_nodo = hijo;
            }
            else if(overLapCosto == minOverlap){
                double areaCosto = calcularAreaCosto(hijo->mbr, punto_ingresado);
                if(areaCosto < calcularAreaCosto(mejor_nodo->mbr, punto_ingresado)){
                    mejor_nodo = hijo;
                }
            }
        }
        return chooseSubTree(mejor_nodo, punto_ingresado);
    }
    else{
        Nodo* mejor_nodo = nullptr;
        double minArea = numeric_limits<double>::infinity();
        
        for (auto& hijo : nodo->hijos){
            double areaCosto = calcularAreaCosto(hijo->mbr, punto_ingresado);
            if(areaCosto < minArea){
                minArea = areaCosto;
                mejor_nodo = hijo;
            }
            else if(areaCosto == minArea){
                if(calcularMBRArea(hijo->mbr) < calcularMBRArea(mejor_nodo->mbr)){
                    mejor_nodo = hijo;
                }
            }
        }
        return chooseSubTree(mejor_nodo, punto_ingresado);
    }
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

void GeoCluster::Split(Nodo* nodo, Nodo*& nuevo_nodo) {
    if (!nodo->esHoja) {
        cout << "Error: Split solo debe llamarse en nodos hoja" << endl;
        return;
    }

    int eje = chooseSplitAxis(nodo->puntos);
    int splitIndex = chooseSplitIndex(nodo->puntos, eje);
    
    vector<Punto> grupo1(nodo->puntos.begin(), nodo->puntos.begin() + splitIndex);
    vector<Punto> grupo2(nodo->puntos.begin() + splitIndex, nodo->puntos.end());
    
    MBR mbrGrupo1 = calcularMBR(grupo1);
    MBR mbrGrupo2 = calcularMBR(grupo2);
    
    nuevo_nodo = new Nodo(true);
    nuevo_nodo->mbr = mbrGrupo2;
    nuevo_nodo->puntos = grupo2;
    nuevo_nodo->m_nivel = nodo->m_nivel;
    
    nodo->mbr = mbrGrupo1;
    nodo->puntos = grupo1;
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

void GeoCluster::insertarPunto(const Punto& punto) {
    if (raiz == nullptr) {
        raiz = new Nodo(true);
        raiz->m_nivel = 0;
    }

    Nodo* nodo_hoja = chooseSubTree(raiz, punto);

    if (nodo_hoja->esHoja) {
        insertIntoLeafNode(nodo_hoja, punto);

        if (nodo_hoja->puntos.size() > MAX_PUNTOS_POR_NODO) {
            Nodo* nuevo_nodo = nullptr;
            Split(nodo_hoja, nuevo_nodo);
            
            // Encontrar padre (implementación simplificada)
            if (nodo_hoja == raiz) {
                insertIntoParent(nullptr, nuevo_nodo);
            } else {
                // Aquí necesitarías encontrar el padre real
                cout << "Warning: Búsqueda de padre no implementada completamente" << endl;
            }
        }

        updateMBR(nodo_hoja);
    }
    else {
        cout << "Error: Se intentó insertar un punto en un nodo interno." << endl;
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