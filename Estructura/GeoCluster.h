#ifndef GEOCLUSTER_H
#define GEOCLUSTER_H
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <array>
#include <unordered_map>
#include <iostream>

using namespace std;

//const int MAX_NIVEL = 5; //Maximo nivel de nodos
const int MAX_PUNTOS_POR_NODO = 30; //Maximo de puntos por nodo
const int MIN_PUNTOS_POR_NODO = 2;  //Minimo de puntos por nodo

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
    double radio;  
    int cantidad;
    int id_subclaster_atributivo; //id del subcluster al que pertenece
    vector<Punto> puntos; //Puntos que pertenecen a este microcluster
    
    MicroCluster(vector<double> centro, double r, int id_subclaster)
        : centroId(centro), radio(r), cantidad(0), id_subclaster_atributivo(id_subclaster) {}   
};

//Estructura Nodo GeoCluster
struct Nodo{
    bool esHoja;
    MBR mbr;
    vector<Nodo*> hijos;  
    Nodo(bool esHoja = false):esHoja(esHoja){ }
};

struct Hoja{
    vector<Punto> puntos;
    vector<MicroCluster> microCluster_hoja;
    MBR mbr_hoja;
};

//Clase GeoCluster - Tree
class GeoCluster {  
public:
    GeoCluster(); //Constructor
    ~GeoCluster(); //Destructor
    Nodo* getRaiz(){ return raiz;} //retorna el nodo raiz (private)
    
    //Funcion que inserta los puntos
    void insertarPunto(const Punto& punto);
    //Funcion consulta 1
    vector<Punto> n_puntos_similiares_a_punto(const Punto& punto_de_busqueda, MBR& rango,int numero_de_puntos_similares);
    //Funcion consulta 2
    vector<vector<Punto>> grupos_similares_de_puntos(MBR& rango);
    
    vector<Punto> buscarPuntosDentroInterseccion(const MBR& rango, Nodo* nodo);
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
    

    //Funciones Extras
    Nodo* findBestLeafNode(const Punto& punto);
    void insertIntoLeafNode(Nodo* nodo_hoja, const Punto& punto);
    void insertIntoParent(Nodo* nodo_hoja, Nodo* nuevo_nodo);
    void updateMBR(Nodo* nodo);
    bool interseccionMBR(const MBR& mbr1, const MBR& mbr2);
    void searchRec(const MBR& rango, Nodo* nodo, vector<Punto>& puntos_similares);
    MBR computeMBR(double ax1, double ay1, double ax2, double ay2,
                   double bx1, double by1, double bx2, double by2);
    bool estaDentroDelMBR(const Punto& punto, const MBR& mbr);
};

#endif
