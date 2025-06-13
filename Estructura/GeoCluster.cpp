#include "GeoCluster.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <array>
#include <unordered_map>
#include<iostream>
using namespace std;

int computeArea(int ax1, int ay1, int ax2, int ay2,int bx1, int by1, int bx2, int by2) {
    int area1 = (ax2 - ax1) * (ay2 - ay1);
    int area2 = (bx2 - bx1) * (by2 - by1);
    
    int interseccion_A = max(0, min(ax2, bx2) - max(ax1, bx1));
    int interseccion_B = max(0, min(ay2, by2) - max(ay1, by1));
    
    int intersectadaArea = interseccion_A * interseccion_B;
    return area1 + area2 - intersectadaArea;
}

/*
Funcion que determina si un rango de busqueda intersecciona con un microcluster
*/
bool interseccion_mbr_microcluster(const MicroCluster& mbr1, const MBR& rect) {
    double lat = mbr1.centroId[0];
    double lon = mbr1.centroId[1];

    return !(lon < rect.m_minp[1] || lon > rect.m_maxp[1] ||
             lat < rect.m_minp[0]  || lat > rect.m_maxp[0]);
}



//Constructor
GeoCluster::GeoCluster() {
    // Inicialización de los atributos de la clase, si es necesario
    raiz = nullptr;  // Suponiendo que `raiz` es un puntero
    // Otros atributos si es necesario...
    std::cout << "Constructor llamado" << std::endl;
}

// Destructor
GeoCluster::~GeoCluster() {
    // Liberar recursos si es necesario, como liberar memoria dinámica
    delete raiz;
    // Liberar otros recursos si es necesario
    std::cout << "Destructor llamado" << std::endl;
}

//Funcion chooseSubTree y sus subfunciones
double GeoCluster::calcularOverlapCosto(const MBR& mbr, const Punto& punto){
    double latMin = min(mbr.m_minp[0], punto.latitud);
    double longMin = min(mbr.m_minp[1], punto.longitud);
    double latMax = max(mbr.m_maxp[0], punto.latitud);
    double longMax = max(mbr.m_maxp[1], punto.longitud);

    //calculamos el area del nuevo mbr
    double nuevaArea = (latMax-latMin) * (longMax-longMin);
    double areaOriginal = (mbr.m_maxp[0]- mbr.m_minp[0]) * (mbr.m_maxp[1]- mbr.m_minp[1]);
    return nuevaArea - areaOriginal;
}

double GeoCluster::calcularAreaCosto(const MBR& mbr, const Punto& punto){
    double latMin = min(mbr.m_minp[0], punto.latitud);
    double longMin = min(mbr.m_minp[1], punto.longitud);
    double latMax = max(mbr.m_maxp[0], punto.latitud);
    double longMax = max(mbr.m_maxp[1], punto.longitud);

    //calculamos el area del nuevo mbr
    return (latMax - latMin) * (longMax - longMin);
}

double GeoCluster::calcularMBRArea(const MBR& mbr) {
    return (mbr.m_maxp[0] - mbr.m_minp[0]) * (mbr.m_maxp[1] - mbr.m_minp[1]);
}

