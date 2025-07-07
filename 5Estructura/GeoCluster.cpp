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
    cout.precision(14);
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

void GeoCluster::inserData(const Punto& punto_a_insertar){
    // Optimizar atributos antes de insertar
    Punto punto_optimizado = punto_a_insertar;
    punto_optimizado.optimizarAtributos();
    
    // NUEVO: Almacenar el punto completo en la hash table
    almacenarPuntoCompleto(punto_optimizado);
    
    niveles_reinsert.clear();
    insertar(punto_optimizado,0);
}

void GeoCluster::insertar(const Punto& punto, int nivel) {
    // Verificar que el punto sea válido
    if (punto.id < 0) {
        cerr << "Error: ID de punto inválido: " << punto.id << endl;
        return;
    }
    
    //I1: Llamar a chooseSubTree para encontrar el mejor nodo hoja
    if(raiz == nullptr){
        raiz = new Nodo(true);
        raiz->m_nivel = 0;
        raiz->padre = nullptr;
    }
    
    Nodo* N = chooseSubTree(raiz,punto,nivel);
    
    // Verificar que el nodo sea válido
    if (N == nullptr) {
        cerr << "Error: No se pudo encontrar nodo para insertar punto " << punto.id << endl;
        return;
    }

    //I2: Si N tiene menos entradas que M, insertar el punto en N
    if (N->puntos.size() < MAX_PUNTOS_POR_NODO){
        insertIntoLeafNode(N,punto);
        updateMBR(N);
    } 
    //S1: Si N tiene M entradas, llamar a OverFlowTreatment
    else{
        bool splite_ocurrido = OverFlowTreatment(N,punto,nivel);
        if(splite_ocurrido){
            updateMBR(N);
        } 
    }
}

Nodo* GeoCluster::chooseSubTree(Nodo* N, const Punto punto, int nivel){
    // Verificar que el nodo sea válido
    if (N == nullptr) {
        cerr << "Error: Nodo nullptr en chooseSubTree" << endl;
        return nullptr;
    }
    
    // CS1: Ponemos a N como la raiz (IMPLICITAMENTE)
    // CS2: Si N es hoja, retorna N
    if (N->esHoja) {
        return N;
    }
    
    // Verificar que el nodo tenga hijos
    if (N->hijos.empty()) {
        cerr << "Error: Nodo interno sin hijos en chooseSubTree" << endl;
        return nullptr;
    }
    
    Nodo* mejor_nodo = nullptr;

    // Si los punteros hijos apuntan a hojas
    if(all_of(N->hijos.begin(), N->hijos.end(), [](Nodo* hijo){return hijo->esHoja;})){
        double minOverlap = numeric_limits<double>::infinity();
        for (auto& hijo: N->hijos){
            if (hijo == nullptr) continue; // Saltar nodos nulos
            
            double costoOverlap = calcularCostoOverlap(hijo->mbr, punto);
            if (costoOverlap < minOverlap){
                minOverlap = costoOverlap;
                mejor_nodo = hijo;
            }
            else if(costoOverlap == minOverlap){
                double CostoAreaAmpliacion = calcularCostoDeAmpliacionArea(hijo->mbr, punto);
                if(mejor_nodo != nullptr && CostoAreaAmpliacion < calcularCostoDeAmpliacionArea(mejor_nodo->mbr, punto)){
                    mejor_nodo = hijo;
                }
            }
        }
    }
    // Si los punteros hijos apuntan a nodos internos
    else{
        double minArea = numeric_limits<double>::infinity();
        for (auto& hijo : N->hijos){
            if (hijo == nullptr) continue; // Saltar nodos nulos
            
            double CostoMinArea = calcularCostoDeAmpliacionArea(hijo->mbr, punto);
            if(CostoMinArea < minArea){
                minArea = CostoMinArea;
                mejor_nodo = hijo;
            }
            else if(CostoMinArea == minArea){
                if(mejor_nodo != nullptr && calcularMBRArea(hijo->mbr) < calcularMBRArea(mejor_nodo->mbr)){
                    mejor_nodo = hijo;
                }
            }
        }  
    }
    
    // CS3: Seleccionar N como el nodo hijo y repetir desde CS2
    if (mejor_nodo != nullptr) {
        return chooseSubTree(mejor_nodo, punto, nivel);
    } 
    // Fallback: si no se encontró mejor nodo, retornar el primer hijo válido
    for (auto& hijo : N->hijos) {
        if (hijo != nullptr) {
            return chooseSubTree(hijo, punto, nivel);
        }
    }
    return nullptr;
}

