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
#include <limits>
#include <cassert>
#include <memory>

using namespace std;

// Parámetros M y m del R*-tree (paper: 2 <= m <= M/2, mejor rendimiento con m = 40% de M)
// Sobreescribibles en compilación (-DGEO_MAX_ENTRIES=8) para tests con nodos pequeños
#ifndef GEO_MAX_ENTRIES
#define GEO_MAX_ENTRIES 1200
#endif
#ifndef GEO_MIN_ENTRIES
#define GEO_MIN_ENTRIES 480   /* 40% de M: el valor con mejor rendimiento en el paper */
#endif
const int MAX_PUNTOS_POR_NODO = GEO_MAX_ENTRIES;
const int MIN_PUNTOS_POR_NODO = GEO_MIN_ENTRIES;
const double PORCENTAJE_PARA_HACER_REINSERT = 0.3; // Para poder hacer reinsert

//Estructura Punto optimizada
struct Punto{
    int id;
    double latitud, longitud;
    vector <double> atributos; //10 atributos de cada punto/ 6-8 puntos con PCA
    int id_cluster_geografico, id_subcluster_atributivo;
    
    // Constructor por defecto optimizado
    Punto() : id(0), latitud(0.0), longitud(0.0), id_cluster_geografico(0), id_subcluster_atributivo(0) {}

    // Constructor para inicializar el punto
    Punto(int _id, double _lat, double _lon, const std::vector<double>& _atributos)
        : id(_id), latitud(_lat), longitud(_lon), atributos(_atributos), id_cluster_geografico(0), id_subcluster_atributivo(0) {}
    
    // Constructor de copia optimizado
    Punto(const Punto& otro) : id(otro.id), latitud(otro.latitud), longitud(otro.longitud), 
                               atributos(otro.atributos), id_cluster_geografico(otro.id_cluster_geografico), 
                               id_subcluster_atributivo(otro.id_subcluster_atributivo) {}
    
    // Operador de asignación optimizado
    Punto& operator=(const Punto& otro) {
        if (this != &otro) {
            id = otro.id;
            latitud = otro.latitud;
            longitud = otro.longitud;
            atributos = otro.atributos;
            id_cluster_geografico = otro.id_cluster_geografico;
            id_subcluster_atributivo = otro.id_subcluster_atributivo;
        }
        return *this;
    }
    
    // Operador < para permitir ordenamiento
    bool operator< (const Punto& otro_MBR) const {
        if (latitud != otro_MBR.latitud) {
            return latitud < otro_MBR.latitud;
        }
        if (longitud != otro_MBR.longitud) {
            return longitud < otro_MBR.longitud;
        }
        return id < otro_MBR.id;
    }
    
    // Método para optimizar memoria
    void optimizarAtributos() {
        if (atributos.size() > 8) {
            atributos.resize(8); // Mantener solo los primeros 8 atributos
        }
    }
};

// NUEVA: Estructura PointID optimizada para memoria
struct PointID {
    int id;                    // ID único del punto
    double latitud; 
    double longitud;  // Coordenadas geográficas
    int id_cluster_geografico; // ID del cluster geográfico
    int id_subcluster_atributivo; // ID del subcluster atributivo
    
    // Constructor por defecto
    PointID() : id(0), latitud(0.0), longitud(0.0), id_cluster_geografico(0), id_subcluster_atributivo(0) {}
    
    // Constructor para inicializar PointID
    PointID(int _id, double _lat, double _lon, int _cluster_geo, int _subcluster_attr)
        : id(_id), latitud(_lat), longitud(_lon), id_cluster_geografico(_cluster_geo), id_subcluster_atributivo(_subcluster_attr) {}
    
    // Constructor desde Punto (para conversión)
    PointID(const Punto& punto) 
        : id(punto.id), latitud(punto.latitud), longitud(punto.longitud), 
          id_cluster_geografico(punto.id_cluster_geografico), id_subcluster_atributivo(punto.id_subcluster_atributivo) {}
    
    // Operador < para permitir ordenamiento
    bool operator< (const PointID& otro) const {
        if (latitud != otro.latitud) {
            return latitud < otro.latitud;
        }
        if (longitud != otro.longitud) {
            return longitud < otro.longitud;
        }
        return id < otro.id;
    }
    
    // Operador == para comparaciones
    bool operator== (const PointID& otro) const {
        return id == otro.id;
    }
};

//Estructura MBR optimizada (basada en el código de ayuda)
struct MBR{
    double m_minp[2]; // [latitud minima, longitud minima]
    double m_maxp[2]; // [latitud maxima, longitud maxima]

