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
    // cout << "  Debug: Insertando punto " << punto.id << " en nodo nivel " << N->m_nivel 
    //      << " (esHoja: " << N->esHoja << ", puntos actuales: " << N->puntos.size() << ")" << endl;

    //I2: Si N tiene menos entradas que M, insertar el punto en N
    if (N->puntos.size() < MAX_PUNTOS_POR_NODO){
        insertIntoLeafNode(N,punto);
        updateMBR(N);
        // cout << "  Debug: Punto " << punto.id << " insertado directamente. Total en nodo: " << N->puntos.size() << endl;
    } 
    //S1: Si N tiene M entradas, llamar a OverFlowTreatment
    else{
        // cout << "  Debug: Nodo lleno, llamando OverFlowTreatment para punto " << punto.id << endl;
        bool splite_ocurrido = OverFlowTreatment(N,punto,nivel);
        if(splite_ocurrido){
            updateMBR(N);
            // cout << "  Debug: Split ocurrido para punto " << punto.id << endl;
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
            // cout << "  Debug: Punto " << punto.id << " insertado en nodo original después del split" << endl;
        } else {
            // Si el nodo original está lleno, insertar en el nuevo nodo
            insertIntoLeafNode(nuevo_nodo, punto);
            updateMBR(nuevo_nodo);
            // cout << "  Debug: Punto " << punto.id << " insertado en nuevo nodo después del split" << endl;
        }

        propagateSplit(N, nuevo_nodo);
        return true;  // Hubo split
    }
}