bool GeoCluster::OverFlowTreatment(Nodo* N, const Punto& punto, int nivel) {
    // Verificar que el nodo sea válido
    if (N == nullptr) {
        cerr << "Error: Nodo nullptr en OverFlowTreatment" << endl;
        return false;
    }
    
    // OVT1: Si no es raíz y es primera vez en este nivel, hacer reinsert
    if (N != raiz && niveles_reinsert.find(nivel) == niveles_reinsert.end()) {
        niveles_reinsert[nivel] = true;
        reinsert(N, punto);
        return false;  // No hubo split
    } else {
        // Si ya se hizo reinsert en este nivel o es raíz, hacer split
        Nodo* nuevo_nodo = nullptr;
        Split(N, nuevo_nodo);

        // Verificar que el split fue exitoso
        if (nuevo_nodo == nullptr) {
            cerr << "Error: Split falló en OverFlowTreatment" << endl;
            return false;
        }

        // Después del split, necesitamos insertar el punto que causó el overflow
        // Primero, agregar el punto al nodo original si hay espacio
        if (N->puntos.size() < MAX_PUNTOS_POR_NODO) {
            insertIntoLeafNode(N, punto);
            updateMBR(N);
        } else {
            // Si el nodo original está lleno, insertar en el nuevo nodo
            insertIntoLeafNode(nuevo_nodo, punto);
            updateMBR(nuevo_nodo);
        }

        propagateSplit(N, nuevo_nodo);
        return true;  // Hubo split
    }
}

void GeoCluster::reinsert(Nodo* N, const Punto& punto) {
    // Verificar que el nodo sea válido
    if (N == nullptr) {
        cerr << "Error: Nodo nullptr en reinsert" << endl;
        return;
    }
    
    vector<pair<double, Punto>> distancias;
    
    /*
    RI1: Calcular distancias al centro del nodo N, con los mbr
         de cada MBR que tiene el nodo N
    */ 
    Punto centro = calcularCentroNodo(N);
    for (const auto& p : N->puntos) {
        double dist = calcularDistancia(obtenerPuntoCompleto(p.id), centro);
        distancias.push_back({dist, obtenerPuntoCompleto(p.id)});
    }
    
    // RI2: Ordenar por distancia
    sort(distancias.begin(), distancias.end(),greater<pair<double, Punto>>());
    
    // RI3: Remover 30% más lejanos
    int p = static_cast<int>(N->puntos.size() * PORCENTAJE_PARA_HACER_REINSERT);
    vector<Punto> puntos_a_reinsertar;
    
    for (int i = 0; i < p && i < distancias.size(); i++) {
        puntos_a_reinsertar.push_back(distancias[i].second);
    }
    
    // Actualizar puntos en N (quedarse con los más cercanos)
    vector<PointID> puntos_restantes;
    for (int i = p; i < distancias.size(); i++) {
        // NUEVO: Convertir Punto a PointID
        PointID point_id(distancias[i].second);
        puntos_restantes.push_back(point_id);
    }
    N->puntos = puntos_restantes;
    
    // Actualizar MBR
    updateMBR(N);
    
    // RI4: Reinsertar puntos (empezando por los más lejanos - reinserción lejana)
    for (const auto& p : puntos_a_reinsertar) {
        insertar(p, N->m_nivel);
    }
    
    // IMPORTANTE: Después del reinsert, necesitamos insertar el punto original que causó el overflow
    insertar(punto, N->m_nivel);
}

Punto GeoCluster::calcularCentroNodo(Nodo* nodo) {
    // Verificar que el nodo sea válido
    if (nodo == nullptr || nodo->puntos.empty()) {
        return Punto();
    }
    
    double sumLat = 0.0, sumLon = 0.0;
    int count = 0;
    
    // NUEVO: Trabajar con PointID
    for (const auto& point_id : nodo->puntos) {
        sumLat += point_id.latitud;
        sumLon += point_id.longitud;
        count++;
    }
    
    if (count == 0) {
        return Punto();
    }
    
    return Punto(0, sumLat / count, sumLon / count, vector<double>());
}

double GeoCluster::calcularDistancia(const Punto& p1, const Punto& p2) {
    return sqrt(pow(p1.latitud - p2.latitud, 2) + pow(p1.longitud - p2.longitud, 2));
}

