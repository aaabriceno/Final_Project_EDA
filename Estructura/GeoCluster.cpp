#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <array>
#include <unordered_map>
using namespace std;

const int MAX_NIVEL = 5; //Maximo nivel de nodos
const int MAX_PUNTOS_POR_NODO = 50; //Maximo de puntos por nodo

//Estructura Punto
struct Punto{
    int id;
    double latitud, longitud;
    vector <double> atributos; //12 attributos a 6-8 aproximadamente
    int id_cluster_geografico, id_subcluster_atributivo;
};


//Estructura Coordenada
/*struct Coordenada{
    double latitud;
    double longitud;
    Coordenada(double lat, double lon) : latitud(lat), longitud(lon) {}
};
*/


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

struct Rama{
    MBR mbr_rama; // MBR de la rama
    Nodo* nodo_hijo; // Puntero al nodo hijo
    
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
    Nodo* buscarMejorNodoHoja(const Punto& punto);
    void insertarEnElNodoHoja(Nodo* nodo_hoja, const Punto& punto);
    void dividirNodo(Nodo* nodo, Nodo*& nuevo_nodo);
    void insertarEnElPadre(Nodo* nodo_hoja, Nodo* nuevo_nodo);
    void actualizarMBR(Nodo* nodo);
    bool interseccionMBR(const MBR& mbr1, const MBR& mbr2);
    void searchRec(const MBR& rango, Nodo* nodo, vector<Punto>& puntos_similares);
};


void GeoCluster::insertarPunto(const Punto& punto) {
    // Paso 1: Encontrar el nodo adecuado para insertar el punto
    Nodo* nodo_hoja = buscarMejorNodoHoja(punto);

    // Paso 2: Insertar el punto en el nodo hoja
    insertarEnElNodoHoja(nodo_hoja, punto);

    // Paso 3: Si el nodo hoja está lleno, dividirlo
    if (nodo_hoja->puntos.size() > MAX_PUNTOS_POR_NODO) {
        Nodo* nuevo_nodo = nullptr;
        dividirNodo(nodo_hoja, nuevo_nodo);

        // Paso 4: Propagar la división hacia los nodos internos si es necesario
        if (nodo_hoja->esNodoRama()) {
            insertarEnElPadre(nodo_hoja, nuevo_nodo);
        }
    }
}


void GeoCluster::dividirNodo(Nodo* nodo, Nodo*& nuevo_nodo){
    // Paso 1: Calcular la mejor partición de los puntos del nodo
    PartitionVars parVars;
    findBestSplit(nodo, parVars);

    // Paso 2: Crear el nuevo nodo
    nuevo_nodo = new Nodo();

    // Paso 3: Redistribuir los puntos entre los dos nodos
    for (int i = 0; i < parVars.m_count[1]; i++) {
        nodo->puntos.push_back(parVars.m_branchBuf[i].m_child);  // Agregar puntos al nuevo nodo
    }

    // Paso 4: Actualizar el MBR de los nodos
    updateMBR(nodo);
    updateMBR(nuevo_nodo);
}


vector<Punto> GeoCluster::n_puntos_similiares_a_punto(const Punto& punto_de_busqueda, MBR& rango, int numero_de_puntos_similares) {
    vector<Punto> puntos_similares;

    // Paso 1: Buscar los nodos que intersectan con el MBR de la consulta
    searchRec(rango, raiz, puntos_similares);

    // Paso 2: Calcular la similitud de los puntos encontrados en los microclusters
    for (auto& punto : puntos_similares) {
        punto.similarity = computeSimilarity(punto_de_busqueda, punto); // Calcular similitud
    }

    // Paso 3: Ordenar los puntos por similitud (más cercano primero)
    sort(puntos_similares.begin(), puntos_similares.end(), 
         [](const Punto& a, const Punto& b) { return a.similarity < b.similarity; });

    // Paso 4: Limitar la cantidad de puntos si es necesario
    if (numero_de_puntos_similares > 0 && puntos_similares.size() > numero_de_puntos_similares) {
        puntos_similares.resize(numero_de_puntos_similares);
    }

    return puntos_similares;
}
