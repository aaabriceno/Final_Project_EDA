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
const int MAX_PUNTOS_POR_NODO = 6; //Maximo de puntos por nodo
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

    void reset(){
         m_minp[0] = m_minp[1] = std::numeric_limits<double>::max();
        m_maxp[0] = m_maxp[1] = std::numeric_limits<double>::lowest();
    }

    // Método que uso para  estirar el MBR para incluir otro MBR
    void stretch(const MBR& other) {
        m_minp[0] = std::min(m_minp[0], other.m_minp[0]);
        m_minp[1] = std::min(m_minp[1], other.m_minp[1]);
        m_maxp[0] = std::max(m_maxp[0], other.m_maxp[0]);
        m_maxp[1] = std::max(m_maxp[1], other.m_maxp[1]);
    }

    // Método que uso para verificar si dos MBRs se intersectan
    bool is_intersected(const MBR& other) const {
        return !(m_maxp[0] < other.m_minp[0] || m_maxp[1] < other.m_minp[1] || 
                 m_minp[0] > other.m_maxp[0] || m_minp[1] > other.m_maxp[1]);
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
    int m_nivel;
    MBR mbr;
    vector<Nodo*> hijos;  
    vector<Punto> puntos; //Solo para las hojas
    vector<MicroCluster> microclusters_en_Hoja;
    Nodo(bool esHoja = false) : esHoja(esHoja), m_nivel(0) {}
};

//Clase GeoCluster - Tree
class GeoCluster {  
public:
    GeoCluster();
    ~GeoCluster();
    Nodo* getRaiz(){ return raiz;}
    void InserData();
    void insertar(const Punto& punto);
    vector<Punto> n_puntos_similiares_a_punto(const Punto& punto_de_busqueda, MBR& rango, int numero_de_puntos_similares);
    vector<vector<Punto>> grupos_similares_de_puntos(MBR& rango);
    
    vector<Punto> buscarPuntosDentroInterseccion(const MBR& rango, Nodo* nodo);
    MBR calcularMBR(const vector<Punto>& puntos);

private:
    Nodo* raiz;
    vector<bool> niveles_de_arbol;

    double calcularOverlapCosto(const MBR& mbr, const Punto& punto);
    double calcularAreaCosto(const MBR& mbr, const Punto& punto);
    double calcularMBRArea(const MBR& mbr);
    Nodo* chooseSubTree(Nodo* nodo, const Punto& punto);
    
    int chooseSplitAxis(const vector<Punto>& puntos);
    int chooseSplitIndex(vector<Punto>& puntos, int eje);
    double calcularOverlap(const MBR& mbr1, const MBR& mbr2);
    double calcularMargen(const MBR& mbr);
    void Split(Nodo* nodo, Nodo*& nuevo_nodo);
    
    Nodo* findBestLeafNode(const Punto& punto);
    void insertIntoLeafNode(Nodo* nodo_hoja, const Punto& punto);
    void insertIntoParent(Nodo* padre, Nodo* nuevo_nodo);
    void updateMBR(Nodo* nodo);
    bool interseccionMBR(const MBR& mbr1, const MBR& mbr2);
    void searchRec(const MBR& rango, Nodo* nodo, vector<Punto>& puntos_similares);
    MBR computeMBR(double ax1, double ay1, double ax2, double ay2,
                   double bx1, double by1, double bx2, double by2);
    bool estaDentroDelMBR(const Punto& punto, const MBR& mbr);
    double computeArea(double ax1, double ay1, double ax2, double ay2,
                      double bx1, double by1, double bx2, double by2);
};

#endif