void GeoCluster::propagateSplit(Nodo* nodo_original, Nodo* nuevo_nodo) {
    // Verificar que los nodos sean válidos
    if (nodo_original == nullptr || nuevo_nodo == nullptr) {
        cerr << "Error: Nodos nullptr en propagateSplit" << endl;
        return;
    }
    
    if (nodo_original == raiz) {
        // Crear nueva raíz
        Nodo* nueva_raiz = new Nodo(false);
        nueva_raiz->hijos.push_back(nodo_original);
        nueva_raiz->hijos.push_back(nuevo_nodo);
        nueva_raiz->m_nivel = nodo_original->m_nivel + 1;
        nueva_raiz->padre = nullptr;
        
        // Actualizar padres
        nodo_original->padre = nueva_raiz;
        nuevo_nodo->padre = nueva_raiz;
        
        // Calcular MBR de la nueva raíz
        nueva_raiz->mbr = nodo_original->mbr;
        nueva_raiz->mbr.stretch(nuevo_nodo->mbr);
        
        raiz = nueva_raiz;
    } else {
        // Insertar en el padre
        insertIntoParent(nodo_original->padre, nuevo_nodo);
        nuevo_nodo->padre = nodo_original->padre;
    }
}

void GeoCluster::Split(Nodo* nodo, Nodo*& nuevo_nodo) {
    // Verificar que el nodo sea válido
    if (nodo == nullptr) {
        cerr << "Error: Nodo nullptr en Split" << endl;
        nuevo_nodo = nullptr;
        return;
    }
    
    // Verificar que el nodo tenga suficientes puntos para hacer split
    if (nodo->puntos.size() < 2) {
        cerr << "Error: Nodo con muy pocos puntos para split" << endl;
        nuevo_nodo = nullptr;
        return;
    }
    
    vector<PointID> puntos = nodo->puntos; // NUEVO: Trabajar con PointID
    
    // Elegir eje de split (optimizado)
    int eje = chooseSplitAxis(puntos);
    
    // Elegir índice de split (optimizado)
    int splitIndex = chooseSplitIndex(puntos, eje);
    
    // Crear nuevo nodo
    nuevo_nodo = new Nodo(true);
    nuevo_nodo->m_nivel = nodo->m_nivel;
    nuevo_nodo->padre = nodo->padre;
    
    // Distribuir puntos
    vector<PointID> puntosNodoOriginal(puntos.begin(), puntos.begin() + splitIndex);
    vector<PointID> puntosNuevoNodo(puntos.begin() + splitIndex, puntos.end());
    
    nodo->puntos = puntosNodoOriginal;
    nuevo_nodo->puntos = puntosNuevoNodo;
    
    // Actualizar MBRs
    updateMBR(nodo);
    updateMBR(nuevo_nodo);
}

int GeoCluster::chooseSplitAxis(const vector<Punto>& puntos) {
    // Verificar que haya suficientes puntos
    if (puntos.size() < 2) {
        return 0; // Default a latitud
    }
    
    // Crear MBRs para latitud y longitud (optimizado)
    MBR mbrLatitud, mbrLongitud;
    
    for (const auto& punto : puntos) {
        mbrLatitud.m_minp[0] = min(mbrLatitud.m_minp[0], punto.latitud);
        mbrLatitud.m_maxp[0] = max(mbrLatitud.m_maxp[0], punto.latitud);
        mbrLongitud.m_minp[1] = min(mbrLongitud.m_minp[1], punto.longitud);
        mbrLongitud.m_maxp[1] = max(mbrLongitud.m_maxp[1], punto.longitud);
    }
    
    double margenLat = mbrLatitud.margin();
    double margenLong = mbrLongitud.margin();

    return (margenLat < margenLong) ? 0 : 1;
}

// NUEVA: Función para elegir eje de split con PointID
int GeoCluster::chooseSplitAxis(const vector<PointID>& puntos) {
    // Verificar que haya suficientes puntos
    if (puntos.size() < 2) {
        return 0; // Default a latitud
    }
    
    // Crear MBRs para latitud y longitud (optimizado)
    MBR mbrLatitud, mbrLongitud;
    
    for (const auto& punto : puntos) {
        mbrLatitud.m_minp[0] = min(mbrLatitud.m_minp[0], punto.latitud);
        mbrLatitud.m_maxp[0] = max(mbrLatitud.m_maxp[0], punto.latitud);
        mbrLongitud.m_minp[1] = min(mbrLongitud.m_minp[1], punto.longitud);
        mbrLongitud.m_maxp[1] = max(mbrLongitud.m_maxp[1], punto.longitud);
    }
    
    double margenLat = mbrLatitud.margin();
    double margenLong = mbrLongitud.margin();

    return (margenLat < margenLong) ? 0 : 1;
}

