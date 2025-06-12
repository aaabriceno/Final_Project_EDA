#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <array>
#include <unordered_map>
using namespace std;

//const int MAX_NIVEL = 5; //Maximo nivel de nodos
const int MAX_PUNTOS_POR_NODO = 50; //Maximo de puntos por nodo
const int MIN_PUNTOS_POR_NODO = 20;
//Estructura Punto
struct Punto{
    int id;
    double latitud, longitud;
    vector <double> atributos; //12 attributos a 6-8 aproximadamente
    int id_cluster_geografico, id_subcluster_atributivo;
};

//Estructura MBR
struct MBR{
    double m_minp[2]; // [latitud minima, longitud minima]
    double m_maxp[2]; // [latitud maxima, longitud maxima]

    MBR(){}
    MBR(double min_latitud, double min_longitud, double max_latitud, double max_longitud) {
        m_minp[0] = min_latitud; // latitud minima
        m_minp[1] = min_longitud; // longitud minima
        m_maxp[0] = max_latitud; // latitud maxima
        m_maxp[1] = max_longitud; // longitud maxima
    }
};

//Estructura MicroCluster
struct MicroCluster{
    vector <double> centroId;
    double radio;  //Maxima distancia al centroide
    int cantidad;
    int id_subclaster_atributivo; //id del subcluster al que pertenece
    vector<Punto> puntos; //Puntos que pertenecen a este microcluster
    
    MicroCluster(vector<double> centro, double r, int id_subclaster)
        : centroId(centro), radio(r), cantidad(0), id_subclaster_atributivo(id_subclaster) {}   
};

//Estructura Nodo GeoCluster
struct Nodo{
    int m_level; //nivel del nodo
    MBR mbr; 
    vector<MicroCluster> micro_clusters;    //CANTIDAD DE MICROCLUSTERS: x
    vector<Nodo*> hijo;  //vector de punteros de estructuras Nodo
    vector<Punto> puntos;    //hojas 50 por nodo quizas?

    bool esNodoRama() {return (m_level> 0);}
    bool esNodoHoja() { return (m_level == 0); }
};


bool interseccion_mbr(const MicroCluster& mbr1, const MBR& rect) {
    double lat = mbr1.centroId[0];
    double lon = mbr1.centroId[1];

    return !(lon < rect.m_minp[1] || lon > rect.m_maxp[1] ||
             lat < rect.m_minp[0]  || lat > rect.m_maxp[0]);
}

class GeoCluster {  
public:
    GeoCluster();
    ~GeoCluster();
    void insertarPunto(const Punto& punto);
    vector<Punto> n_puntos_similiares_a_punto(const Punto& punto_de_busqueda, MBR& rango,int numero_de_puntos_similares);
    vector<vector<Punto>> grupos_similares_de_puntos(MBR& rango);

private:
    Nodo* raiz;

    //funcion chooseSubTree y sus funciones internas
    Nodo* chooseSubTree(const Punto& punto);
    double calcularOverlapCosto(const MBR& mbr, const Punto& punto);
    double calcularAreaCosto(const MBR& mbr, const Punto& punto);
    double calcularMBRArea(const MBR& mbr);

    //funcion Split y sus funciones internas
    void Split(Nodo* nodo, Nodo*& nuevo_nodo);
    int chooseSplitAxis(const vector<Punto>& puntos);
    double calcularMargen(const vector<Punto>& puntosLat);
    MBR calcularMBR(const vector<Punto>& puntos);
    double calcularOverlap(const MBR& mbr1, const MBR& mbr2);
    int chooseSplitIndex(vector<Punto>& puntos, int eje);

    //
    Nodo* findBestLeafNode(const Punto& punto);
    void insertIntoLeafNode(Nodo* nodo_hoja, const Punto& punto);
    
    void insertIntoParent(Nodo* nodo_hoja, Nodo* nuevo_nodo);
    void updateMBR(Nodo* nodo);
    bool interseccionMBR(const MBR& mbr1, const MBR& mbr2);
    void searchRec(const MBR& rango, Nodo* nodo, vector<Punto>& puntos_similares);
};


