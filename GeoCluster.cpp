#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cmath>
using namespace std;

//Estructura Punto
struct Punto{
    int id;
    double latitud, longitud;
    vector <double> atributos; //20 attributos a 10 aproximadamente
    int id_inicial_cluster;
};

//Estructura MicroCluster
struct MicroCluster{
    vector <double> centroId;
    double radio;  //Maxima distancia al centroide
    int cantidad; //Numero de puntos
};

//Estructura Nodo GeoCluster
struct GeoClusterNodo{
    double mbr[4]; //lat y long
    vector<MicroCluster> micro_clusters;    //CANTIDAD DE MICROCLUSTERS: 3
    vector<GeoClusterNodo*> children;  //vector de punteros de estructuras geoclusternodo
    vector<Punto> puntos;    //hojas 50 por nodo quizas?
    bool esHoja;
};

struct Rectangulo_Busqueda{
    double lat_min, lat_max, long_min, long_max;
    Rectangulo_Busqueda(double lat_min, double lat_max, double long_min, double long_max) {
        this->lat_min = lat_min;
        this->lat_max = lat_max;
        this->long_min = long_min;
        this->long_max = long_max;
    }
};


bool interseccion_mbr(const MicroCluster& mbr1, const Rectangulo_Busqueda& mbr2) {
    // Verifica si hay intersección entre el MBR y el rectángulo de búsqueda
    return !(mbr1.centroId[0] < mbr2.long_min || mbr1.centroId[0] > mbr2.long_max ||
             mbr1.centroId[1] < mbr2.lat_min || mbr1.centroId[1] > mbr2.lat_max);
}

