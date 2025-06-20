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

// Parámetros optimizados para 500,000 puntos
// Cálculo: sqrt(500,000) * 0.5 = 353
const int MAX_PUNTOS_POR_NODO = 350; // Máximo de puntos por nodo (reducido para estabilidad)
const int MIN_PUNTOS_POR_NODO = 140;  // Mínimo de puntos por nodo (40% del máximo)
const double PORCENTAJE_PARA_HACER_REINSERT = 0.3; // Para poder hacer reinsert


//Estructura Punto
struct Punto{
    int id;
    double latitud, longitud;
    vector <double> atributos; //12 attributos a 6-8 aproximadamente
    int id_cluster_geografico, id_subcluster_atributivo;
    
    // Constructor por defecto
    Punto() : id(0), latitud(0.0), longitud(0.0), atributos({}) {}

    // Constructor para inicializar el punto
    Punto(int _id, double _lat, double _lon, const std::vector<double>& _atributos)
        : id(_id), latitud(_lat), longitud(_lon), atributos(_atributos) {}
    
        // Operador < para permitir ordenamiento
    bool operator< (const Punto& other) const {
        if (latitud != other.latitud) {
            return latitud < other.latitud;
        }
        if (longitud != other.longitud) {
            return longitud < other.longitud;
        }
        return id < other.id;
    }
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
    //int cantidad;
    int id_subclaster_atributivo; //id del subcluster al que pertenece
    vector<Punto> puntos; //Puntos que pertenecen a este microcluster
    
    MicroCluster(vector<double> centro, double r, int id_subclaster)
        : centroId(centro), radio(r), id_subclaster_atributivo(id_subclaster) {}   
};

//Estructura Nodo
struct Nodo{
    bool esHoja;
    int m_nivel;
    MBR mbr;
    vector<Nodo*> hijos;  
    vector<Punto> puntos; //Solo para las hojas
    vector<MicroCluster> microclusters_en_Hoja; //Solo para las hojas
    Nodo* padre; //Puntero al padre
    Nodo(bool esHoja = false) : esHoja(esHoja), m_nivel(0), padre(nullptr) {}
};

//Clase GeoCluster-Tree
class GeoCluster {  
public:
    GeoCluster(); //Constructor
    ~GeoCluster(); //Destructor
    Nodo* getRaiz(){ return raiz;}
    
    //Funciones para insercion de datos
    void inserData(const Punto& punto);
        void insertar(const Punto& punto,int nivel);

    //FUNCIONES 1 Y 2
    vector<Punto> n_puntos_similiares_a_punto(const Punto& punto_de_busqueda, MBR& rango, int numero_de_puntos_similares);
    vector<vector<Punto>> grupos_similares_de_puntos(MBR& rango);
    
    vector<Punto> buscarPuntosDentroInterseccion(const MBR& rango, Nodo* nodo);
    MBR calcularMBR(const vector<Punto>& puntos);
    void imprimirArbol(Nodo* nodo = nullptr, int nivel = 0);
    int contarPuntosEnArbol(Nodo* nodo = nullptr);
    void verificarDuplicados();

private:
    Nodo* raiz = nullptr;
    unordered_map<int,bool> niveles_reinsert;

    Nodo* chooseSubTree(Nodo* N,const Punto punto, int nivel);
        double calcularCostoOverlap(const MBR& mbr, const Punto& punto);
        double calcularCostoDeAmpliacionArea(const MBR& mbr, const Punto& punto);
        double calcularMBRArea(const MBR& mbr);

    bool OverFlowTreatment(Nodo*N, const Punto& punto, int nivel);   
        void reinsert(Nodo* N, const Punto& punto);
            Punto calcularCentroNodo(Nodo* nodo);
            double calcularDistancia(const Punto& p1, const Punto& p2);
            Nodo* encontrarPadre(Nodo* raiz, Nodo* hijo);
            void propagateSplit(Nodo* nodo_original, Nodo* nuevo_nodo);
        void Split(Nodo* nodo, Nodo*& nuevo_nodo);
            int chooseSplitAxis(const vector<Punto>& puntos);
                double calcularMargen(const MBR& mbr);
            int chooseSplitIndex(vector<Punto>& puntos, int eje);
                double calcularOverlap(const MBR& mbr1, const MBR& mbr2);

    
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
    
    // Funciones auxiliares para verificación
    void verificarDuplicadosRec(Nodo* nodo, vector<int>& ids);
};

#endif