MBR GeoCluster::calcularMBR(const vector<Punto>& puntos) {
    double minLat = puntos[0].latitud, maxLat = puntos[0].latitud;
    double minLon = puntos[0].longitud, maxLon = puntos[0].longitud;

    // Calcular los valores mínimo y máximo de latitud y longitud
    for (const auto& punto : puntos) {
        minLat = std::min(minLat, punto.latitud);
        maxLat = std::max(maxLat, punto.latitud);
        minLon = std::min(minLon, punto.longitud);
        maxLon = std::max(maxLon, punto.longitud);
    }

    // Crear el MBR que cubre todos los puntos
    return MBR(minLat, minLon, maxLat, maxLon);
}

double GeoCluster::calcularOverlap(const MBR& mbr1, const MBR& mbr2) {
    double lat_overlap = std::max(0.0, std::min(mbr1.m_maxp[0], mbr2.m_maxp[0]) - std::max(mbr1.m_minp[0], mbr2.m_minp[0]));
    double lon_overlap = std::max(0.0, std::min(mbr1.m_maxp[1], mbr2.m_maxp[1]) - std::max(mbr1.m_minp[1], mbr2.m_minp[1]));

    return lat_overlap * lon_overlap;  // El área de la intersección
}


int GeoCluster::chooseSplitAxis(const vector<Punto>& puntos){
    //double margeLat = 0.0, margenLong = 0.0;  
    //Paso 1: Ordenar por latitud(eje 0) y longitud(eje 1)
    vector<Punto> puntosLatitud = puntos;
    vector<Punto> puntosLongitud = puntos;

    //ordenar los puntos 
    sort(puntosLatitud.begin(), puntosLatitud.end(), [](const Punto& p1, const Punto& p2) {
        return p1.latitud < p2.latitud;
    });

    sort(puntosLongitud.begin(), puntosLatitud.end(),[](const Punto& p1, const Punto& p2) {
        return p1.longitud < p2.longitud;
    });

    //Paso 2: Calcular el numero de distribuciones (M - 2m + 2) para cada eje
    int M = MAX_PUNTOS_POR_NODO;
    int m = M/2;
    
    int numero_distribuciones_lat = M - 2*m + 2;
    int numero_distribuciones_long = M - 2*m + 2;

    //Paso 3:Calcular el margen para cada eje
    double margenLat = calcularMargen(puntosLatitud);
    double margenLong = calcularMargen(puntosLongitud);

    return (margenLat < margenLong) ? 0:1;
}

int GeoCluster::chooseSplitIndex(vector<Punto>& puntos, int eje) {
    int M = puntos.size();  // Número total de puntos
    int m = M / 2;  // Aproximadamente la mitad de los puntos

    // Ordenar los puntos según el eje seleccionado (latitud o longitud)
    if (eje == 0) {
        sort(puntos.begin(), puntos.end(), [](const Punto& p1, const Punto& p2) {
            return p1.latitud < p2.latitud;  // Ordenar por latitud
        });
    } else {
        sort(puntos.begin(), puntos.end(), [](const Punto& p1, const Punto& p2) {
            return p1.longitud < p2.longitud;  // Ordenar por longitud
        });
    }

    // Paso 1: Calcular las distribuciones posibles (M - 2m + 2)
    // Para cada índice de división, calculamos el solapamiento de los MBRs
    double minOverlap = numeric_limits<double>::infinity();
    int bestSplitIndex = 0;

    for (int i = 1; i < M; ++i) {  // Para cada posible división
        vector<Punto> grupo1(puntos.begin(), puntos.begin() + i);  // Grupo 1 con i puntos
        vector<Punto> grupo2(puntos.begin() + i, puntos.end());   // Grupo 2 con el resto de los puntos

        // Paso 2: Calcular los MBRs para cada grupo
        MBR mbrGrupo1 = calcularMBR(grupo1);
        MBR mbrGrupo2 = calcularMBR(grupo2);

        // Paso 3: Calcular el solapamiento entre los MBRs
        double overlap = calcularOverlap(mbrGrupo1, mbrGrupo2);

        // Paso 4: Seleccionar el índice con el menor solapamiento
        if (overlap < minOverlap) {
            minOverlap = overlap;
            bestSplitIndex = i;
        }
    }

    return bestSplitIndex;  // Devolver el índice con el menor solapamiento
}


void Split(Nodo* nodo, Nodo* nuevo_nodo){

}

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

//funcion para elegir el subarbol adecuado para insertar un punto
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