void GeoCluster::reinsert(Nodo* N, const Punto& punto) {
    // cout << "  Debug: Iniciando reinsert para nodo con " << N->puntos.size() << " puntos" << endl;
    
    vector<pair<double, Punto>> distancias;
    
    /*
    RI1: Calcular distancias al centro del nodo N, con los mbr
         de cada MBR que tiene el nodo N
    */ 
    Punto centro = calcularCentroNodo(N);
    for (const auto& p : N->puntos) {
        double dist = calcularDistancia(p, centro);
        distancias.push_back({dist, p});
        // cout << "  Debug: Punto " << p.id << " a distancia " << dist << " del centro" << endl;
    }
    
    // RI2: Ordenar por distancia
    sort(distancias.begin(), distancias.end(),greater<pair<double, Punto>>());
    
    // RI3: Remover 30% más lejanos
    int p = static_cast<int>(N->puntos.size() * PORCENTAJE_PARA_HACER_REINSERT);
    vector<Punto> puntos_a_reinsertar;
    
    // cout << "  Debug: Removiendo " << p << " puntos más lejanos para reinsertar" << endl;
    
    for (int i = 0; i < p; i++) {
        puntos_a_reinsertar.push_back(distancias[i].second);
        // cout << "  Debug: Punto " << distancias[i].second.id << " marcado para reinsertar" << endl;
    }
    
    // Actualizar puntos en N (quedarse con los más cercanos)
    vector<Punto> puntos_restantes;
    for (int i = p; i < distancias.size(); i++) {
        puntos_restantes.push_back(distancias[i].second);
        // cout << "  Debug: Punto " << distancias[i].second.id << " se queda en el nodo" << endl;
    }
    N->puntos = puntos_restantes;
    
    // Actualizar MBR
    updateMBR(N);
    
    // cout << "  Debug: Reinsertando " << puntos_a_reinsertar.size() << " puntos" << endl;
    
    // RI4: Reinsertar puntos (empezando por los más lejanos - reinserción lejana)
    for (const auto& p : puntos_a_reinsertar) {
        // cout << "  Debug: Reinsertando punto " << p.id << endl;
        insertar(p, N->m_nivel);
    }
    
    // IMPORTANTE: Después del reinsert, necesitamos insertar el punto original que causó el overflow
    // cout << "  Debug: Insertando punto original " << punto.id << " después del reinsert" << endl;
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
    // cout << "  Debug: === PROPAGANDO SPLIT ===" << endl;
    // cout << "    Nodo original: " << nodo_original->puntos.size() << " puntos" << endl;
    // cout << "    Nuevo nodo: " << nuevo_nodo->puntos.size() << " puntos" << endl;
    
    if (nodo_original == raiz) {
        // Crear nueva raíz
        Nodo* nueva_raiz = new Nodo(false);
        nueva_raiz->hijos.push_back(nodo_original);
        nueva_raiz->hijos.push_back(nuevo_nodo);
        nueva_raiz->m_nivel = nodo_original->m_nivel + 1;
        
        // Actualizar MBR de la nueva raíz
        nueva_raiz->mbr = nodo_original->mbr;
        nueva_raiz->mbr.stretch(nuevo_nodo->mbr);
        
        raiz = nueva_raiz;
        // cout << "    Nueva raíz creada en nivel " << nueva_raiz->m_nivel << endl;
    } else {
        // Encontrar padre y actualizar
        Nodo* padre = encontrarPadre(raiz, nodo_original);
        if (padre != nullptr) {
            int indice_hijo = padre->hijos.size();
            padre->hijos.push_back(nuevo_nodo);
            updateMBR(padre);
            
            // cout << "    Agregando hijo " << indice_hijo << " con " 
            //      << nuevo_nodo->puntos.size() << " puntos al padre (nivel " << padre->m_nivel << ")" << endl;
            // cout << "    Padre ahora tiene " << padre->hijos.size() << " hijos" << endl;
            
            // Si el padre se desborda, aplicar overflow treatment
            if (padre->hijos.size() > MAX_PUNTOS_POR_NODO) {
                // cout << "    ¡Padre se desborda! Aplicando overflow treatment..." << endl;
                // Crear un punto dummy para la llamada
                Punto punto_dummy;
                punto_dummy.id = -1;
                punto_dummy.latitud = 0.0;
                punto_dummy.longitud = 0.0;
                OverFlowTreatment(padre, punto_dummy, padre->m_nivel);
            }
        } else {
            // cout << "    ERROR: No se encontró padre para el nodo" << endl;
        }
    }
    // cout << "  Debug: === FIN PROPAGACIÓN SPLIT ===" << endl;
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
        // cout << "ERROR: Se perdieron puntos en el split!" << endl;
        // cout << "Original: " << nodo->puntos.size() << ", Grupo1: " << grupo1.size() 
        //      << ", Grupo2: " << grupo2.size() << endl;
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
        // cout << "Warning: Nodo interno excede capacidad - split no implementado" << endl;
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

bool GeoCluster::estaDentroDelMBR(const Punto& punto, const MBR& mbr) {
    bool dentroLatitud = (punto.latitud >= mbr.m_minp[0] && punto.latitud <= mbr.m_maxp[0]);
    bool dentroLongitud = (punto.longitud >= mbr.m_minp[1] && punto.longitud <= mbr.m_maxp[1]);

    return dentroLatitud && dentroLongitud;
}

bool GeoCluster::interseccionMBR(const MBR& mbr1, const MBR& mbr2) {
    return mbr1.is_intersected(mbr2);
}

vector<Punto> GeoCluster::buscarPuntosDentroInterseccion(const MBR& rango, Nodo* nodo) {
    vector<Punto> puntos_encontrados;
    if (nodo == nullptr) {
        cout << "[DEBUG] Nodo es nullptr" << endl;
        return puntos_encontrados;
    }

    cout << "[DEBUG] Verificando nodo nivel " << nodo->m_nivel << endl;
    cout << "[DEBUG] MBR del nodo: [" << nodo->mbr.m_minp[0] << "," << nodo->mbr.m_maxp[0] << "] x ["
         << nodo->mbr.m_minp[1] << "," << nodo->mbr.m_maxp[1] << "]" << endl;
    cout << "[DEBUG] MBR de búsqueda: [" << rango.m_minp[0] << "," << rango.m_maxp[0] << "] x ["
         << rango.m_minp[1] << "," << rango.m_maxp[1] << "]" << endl;

    // Si no hay intersección con el MBR del nodo, retornar vacío
    if (!interseccionMBR(nodo->mbr, rango)) {
        cout << "[DEBUG] No hay intersección con este nodo" << endl;
        return puntos_encontrados;
    }

    cout << "[DEBUG] Hay intersección con este nodo" << endl;

    // Si es hoja, verificar cada punto
    if (nodo->esHoja) {
        cout << "[DEBUG] Es nodo hoja con " << nodo->puntos.size() << " puntos" << endl;
        for (const auto& punto : nodo->puntos) {
            if (punto.latitud >= rango.m_minp[0] && punto.latitud <= rango.m_maxp[0] &&
                punto.longitud >= rango.m_minp[1] && punto.longitud <= rango.m_maxp[1]) {
                puntos_encontrados.push_back(punto);
            }
        }
        cout << "[DEBUG] Encontrados " << puntos_encontrados.size() << " puntos en este nodo hoja" << endl;
    }
    // Si no es hoja, buscar recursivamente en los hijos
    else {
        cout << "[DEBUG] No es hoja, tiene " << nodo->hijos.size() << " hijos" << endl;
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
        // cout << "Árbol vacío" << endl;
        return;
    }
    
    string indent(nivel * 2, ' ');
    // cout << indent << "Nivel " << nivel << " - ";
    
    if (nodo->esHoja) {
        // cout << "HOJA - " << nodo->puntos.size() << " puntos" << endl;
        // cout << indent << "  MBR: (" << nodo->mbr.m_minp[0] << "," << nodo->mbr.m_minp[1] << ") a ("
        //      << nodo->mbr.m_maxp[0] << "," << nodo->mbr.m_maxp[1] << ")" << endl;
        for (const auto& punto : nodo->puntos) {
            // cout << indent << "    Punto " << punto.id << ": (" << punto.latitud << "," << punto.longitud << ")" << endl;
        }
    } else {
        // cout << "INTERNO - " << nodo->hijos.size() << " hijos" << endl;
        // cout << indent << "  MBR: (" << nodo->mbr.m_minp[0] << "," << nodo->mbr.m_minp[1] << ") a ("
        //      << nodo->mbr.m_maxp[0] << "," << nodo->mbr.m_maxp[1] << ")" << endl;
        for (size_t i = 0; i < nodo->hijos.size(); i++) {
            // cout << indent << "  Hijo " << i << ":" << endl;
            imprimirArbol(nodo->hijos[i], nivel + 1);
        }
    }
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
        for (const auto& punto : nodo->puntos) {
            grupos_por_subcluster[punto.id_subcluster_atributivo].push_back(punto);
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
    cout << "\n📊 ESTADÍSTICAS DE MICROCLUSTERS" << endl;
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
    
    for (const auto& punto : nodo_hoja->puntos) {
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
            // cout << "¡DUPLICADO ENCONTRADO! ID: " << ids_encontrados[i] << endl;
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


// ============================================================================
// IMPLEMENTACIÓN DE MATRIZ SIMILITUD POR NODO
// ============================================================================

void MatrizSimilitudNodo::calcularSimilitudNodo(const vector<Punto>& puntos_nodo) {
    // cout << "  Calculando matriz de similitud para " << puntos_nodo.size() << " puntos..." << endl;
    
    ranking_similitud.clear();
    
    // Para cada punto, calcular similitud con todos los demás
    for (size_t i = 0; i < puntos_nodo.size(); i++) {
        vector<pair<double, int>> similitudes; // (similitud, id_punto)
        
        for (size_t j = 0; j < puntos_nodo.size(); j++) {
            if (i != j) { // No comparar consigo mismo
                double similitud = calcularSimilitudPuntos(puntos_nodo[i], puntos_nodo[j]);
                similitudes.push_back({similitud, puntos_nodo[j].id});
            }
        }
        
        // Ordenar por similitud (mayor primero)
        sort(similitudes.begin(), similitudes.end(), 
             [](const pair<double, int>& a, const pair<double, int>& b) {
                 return a.first > b.first;
             });
        
        // Guardar solo los IDs ordenados
        vector<int> ranking_ids;
        for (const auto& [similitud, id] : similitudes) {
            ranking_ids.push_back(id);
        }
        
        ranking_similitud[puntos_nodo[i].id] = ranking_ids;
    }
    
    // cout << "  ✅ Matriz de similitud calculada para " << ranking_similitud.size() << " puntos" << endl;
}

vector<Punto> MatrizSimilitudNodo::obtenerSimilaresNodo(int punto_id, int n_similares, const vector<Punto>& puntos_nodo) {
    vector<Punto> resultado;
    
    // Buscar el ranking del punto
    auto it = ranking_similitud.find(punto_id);
    if (it == ranking_similitud.end()) {
        return resultado; // Punto no encontrado
    }
    
    const vector<int>& ranking = it->second;
    int n_obtener = min(n_similares, (int)ranking.size());
    
    // Crear mapa de IDs para búsqueda rápida
    unordered_map<int, Punto> mapa_puntos;
    for (const auto& punto : puntos_nodo) {
        mapa_puntos[punto.id] = punto;
    }
    
    // Obtener los N puntos más similares
    for (int i = 0; i < n_obtener; i++) {
        int id_similar = ranking[i];
        auto punto_it = mapa_puntos.find(id_similar);
        if (punto_it != mapa_puntos.end()) {
            resultado.push_back(punto_it->second);
        }
    }
    
    return resultado;
}

vector<Punto> MatrizSimilitudNodo::obtenerSimilaresEnRangoNodo(int punto_id, const MBR& rango, int n_similares, const vector<Punto>& puntos_nodo) {
    vector<Punto> resultado;
    
    // Buscar el ranking del punto
    auto it = ranking_similitud.find(punto_id);
    if (it == ranking_similitud.end()) {
        return resultado; // Punto no encontrado
    }
    
    const vector<int>& ranking = it->second;
    
    // Crear mapa de IDs para búsqueda rápida
    unordered_map<int, Punto> mapa_puntos;
    for (const auto& punto : puntos_nodo) {
        mapa_puntos[punto.id] = punto;
    }
    
    // Verificar cada punto en el ranking si está en el rango
    for (int id_similar : ranking) {
        auto punto_it = mapa_puntos.find(id_similar);
        if (punto_it != mapa_puntos.end()) {
            const Punto& punto = punto_it->second;
            
            // Verificar si está en el rango
            if (punto.latitud >= rango.m_minp[0] && punto.latitud <= rango.m_maxp[0] &&
                punto.longitud >= rango.m_minp[1] && punto.longitud <= rango.m_maxp[1]) {
                resultado.push_back(punto);
                
                if (resultado.size() >= n_similares) {
                    break;
                }
            }
        }
    }
    
    return resultado;
}

// ============================================================================
// FUNCIONES AUXILIARES PARA MATRICES DE SIMILITUD
// ============================================================================

void GeoCluster::calcularMatricesSimilitudEnHojas() {
    cout << "\n🔬 Calculando matrices de similitud en todas las hojas..." << endl;
    calcularMatricesSimilitudRec(raiz);
    cout << "Matrices de similitud calculadas en todas las hojas" << endl;
}

void GeoCluster::calcularMatricesSimilitudRec(Nodo* nodo) {
    if (nodo == nullptr) return;
    
    if (nodo->esHoja) {
        // Calcular matriz de similitud para este nodo hoja
        if (!nodo->puntos.empty()) {
            // cout << "  Nodo hoja nivel " << nodo->m_nivel << ": " << nodo->puntos.size() << " puntos" << endl;
            nodo->matriz_similitud.calcularSimilitudNodo(nodo->puntos);
        }
    } else {
        // Recursión para nodos internos
        for (Nodo* hijo : nodo->hijos) {
            calcularMatricesSimilitudRec(hijo);
        }
    }
}

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