    MBR(){
        reset();
    }
    
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

    // Método optimizado para estirar el MBR
    void stretch(const MBR& otro_MBR) {
        m_minp[0] = std::min(m_minp[0], otro_MBR.m_minp[0]);
        m_minp[1] = std::min(m_minp[1], otro_MBR.m_minp[1]);
        m_maxp[0] = std::max(m_maxp[0], otro_MBR.m_maxp[0]);
        m_maxp[1] = std::max(m_maxp[1], otro_MBR.m_maxp[1]);
    }

    // Método optimizado para verificar intersección
    bool is_intersected(const MBR& otro_MBR) const {
        return !(m_maxp[0] < otro_MBR.m_minp[0] || m_maxp[1] < otro_MBR.m_minp[1] || 
                 m_minp[0] > otro_MBR.m_maxp[0] || m_minp[1] > otro_MBR.m_maxp[1]);
    }
    
    // Verificar si el MBR es válido
    bool esValido() const {
        return m_minp[0] <= m_maxp[0] && m_minp[1] <= m_maxp[1];
    }
    
    // Método para calcular área (optimizado)
    double area() const {
        return (m_maxp[0] - m_minp[0]) * (m_maxp[1] - m_minp[1]);
    }
    
    // Método para calcular margen (optimizado)
    double margin() const {
        return 2 * ((m_maxp[0] - m_minp[0]) + (m_maxp[1] - m_minp[1]));
    }
    
    // Método para calcular overlap (optimizado)
    double overlap(const MBR& otro_MBR) const {
        double lat_overlap = std::max(0.0, std::min(m_maxp[0], otro_MBR.m_maxp[0]) - std::max(m_minp[0], otro_MBR.m_minp[0]));
        double lon_overlap = std::max(0.0, std::min(m_maxp[1], otro_MBR.m_maxp[1]) - std::max(m_minp[1], otro_MBR.m_minp[1]));
        return lat_overlap * lon_overlap;
    }
};

//Estructura MicroCluster optimizada
struct MicroCluster{
    vector <double> centroId;
    double radio;  
    int id_cluster_geografico;    // ID del cluster geográfico
    int id_subcluster_atributivo; // ID del subcluster atributivo
    vector<Punto> puntos; //Puntos que pertenecen a este microcluster
    
    MicroCluster(vector<double> centro, double r, int id_cluster_geo, int id_subcluster)
        : centroId(centro), radio(r), id_cluster_geografico(id_cluster_geo), 
          id_subcluster_atributivo(id_subcluster) {}   
    
    // Método para optimizar memoria
    void optimizarPuntos() {
        if (puntos.size() > 1000) {
            puntos.resize(1000); // Limitar puntos por microcluster
        }
    }
};

// Estructura para retornar resultados de búsqueda
struct ResultadoBusqueda {
    vector<Punto> puntos;
    vector<int> ids;
    vector<double> similitudes;  // Almacenar las similitudes en el mismo orden que los puntos
    
    ResultadoBusqueda() {}
    
    void agregar(const Punto& punto, double similitud) {
        puntos.push_back(punto);
        ids.push_back(punto.id);
        similitudes.push_back(similitud);
    }
};

// Estructura para matriz de similitud temporal del MBR
struct MatrizSimilitudMBR {
    // Hash table para almacenar similitudes: key = "id1_id2", value = similitud
    unordered_map<string, double> similitudes;
    // Hash table para almacenar rankings precalculados: key = id_punto, value = vector de {similitud, id_punto}
    unordered_map<int, vector<pair<double, int>>> rankings;
    
    // Función para generar la clave del hash
    string generarClave(int id1, int id2) {
        return to_string(min(id1, id2)) + "_" + to_string(max(id1, id2));
    }
    
    // Obtener similitud entre dos puntos (si está calculada)
    double obtenerSimilitud(int id1, int id2) {
        string clave = generarClave(id1, id2);
        auto it = similitudes.find(clave);
        return (it != similitudes.end()) ? it->second : -1.0;
    }
    
    // Almacenar similitud entre dos puntos
    void almacenarSimilitud(int id1, int id2, double similitud) {
        string clave = generarClave(id1, id2);
        similitudes[clave] = similitud;
    }
    
    // Almacenar ranking precalculado para un punto
    void almacenarRanking(int id_punto, const vector<pair<double, int>>& ranking) {
        rankings[id_punto] = ranking;
    }
    
    // Obtener ranking precalculado para un punto
    const vector<pair<double, int>>& obtenerRanking(int id_punto) {
        static vector<pair<double, int>> ranking_vacio;
        auto it = rankings.find(id_punto);
        return (it != rankings.end()) ? it->second : ranking_vacio;
    }
    
