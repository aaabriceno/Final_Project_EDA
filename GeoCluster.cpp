#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <array>
#include <unordered_map>
using namespace std;

//Estructura Punto
struct Punto{
    int id;
    double latitud, longitud;
    vector <double> atributos; //20 attributos a 10 aproximadamente
    int id_cluster_geografico, id_subcluster_atributivo;
};

//Estructura Coordenada
struct Coordenada{
    double latitud;
    double longitud;
    Coordenada(double lat, double lon) : latitud(lat), longitud(lon) {}
};

//Estructura MicroCluster
struct MicroCluster{
    vector <double> centroId;
    double radio;  //Maxima distancia al centroide
    int cantidad;
    int id_subclaster_atributivo; //id del subcluster al que pertenece
};

//Estructura Nodo GeoCluster
struct GeoClusterNodo{
    int m_level; //nivel del nodo
    
    array <double,4> mbr; // mbr[0] = lat_min, mbr[1] = long_min, mbr[2] = lat_max, mbr[3] = long_max
    vector<MicroCluster> micro_clusters;    //CANTIDAD DE MICROCLUSTERS: 3
    vector<GeoClusterNodo*> hijo;  //vector de punteros de estructuras geoclusternodo
    vector<Punto> puntos;    //hojas 50 por nodo quizas?
    

    bool esNodoRama() {return (m_level> 0);}
    bool esNodoHoja() { return (m_level == 0); }
};

struct MBR{
    Coordenada punto_min;
    Coordenada punto_max;
    MBR(Coordenada min, Coordenada max) : punto_min(min), punto_max(max) {}
};

bool interseccion_mbr(const MicroCluster& mbr1, const MBR& rect) {
    double lat = mbr1.centroId[0];
    double lon = mbr1.centroId[1];

    return !(lon < rect.punto_min.longitud || lon > rect.punto_max.longitud ||
             lat < rect.punto_min.latitud  || lat > rect.punto_max.latitud);
}




class GeoCluster {
    
public:
    GeoCluster();
    ~GeoCluster();
    void insertarPunto(const Punto& punto);
    vector<Punto> n_puntos_similiares_a_punto(const Punto& punto_de_busqueda, MBR& rango,int numero_de_puntos_similares);

};