int GeoCluster::chooseSplitIndex(vector<Punto>& puntos, int eje) {
    // Verificar que haya suficientes puntos
    if (puntos.size() < 2) {
        return 1;
    }
    
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
    if (numero_de_distribuciones <= 0) {
        return m; // Fallback
    }
    
    double minOverlap = numeric_limits<double>::infinity();
    double minMargen = numeric_limits<double>::infinity();
    double minArea = numeric_limits<double>::infinity();
    int bestSplitIndex = m;

    for (int i = 0; i < numero_de_distribuciones; ++i) {
        int grupo1_size = m + i;
        
        if (grupo1_size >= puntos.size()) break;
        
        vector<Punto> grupo1(puntos.begin(), puntos.begin() + grupo1_size);
        vector<Punto> grupo2(puntos.begin() + grupo1_size, puntos.end());

        MBR mbrGrupo1 = calcularMBR(grupo1);
        MBR mbrGrupo2 = calcularMBR(grupo2);

        double overlap = mbrGrupo1.overlap(mbrGrupo2);
        double margen = mbrGrupo1.margin() + mbrGrupo2.margin();
        double area = mbrGrupo1.area() + mbrGrupo2.area();

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

// NUEVA: Función para elegir índice de split con PointID
int GeoCluster::chooseSplitIndex(vector<PointID>& puntos, int eje) {
    // Verificar que haya suficientes puntos
    if (puntos.size() < 2) {
        return 1;
    }
    
    int M = puntos.size();
    int m = MIN_PUNTOS_POR_NODO;

    if (eje == 0) {
        sort(puntos.begin(), puntos.end(), [](const PointID& p1, const PointID& p2) {
            return p1.latitud < p2.latitud;
        });
    } else {
        sort(puntos.begin(), puntos.end(), [](const PointID& p1, const PointID& p2) {
            return p1.longitud < p2.longitud;
        });
    }

    int numero_de_distribuciones = M - 2 * m + 2;
    if (numero_de_distribuciones <= 0) {
        return m; // Fallback
    }
    
    double minOverlap = numeric_limits<double>::infinity();
    double minMargen = numeric_limits<double>::infinity();
    double minArea = numeric_limits<double>::infinity();
    int bestSplitIndex = m;

    for (int i = 0; i < numero_de_distribuciones; ++i) {
        int grupo1_size = m + i;
        
        if (grupo1_size >= puntos.size()) break;
        
        vector<PointID> grupo1(puntos.begin(), puntos.begin() + grupo1_size);
        vector<PointID> grupo2(puntos.begin() + grupo1_size, puntos.end());

        MBR mbrGrupo1 = calcularMBR(grupo1);
        MBR mbrGrupo2 = calcularMBR(grupo2);

        double overlap = mbrGrupo1.overlap(mbrGrupo2);
        double margen = mbrGrupo1.margin() + mbrGrupo2.margin();
        double area = mbrGrupo1.area() + mbrGrupo2.area();

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
    // Verificar que el nodo sea válido
    if (nodo_hoja == nullptr) {
        cerr << "Error: Nodo nullptr en insertIntoLeafNode" << endl;
        return;
    }
    
    // Verificar que sea una hoja
    if (!nodo_hoja->esHoja) {
        cerr << "Error: Intentando insertar en nodo no-hoja" << endl;
        return;
    }
    
    // NUEVO: Convertir Punto a PointID para ahorrar memoria
    PointID point_id(punto);
    nodo_hoja->puntos.push_back(point_id);
    updateMBR(nodo_hoja);
}

void GeoCluster::insertIntoParent(Nodo* padre, Nodo* nuevo_nodo) {
    // Verificar que los nodos sean válidos
    if (padre == nullptr || nuevo_nodo == nullptr) {
        cerr << "Error: Nodos nullptr en insertIntoParent" << endl;
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
        for (const auto& point_id : nodo->puntos) {
            if (point_id.latitud >= rango.m_minp[0] && point_id.latitud <= rango.m_maxp[0] &&
                point_id.longitud >= rango.m_minp[1] && point_id.longitud <= rango.m_maxp[1]) {
                // NUEVO: Recuperar el punto completo de la hash table
                Punto punto_completo = obtenerPuntoCompleto(point_id.id);
                if (punto_completo.id != 0) { // Verificar que se encontró el punto
                    puntos_encontrados.push_back(punto_completo);
                }
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
            for (const auto& point_id : nodo->puntos) {
                if (estaDentroDelMBR(point_id, rango)) {
                    // NUEVO: Recuperar el punto completo de la hash table
                    Punto punto_completo = obtenerPuntoCompleto(point_id.id);
                    if (punto_completo.id != 0) { // Verificar que se encontró el punto
                        puntos_similares.push_back(punto_completo);
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
            Punto punto_completo = obtenerPuntoCompleto(point_id.id);
            if (punto_completo.id != 0) { // Verificar que se encontró el punto
                grupos_por_subcluster[punto_completo.id_subcluster_atributivo].push_back(punto_completo);
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
        // Recuperar el punto completo de la hash table
        Punto punto = obtenerPuntoCompleto(point_id.id);
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

// Funciones auxiliares que faltan
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