    // Limpiar todas las estructuras
    void limpiar() {
        similitudes.clear();
        rankings.clear();
    }
};

struct BusquedaOptimizada {
    // Hash table: cluster_geo_subcluster -> vector de puntos
    unordered_map<string, vector<Punto>> puntos_por_cluster;
    
    // Función para generar clave de cluster
    string generarClaveCluster(int cluster_geo, int subcluster) {
        return to_string(cluster_geo) + "_" + to_string(subcluster);
    }
    
    // Agregar punto a la estructura
    void agregarPunto(const Punto& punto, int nodo_id) {
        string clave = generarClaveCluster(punto.id_cluster_geografico, punto.id_subcluster_atributivo);
        puntos_por_cluster[clave].push_back(punto);
    }
    
    // Buscar puntos similares por cluster
    vector<Punto> buscarPuntosSimilares(const Punto& punto_busqueda, int n_puntos) {
        vector<Punto> resultado;
        
        // Prioridad 1: Mismo cluster geográfico y subcluster atributivo
        string clave_exacta = generarClaveCluster(punto_busqueda.id_cluster_geografico, punto_busqueda.id_subcluster_atributivo);
        auto it_exacto = puntos_por_cluster.find(clave_exacta);
        if (it_exacto != puntos_por_cluster.end()) {
            for (const auto& punto : it_exacto->second) {
                if (punto.id != punto_busqueda.id) {
                    resultado.push_back(punto);
                    if ((int)resultado.size() >= n_puntos) return resultado;
                }
            }
        }
        
        // Prioridad 2: Mismo cluster geográfico, subcluster diferente
        for (auto& par : puntos_por_cluster) {
            if (par.first != clave_exacta) {
                size_t pos_guion = par.first.find('_');
                if (pos_guion != string::npos) {
                    int cluster_geo = stoi(par.first.substr(0, pos_guion));
                    if (cluster_geo == punto_busqueda.id_cluster_geografico) {
                        for (const auto& punto : par.second) {
                            if (punto.id != punto_busqueda.id) {
                                resultado.push_back(punto);
                                if ((int)resultado.size() >= n_puntos) return resultado;
                            }
                        }
                    }
                }
            }
        }
        
        // Prioridad 3: Cualquier otro punto
        for (auto& par : puntos_por_cluster) {
            if (par.first != clave_exacta) {
                size_t pos_guion = par.first.find('_');
                if (pos_guion != string::npos) {
                    int cluster_geo = stoi(par.first.substr(0, pos_guion));
                    if (cluster_geo != punto_busqueda.id_cluster_geografico) {
                        for (const auto& punto : par.second) {
                            if (punto.id != punto_busqueda.id) {
                                resultado.push_back(punto);
                                if ((int)resultado.size() >= n_puntos) return resultado;
                            }
                        }
                    }
                }
            }
        }
        
        return resultado;
    }
    
    // Limpiar estructura
    void limpiar() {
        puntos_por_cluster.clear();
    }
};

struct Nodo{
    bool esHoja;
    int m_nivel;   // 0 = hoja; los internos llevan nivel de hijo + 1
    MBR mbr;
    vector<Nodo*> hijos;
    vector<PointID> puntos; //Solo para las hojas - OPTIMIZADO: usar PointID en lugar de Punto
    vector<MicroCluster> microclusters_en_Hoja; //Solo para las hojas
    Nodo* padre; //Puntero al padre
    Nodo(bool esHoja = false) : esHoja(esHoja), m_nivel(0), padre(nullptr) {}

    // Libera todo el subárbol (antes solo se liberaba la raíz: fuga de memoria)
    ~Nodo() {
        for (Nodo* hijo : hijos) delete hijo;
    }
    // Nodos poseen a sus hijos: prohibir copias para evitar dobles delete
    Nodo(const Nodo&) = delete;
    Nodo& operator=(const Nodo&) = delete;
};

class GeoCluster {
    private:
        Nodo* raiz;
        unordered_map<int, Punto> puntos_completos; // Hash table para almacenar puntos completos
        unordered_map<int, bool> niveles_reinsert;
        
    public:
        GeoCluster();
        ~GeoCluster();

        // Acceso de solo lectura a la raíz (recorridos externos: tests, estadísticas)
        Nodo* obtenerRaiz() const { return raiz; }
        
