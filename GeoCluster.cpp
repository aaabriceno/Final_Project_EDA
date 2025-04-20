#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
//#include <Eigen/dense>
#include <vector>
using namespace std;

//Estructura Punto
struct Punto{
    int id;
    double latitud, longitud;
    vector <int> atributos;
    int id_inicial_cluster;
};

//Estructura MicroCluster
struct MicroCluster{
    vector <int> centroId;
    int radio;  //DistanciaMaxima,debo buscar que algoritmo implmentar
    int cantidad; //Numero de puntos
};

//Estructura Nodo GeoCluster
struct GeoClusterNodo{
    double mbr[4]; //lat y long
    vector<MicroCluster> micro_clusters;    //CANTIDAD DE MICROCLUSTERS
    vector<GeoClusterNodo*> children;  //vector de punteros de estructuras geoclusternodo
    vector<Punto> puntos;    //hojas 50 por nodo quizas?
    bool esHoja;
};