Nodo* GeoCluster::chooseSubTree(const Punto& punto_ingresado){
    Nodo* N = raiz;
    if (N->esNodoHoja()) {
        return N; // Si es hoja, retornar el nodo
    }
    
    if(all_of(N->hijo.begin(),N->hijo.end(),[](Nodo* hijo){return hijo->esNodoHoja();})){
        Nodo* mejor_nodo = nullptr;
        double minOverLap = numeric_limits<double>::infinity();

        for (auto& hijo: N->hijo){
            double overLapCosto = calcularOverlapCosto(hijo->mbr, punto_ingresado);
            if (overLapCosto < minOverLap){
                minOverLap = overLapCosto;
                mejor_nodo = hijo;
            }

            else if(overLapCosto == minOverLap){
                double areaCosto = calcularAreaCosto(hijo->mbr, punto_ingresado);
                if( areaCosto < calcularAreaCosto(mejor_nodo->mbr, punto_ingresado)){
                    mejor_nodo = hijo;
                }
            }
        }
        return mejor_nodo;
    }
    
    else{
        Nodo* mejor_nodo = nullptr;
        double  minArea = numeric_limits<double>::infinity();
        for (auto & hijo :N->hijo){
            double areaCosto = calcularAreaCosto(hijo->mbr,punto_ingresado);
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
        return chooseSubTree(punto_ingresado);
    }
}


//Funcion Split y sus subfunciones
double GeoCluster::calcularOverlap(const MBR& mbr1, const MBR& mbr2) {
    double lat_overlap = std::max(0.0, std::min(mbr1.m_maxp[0], mbr2.m_maxp[0]) - std::max(mbr1.m_minp[0], mbr2.m_minp[0]));
    double lon_overlap = std::max(0.0, std::min(mbr1.m_maxp[1], mbr2.m_maxp[1]) - std::max(mbr1.m_minp[1], mbr2.m_minp[1]));

    return lat_overlap * lon_overlap;  // El área de la intersección
}

double GeoCluster::calcularMargen(const MBR& mbr) {
    // Calcular la longitud de los bordes en cada dimensión
    double latitudLongitud = mbr.m_maxp[0] - mbr.m_minp[0];  // Longitud en el eje latitud
    double longitudLongitud = mbr.m_maxp[1] - mbr.m_minp[1];  // Longitud en el eje longitud

    // El margen es 2 veces la suma de las longitudes
    return 2 * (latitudLongitud + longitudLongitud);
}

MBR GeoCluster::calcularMBR(const vector<Punto>& puntos) {
    if (puntos.empty()) {
        // Retornar un MBR inválido o vacío si no hay puntos
        return MBR(0, 0, 0, 0);
    }
    double minLat = puntos[0].latitud, maxLat = puntos[0].latitud;
    double minLon = puntos[0].longitud, maxLon = puntos[0].longitud;

    // Calcular los valores mínimo y máximo de latitud y longitud
    for (const auto& punto : puntos) {
        minLat = std::min(minLat, punto.latitud);
        minLon = std::min(minLon, punto.longitud);
        maxLat = std::max(maxLat, punto.latitud);
        maxLon = std::max(maxLon, punto.longitud);
    }

    // Crear el MBR que cubre todos los puntos
    return MBR(minLat, minLon, maxLat, maxLon);
}

int GeoCluster::chooseSplitAxis(const vector<Punto>& puntos){
    // Paso 1: Ordenar por latitud (eje 0) y longitud (eje 1)
    vector<Punto> puntosLatitud = puntos;
    vector<Punto> puntosLongitud = puntos;

    // Ordenar por latitud
    sort(puntosLatitud.begin(), puntosLatitud.end(), [](const Punto& p1, const Punto& p2) {
        return p1.latitud < p2.latitud;
    });

    // Ordenar por longitud
    sort(puntosLongitud.begin(), puntosLongitud.end(), [](const Punto& p1, const Punto& p2) {
        return p1.longitud < p2.longitud;
    });

    // Paso 2: Calcular el MBR para cada conjunto de puntos ordenados
    MBR mbrLatitud = calcularMBR(puntosLatitud);  // MBR para los puntos ordenados por latitud
    MBR mbrLongitud = calcularMBR(puntosLongitud);  // MBR para los puntos ordenados por longitud

    // Paso 3: Calcular el margen para cada eje (latitud y longitud)
    double margenLat = calcularMargen(mbrLatitud);  // Calcular margen para el MBR de latitud
    double margenLong = calcularMargen(mbrLongitud);  // Calcular margen para el MBR de longitud

    // Paso 4: Seleccionar el eje con menor margen
    return (margenLat < margenLong) ? 0 : 1; // 0: latitud, 1: longitud
}

int GeoCluster::chooseSplitIndex(vector<Punto>& puntos, int eje) {
    int M = MAX_PUNTOS_POR_NODO;  // Número total de puntos
    int m = MIN_PUNTOS_POR_NODO;  // Número mínimo de puntos por grupo

    // Ordenar los puntos según el eje seleccionado (latitud o longitud)
    if (eje == 0) { // Ordenar por latitud
        sort(puntos.begin(), puntos.end(), [](const Punto& p1, const Punto& p2) {
            return p1.latitud < p2.latitud;  // Ordenar por latitud
        });
    } else { // Ordenar por longitud
        sort(puntos.begin(), puntos.end(), [](const Punto& p1, const Punto& p2) {
            return p1.longitud < p2.longitud;  // Ordenar por longitud
        });
    }

    // Paso 1: Calcular las distribuciones posibles (M - 2m + 2)
    int numero_de_distribuciones = M - 2 * m + 2;  // Las distribuciones válidas
    double minOverlap = numeric_limits<double>::infinity();
    double minMargen = numeric_limits<double>::infinity();
    double minArea = numeric_limits<double>::infinity();
    int bestSplitIndex = 0;

    // Paso 2: Evaluar todas las distribuciones posibles
    for (int i = 0; i < numero_de_distribuciones; ++i) {  // Para cada posible división (entre m y M-m)
        vector<Punto> grupo1(puntos.begin(), puntos.begin() + i);  // Grupo 1 con i puntos
        vector<Punto> grupo2(puntos.begin() + i, puntos.end());   // Grupo 2 con el resto de los puntos

        // Calcular los MBRs para ambos grupos
        MBR mbrGrupo1 = calcularMBR(grupo1);
        MBR mbrGrupo2 = calcularMBR(grupo2);

        // Calcular el solapamiento entre los MBRs
        double overlap = calcularOverlap(mbrGrupo1, mbrGrupo2);

        // Calcular el margen entre los MBRs
        double margen = calcularMargen(mbrGrupo1) + calcularMargen(mbrGrupo2);

        // Calcular el área total
        double area = calcularMBRArea(mbrGrupo1) + calcularMBRArea(mbrGrupo2);

        // Paso 3: Evaluar solapamiento
        if (overlap < minOverlap) {
            minOverlap = overlap;
            bestSplitIndex = i;
        } else if (overlap == minOverlap) {
            // Si hay empate en overlap, elegir el margen más pequeño
            if (margen < minMargen) {
                minMargen = margen;
                bestSplitIndex = i;
            } else if (margen == minMargen) {
                // Si hay empate en margen, elegir el área más pequeña
                if (area < minArea) {
                    minArea = area;
                    bestSplitIndex = i;
                }
            }
        }
    }

    return bestSplitIndex;  // Devolver el índice con el menor solapamiento
}

void GeoCluster::Split(Nodo* nodo, Nodo*& nuevo_nodo) {
    // Paso 1: Elegir el eje y el índice para la división
    int eje = chooseSplitAxis(nodo->puntos);
    int splitIndex = chooseSplitIndex(nodo->puntos, eje);
    
    // Paso 2: Dividir los puntos en dos grupos según el índice
    vector<Punto> grupo1(nodo->puntos.begin(), nodo->puntos.begin() + splitIndex);
    vector<Punto> grupo2(nodo->puntos.begin() + splitIndex, nodo->puntos.end());
    
    // Paso 3: Crear los MBRs para los dos grupos
    MBR mbrGrupo1 = calcularMBR(grupo1);
    MBR mbrGrupo2 = calcularMBR(grupo2);
    
    // Paso 4: Crear un nuevo nodo para el segundo grupo
    nuevo_nodo = new Nodo();
    nuevo_nodo->mbr = mbrGrupo2;
    nuevo_nodo->puntos = grupo2;
    
    // Actualizar el MBR del nodo original
    nodo->mbr = mbrGrupo1;
    nodo->puntos = grupo1;
}

void GeoCluster::insertIntoLeafNode(Nodo* nodo_hoja, const Punto& punto) {
    // Insertamos el punto en el nodo hoja (aquí se puede hacer en orden)
    nodo_hoja->puntos.push_back(punto);
    
    // Actualizar el MBR del nodo hoja
    nodo_hoja->mbr = calcularMBR(nodo_hoja->puntos);
}

void GeoCluster::insertIntoParent(Nodo* nodo_hoja, Nodo* nuevo_nodo) {
    // Si el nodo padre está lleno, realizar un split
    if (nodo_hoja->puntos.size() > MAX_PUNTOS_POR_NODO) {
        Nodo* nuevo_nodo_padre = nullptr;
        Split(nodo_hoja, nuevo_nodo_padre);
        
        // Insertar el nuevo nodo en el nodo padre
        insertIntoParent(nodo_hoja, nuevo_nodo_padre);
    }
    
    // Si no hay split, simplemente añadir el nuevo nodo al nodo padre
    nodo_hoja->hijo.push_back(nuevo_nodo);
}

// Actualizar el MBR de un nodo
void GeoCluster::updateMBR(Nodo* nodo) {
    // Recalcular el MBR de un nodo para incluir todos sus puntos
    if (!nodo->esNodoHoja()) {
        for (Nodo* hijo : nodo->hijo) {
            // Recalcular el MBR del nodo padre en función de los MBR de sus hijos
            nodo->mbr = calcularMBR(hijo->puntos);
        }
    }
}

void GeoCluster::insertarPunto(const Punto& punto) {
    if (raiz == nullptr) {
        raiz = new Nodo();  // Si la raíz es nula, inicializarla
        raiz->m_level = 0;  // Nivel de hoja
        // Aquí puedes inicializar cualquier otro atributo necesario
    }

    Nodo* nodo_hoja = chooseSubTree(punto);  // Encontrar el subárbol adecuado
    insertIntoLeafNode(nodo_hoja, punto);    // Insertar el punto en el nodo hoja

    // Si el nodo hoja está lleno, realizar un split
    if (nodo_hoja->puntos.size() > MAX_PUNTOS_POR_NODO) {
        Nodo* nuevo_nodo = nullptr;
        Split(nodo_hoja, nuevo_nodo);
        insertIntoParent(nodo_hoja, nuevo_nodo);  // Insertar el nuevo nodo en el nodo padre
    }

    updateMBR(nodo_hoja);  // Actualizar el MBR hacia arriba si es necesario
}

MBR GeoCluster::computeMBR(double ax1, double ay1, double ax2, double ay2,
                           double bx1, double by1, double bx2, double by2){
    // Calcular las coordenadas del MBR de intersección
    double interseccion_minLat = max(ax1, bx1);  // Latitud mínima de la intersección
    double interseccion_minLon = max(ay1, by1);  // Longitud mínima de la intersección
    double interseccion_maxLat = min(ax2, bx2);  // Latitud máxima de la intersección
    double interseccion_maxLon = min(ay2, by2);  // Longitud máxima de la intersección

    // Verificar si hay intersección. Si no hay intersección, devolver un MBR vacío.
    if (interseccion_minLat >= interseccion_maxLat || interseccion_minLon >= interseccion_maxLon) {
        return MBR();  // Retornamos un MBR vacío si no hay intersección
    }

    // Crear un nuevo MBR con las coordenadas de la intersección
    return MBR(interseccion_minLat, interseccion_minLon, interseccion_maxLat, interseccion_maxLon);
}

bool GeoCluster::estaDentroDelMBR(const Punto& punto, const MBR& mbr) {
    // Verificar si la latitud del punto está dentro de los límites del MBR
    bool dentroLatitud = (punto.latitud >= mbr.m_minp[0] && punto.latitud <= mbr.m_maxp[0]);
    
    // Verificar si la longitud del punto está dentro de los límites del MBR
    bool dentroLongitud = (punto.longitud >= mbr.m_minp[1] && punto.longitud <= mbr.m_maxp[1]);

    // El punto está dentro del MBR si cumple ambas condiciones
    return dentroLatitud && dentroLongitud;
}


bool GeoCluster::interseccionMBR(const MBR& mbr1, const MBR& mbr2) {
    // Calculamos el área de la intersección entre los dos MBRs
    int areaTotal = computeArea(mbr1.m_minp[0], mbr1.m_minp[1], mbr1.m_maxp[0], mbr1.m_maxp[1], 
                                mbr2.m_minp[0], mbr2.m_minp[1], mbr2.m_maxp[0], mbr2.m_maxp[1]);
    
    // Si el área total es mayor que 0, entonces hay intersección
    return areaTotal > 0;
}

vector<Punto> GeoCluster::buscarPuntosDentroInterseccion(const MBR& rango, Nodo* nodo) {
    vector<Punto> puntosEncontrados;

    // Verificar si el nodo actual se solapa con el MBR de la consulta
    MBR mbrInterseccion = computeMBR(nodo->mbr.m_minp[0], nodo->mbr.m_minp[1], 
                                     nodo->mbr.m_maxp[0], nodo->mbr.m_maxp[1], 
                                     rango.m_minp[0], rango.m_minp[1], 
                                     rango.m_maxp[0], rango.m_maxp[1]);
    
    // Si el MBR de intersección es válido, buscar puntos dentro de él
    if (mbrInterseccion.m_minp[0] != 0 && mbrInterseccion.m_minp[1] != 0) {
        // Si es una hoja, agregar los puntos del nodo si caen dentro del MBR de intersección
        if (nodo->esNodoHoja()) {
            for (const auto& punto : nodo->puntos) {
                if (estaDentroDelMBR(punto, mbrInterseccion)) {
                    puntosEncontrados.push_back(punto);  // Agregar punto si está dentro
                }
            }
        } else {  // Si no es hoja, buscar recursivamente en los hijos
            for (Nodo* hijo : nodo->hijo) {
                std::vector<Punto> puntosHijo = buscarPuntosDentroInterseccion(rango, hijo);
                puntosEncontrados.insert(puntosEncontrados.end(), puntosHijo.begin(), puntosHijo.end());
            }
        }
    }

    return puntosEncontrados;
}

void GeoCluster::searchRec(const MBR& rango, Nodo* nodo, vector<Punto>& puntos_similares) {
    // Verificar si el nodo actual se solapa con el MBR de la consulta
    if (interseccionMBR(nodo->mbr, rango)) {
        // Si es una hoja, agregar los puntos al resultado
        if (nodo->esNodoHoja()) {
            for (const auto& punto : nodo->puntos) {
                puntos_similares.push_back(punto);
            }
        } else {  // Si no es hoja, buscar recursivamente en los hijos
            for (Nodo* hijo : nodo->hijo) {
                searchRec(rango, hijo, puntos_similares);
            }
        }
    }
}