        // MÉTODOS PRINCIPALES
        void inserData(const Punto& punto);
        void insertar(const Punto& punto,int nivel);
        ResultadoBusqueda n_puntos_similiares_a_punto(const Punto& punto_de_busqueda, MBR& rango, int numero_de_puntos_similares);
        void crearMicroclustersEnHojas();
        void mostrarEstadisticasMicroclusters();
        ResultadoBusqueda consultaOptimizadaConMicroclusters(const Punto& punto_de_busqueda, MBR& rango, int numero_de_puntos_similares);
        void analizarRendimientoConsultas();
        void benchmarkConsultas();
        
        // FUNCIONES AUXILIARES
        MBR calcularMBR(const vector<Punto>& puntos);
        MBR calcularMBR(const vector<PointID>& puntos); // NUEVA: Para PointID
        void verificarDuplicados();
        
        // NUEVOS MÉTODOS PARA MANEJAR LA HASH TABLE DE PUNTOS COMPLETOS
        void almacenarPuntoCompleto(const Punto& punto);
        Punto obtenerPuntoCompleto(int id_punto) const;
        bool existePuntoCompleto(int id_punto) const;
        void limpiarPuntosCompletos();
        int obtenerNumeroPuntosCompletos() const;
        
        // NUEVAS FUNCIONES OPTIMIZADAS DE CONSULTA
        vector<Punto> buscarPuntosPorSimilitudCluster(const Punto& punto_referencia, const MBR& rango, int n_puntos);
        vector<Punto> buscarPuntosDentroInterseccion(const MBR& rango, Nodo* nodo);
        vector<Punto> buscarPuntosEnRangoOptimizado(const Punto& punto_referencia, const MBR& rango, int n_puntos);
        
        // FUNCIONES AUXILIARES PARA CONSULTAS OPTIMIZADAS
        Nodo* encontrarNodoHoja(const Punto& punto);
        vector<Nodo*> obtenerNodosHermanosEnRango(Nodo* nodo_actual, const MBR& rango);
        
        // FUNCIONES DE IMPRESIÓN Y DEBUG
        void imprimirArbol(Nodo* nodo, int nivel);
        void imprimirArbol();
        
        // FUNCIONES DE BÚSQUEDA TRADICIONAL
        vector<Punto> buscarPorSubcluster(int id_subcluster, MBR& rango);
        vector<Punto> buscarPorSimilitud(const Punto& referencia, double umbral_similitud, MBR& rango);
        vector<vector<Punto>> grupos_similares_de_puntos(MBR& rango);
        
        // FUNCIONES DE MICROCLUSTERS
        void crearMicroclustersRec(Nodo* nodo);
        void crearMicroclustersEnNodoHoja(Nodo* nodo_hoja);
        double calcularDistanciaAtributos(const vector<double>& centro, const vector<double>& atributos);
        
        // FUNCIONES DE ÁRBOL R* (Beckmann et al. 1990, sección 4)
        Nodo* chooseSubTree(const MBR& entrada, int nivel_destino);        // 4.1 CS1-CS3
        Nodo* hijoConMenorAmpliacionArea(Nodo* N, const MBR& entrada);
        Nodo* hijoConMenorAmpliacionOverlap(Nodo* N, const MBR& entrada);
        void overflowTreatment(Nodo* N);                                   // 4.3 OT1
        void reinsertar(Nodo* N);                                          // 4.3 RI1-RI4 (close reinsert)
        void split(Nodo* N);                                               // 4.2 S1-S3 + propagación I3
        void ajustarMBRHaciaArriba(Nodo* N);                               // I4
        void insertarSubarbol(Nodo* subarbol);                             // reinserción en niveles internos
        int chooseSplitAxis(const vector<PointID>& puntos);                // 4.2 CSA (expuesto para tests)
        void updateMBR(Nodo* nodo);
        bool interseccionMBR(const MBR& mbr1, const MBR& mbr2);
        void searchRec(const MBR& rango, Nodo* nodo, vector<Punto>& puntos_similares);
        bool estaDentroDelMBR(const Punto& punto, const MBR& mbr);
        bool estaDentroDelMBR(const PointID& point_id, const MBR& mbr); // NUEVA: Para PointID
        
        // FUNCIONES DE SIMILITUD
        double calcularSimilitudAtributos(const Punto& p1, const Punto& p2);
        double calcularSimilitudOptimizada(const Punto& p1, const Punto& p2);
        
        // FUNCIONES AUXILIARES
        void verificarDuplicadosRec(Nodo* nodo, vector<int>& ids);
        void buscarNodosHojasEnRango(Nodo* nodo, const MBR& rango, vector<Nodo*>& nodos_encontrados);
};

#endif
