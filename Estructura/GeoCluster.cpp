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

bool interseccion_mbr_microcluster(const MicroCluster& mbr1, const MBR& rect) {
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
    vector<Punto> buscarPuntosDentroMBR(const MBR& rango);
    MBR calcularMBR(const vector<Punto>& puntos);
private:
    Nodo* raiz;

    //funcion chooseSubTree y sus funciones internas
    double calcularOverlapCosto(const MBR& mbr, const Punto& punto);
    double calcularAreaCosto(const MBR& mbr, const Punto& punto);
    double calcularMBRArea(const MBR& mbr);
    Nodo* chooseSubTree(const Punto& punto);
    
    
    //funcion Split y sus funciones internas
    int chooseSplitAxis(const vector<Punto>& puntos);
    int chooseSplitIndex(vector<Punto>& puntos, int eje);
    double calcularOverlap(const MBR& mbr1, const MBR& mbr2);
    double calcularMargen(const MBR& mbr);
    
    
    void Split(Nodo* nodo, Nodo*& nuevo_nodo);
    

    //
    Nodo* findBestLeafNode(const Punto& punto);
    void insertIntoLeafNode(Nodo* nodo_hoja, const Punto& punto);
    void insertIntoParent(Nodo* nodo_hoja, Nodo* nuevo_nodo);
    void updateMBR(Nodo* nodo);
    bool interseccionMBR(const MBR& mbr1, const MBR& mbr2);
    void searchRec(const MBR& rango, Nodo* nodo, vector<Punto>& puntos_similares);
};

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

vector<Punto> GeoCluster::buscarPuntosDentroMBR(const MBR& rango) {
    vector<Punto> puntosEncontrados;
    // Llamar a la función recursiva para buscar en todo el árbol
    searchRec(rango, raiz, puntosEncontrados);
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

bool GeoCluster::interseccionMBR(const MBR& mbr1, const MBR& mbr2) {
    // Verificar si hay superposición en el eje de latitud
    bool overlapLat = !(mbr1.m_maxp[0] <= mbr2.m_minp[0] || mbr1.m_minp[0] >= mbr2.m_maxp[0]);
    
    // Verificar si hay superposición en el eje de longitud
    bool overlapLon = !(mbr1.m_maxp[1] <= mbr2.m_minp[1] || mbr1.m_minp[1] >= mbr2.m_maxp[1]);

    // Los MBRs se solapan si ambos ejes se solapan
    return overlapLat && overlapLon;
}


int main() {
    // Crear un objeto de la clase GeoCluster
    GeoCluster geoCluster;

    // Crear algunos puntos de ejemplo
    vector<Punto> puntos = {
        {1, 19.5, -70.5, {1.0, 2.0, 3.0}},
        {2, 20.0, -70.2, {1.5, 2.5, 3.5}},
        {3, 21.0, -71.0, {2.0, 3.0, 4.0}},
        {4, 22.0, -69.5, {3.0, 4.0, 5.0}},
        {5, 23.0, -68.0, {4.0, 5.0, 6.0}},
        {6, 20.5, -70.0, {1.2, 2.2, 3.2}},
        {7, 21.5, -69.5, {2.5, 3.5, 4.5}},
    };

    // Insertamos los puntos en el árbol
    for (const auto& punto : puntos) {
        geoCluster.insertarPunto(punto);
    }

    // Calcular y mostrar el MBR de todos los puntos
    MBR mbr = geoCluster.calcularMBR(puntos);

    // Imprimir el MBR
    cout << "\nMBR de los 7 puntos:" << endl;
    cout << "Latitud mínima: " << mbr.m_minp[0] << ", Longitud mínima: " << mbr.m_minp[1] << endl;
    cout << "Latitud máxima: " << mbr.m_maxp[0] << ", Longitud máxima: " << mbr.m_maxp[1] << endl;

    return 0;
}