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

// PARÁMETROS OPTIMIZADOS (basados en el código de ayuda)
// Código de ayuda usa: min=50, max=100
const int MAX_PUNTOS_POR_NODO = 1200;  // Reducido de 2000 para mayor eficiencia
const int MIN_PUNTOS_POR_NODO = 550;   // Reducido de 800 para mayor eficiencia
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

// Clase para matriz de similitud por nodo hoja (optimizada)
class MatrizSimilitudNodo {
    public:
        // Constructor
        MatrizSimilitudNodo() {}
        
        // Calcular similitud entre puntos del nodo
        void calcularSimilitudNodo(const vector<Punto>& puntos_nodo);
        
        // Obtener N puntos más similares dentro del nodo
        vector<Punto> obtenerSimilaresNodo(int punto_id, int n_similares, const vector<Punto>& puntos_nodo);
        
        // Obtener puntos similares en rango específico del nodo
        vector<Punto> obtenerSimilaresEnRangoNodo(int punto_id, const MBR& rango, int n_similares, const vector<Punto>& puntos_nodo);
        
        // Verificar si la matriz está calculada
        bool estaCalculada() const { return !ranking_similitud.empty(); }
        
        // Obtener estadísticas de la matriz
        int obtenerNumeroPuntos() const { return ranking_similitud.size(); }

        // Obtener similitud entre dos puntos si está calculada
        double obtenerSimilitud(int id1, int id2) const {
            auto it1 = ranking_similitud.find(id1);
            if (it1 == ranking_similitud.end()) return -1.0;
            
            // Buscar id2 en el ranking de id1
            const auto& ranking = it1->second;
            for (size_t i = 0; i < ranking.size(); ++i) {
                if (ranking[i] == id2) {
                    // La similitud está implícita en el orden del ranking
                    return 1.0 - (static_cast<double>(i) / ranking.size());
                }
            }
            return -1.0;
        }

        // Calcular similitud entre dos puntos (optimizado)
        static double calcularSimilitudPuntos(const Punto& p1, const Punto& p2) {
            // Verificar que los puntos sean válidos
            if (p1.atributos.empty() || p2.atributos.empty()) {
                return 0.0;
            }
            
            // 1. Similitud por atributos PCA (40% del peso)
            double similitud_atributos = 0.0;
            double distancia = 0.0;
            
            // Usar solo los primeros 8 componentes PCA (si están disponibles)
            int num_atributos = min(8, (int)min(p1.atributos.size(), p2.atributos.size()));
            
            for (int i = 0; i < num_atributos; i++) {
                double diff = p1.atributos[i] - p2.atributos[i];
                distancia += diff * diff;
            }
            
            similitud_atributos = exp(-sqrt(distancia));
            
            // 2. Similitud por cluster geográfico (30% del peso)
            double similitud_cluster_geo = 0.0;
            if (p1.id_cluster_geografico == p2.id_cluster_geografico) {
                similitud_cluster_geo = 1.0; // Mismo cluster = máxima similitud
            } else {
                similitud_cluster_geo = 0.0; // Diferente cluster = mínima similitud
            }
            
            // 3. Similitud por subcluster atributivo (30% del peso)
            double similitud_subcluster = 0.0;
            if (p1.id_subcluster_atributivo == p2.id_subcluster_atributivo) {
                similitud_subcluster = 1.0; // Mismo subcluster = máxima similitud
            } else {
                similitud_subcluster = 0.0; // Diferente subcluster = mínima similitud
            }
            
            // 4. Combinar similitudes con pesos
            double similitud_final = (0.4 * similitud_atributos) + 
                                   (0.3 * similitud_cluster_geo) + 
                                   (0.3 * similitud_subcluster);
            
            return similitud_final;
        }

        // Calcular similitud avanzada que incluye proximidad geográfica (optimizada)
        static double calcularSimilitudAvanzada(const Punto& p1, const Punto& p2) {
            // Verificar que los puntos sean válidos
            if (p1.atributos.empty() || p2.atributos.empty()) {
                return 0.0;
            }
            
            // 1. Similitud por atributos PCA (35% del peso)
            double similitud_atributos = 0.0;
            double distancia = 0.0;
            
            int num_atributos = min(8, (int)min(p1.atributos.size(), p2.atributos.size()));
            
            for (int i = 0; i < num_atributos; i++) {
                double diff = p1.atributos[i] - p2.atributos[i];
                distancia += diff * diff;
            }
            
            similitud_atributos = exp(-sqrt(distancia));
            
            // 2. Similitud por cluster geográfico (25% del peso)
            double similitud_cluster_geo = 0.0;
            if (p1.id_cluster_geografico == p2.id_cluster_geografico) {
                similitud_cluster_geo = 1.0;
            } else {
                similitud_cluster_geo = 0.0;
            }
            
            // 3. Similitud por subcluster atributivo (25% del peso)
            double similitud_subcluster = 0.0;
            if (p1.id_subcluster_atributivo == p2.id_subcluster_atributivo) {
                similitud_subcluster = 1.0;
            } else {
                similitud_subcluster = 0.0;
            }
            
            // 4. Similitud geográfica (15% del peso)
            double distancia_geo = sqrt(pow(p1.latitud - p2.latitud, 2) + pow(p1.longitud - p2.longitud, 2));
            double similitud_geografica = exp(-distancia_geo * 1000); // Factor de escala
            
            // 5. Combinar similitudes con pesos
            double similitud_final = (0.35 * similitud_atributos) + 
                                   (0.25 * similitud_cluster_geo) + 
                                   (0.25 * similitud_subcluster) + 
                                   (0.15 * similitud_geografica);
            
            return similitud_final;
        }

    private:
        // Hash table: id_punto -> vector de ids ordenados por similitud
        unordered_map<int, vector<int>> ranking_similitud;
};

//Estructura Nodo
struct Nodo{
    bool esHoja;
    int m_nivel;
    MBR mbr;
    vector<Nodo*> hijos;  
    vector<Punto> puntos; //Solo para las hojas
    vector<MicroCluster> microclusters_en_Hoja; //Solo para las hojas
    MatrizSimilitudNodo matriz_similitud; //NUEVO: Matriz de similitud por nodo hoja
    Nodo* padre; //Puntero al padre
    Nodo(bool esHoja = false) : esHoja(esHoja), m_nivel(0), padre(nullptr) {}
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
        similitudes[generarClave(id1, id2)] = similitud;
    }
    
    // Almacenar ranking para un punto
    void almacenarRanking(int id_punto, const vector<pair<double, int>>& ranking) {
        rankings[id_punto] = ranking;
    }
    
    // Obtener ranking de un punto
    const vector<pair<double, int>>& obtenerRanking(int id_punto) {
        return rankings[id_punto];
    }
    
    // Limpiar la matriz
    void limpiar() {
        similitudes.clear();
        rankings.clear();
    }
};

// Estructura optimizada para búsquedas rápidas por clusters
struct BusquedaOptimizada {
    // Hash table: cluster_geo_subcluster -> vector de puntos
    unordered_map<string, vector<Punto>> puntos_por_cluster;
    
    // Hash table: nodo_id -> matriz de similitud precalculada
    unordered_map<int, MatrizSimilitudNodo> matrices_por_nodo;
    
    // Función para generar clave de cluster
    string generarClaveCluster(int cluster_geo, int subcluster) {
        return to_string(cluster_geo) + "_" + to_string(subcluster);
    }
    
    // Agregar punto a la estructura
    void agregarPunto(const Punto& punto, int nodo_id) {
        string clave = generarClaveCluster(punto.id_cluster_geografico, punto.id_subcluster_atributivo);
        puntos_por_cluster[clave].push_back(punto);
    }
    
    // Obtener puntos por cluster de forma O(1)
    vector<Punto> obtenerPuntosPorCluster(int cluster_geo, int subcluster) {
        string clave = generarClaveCluster(cluster_geo, subcluster);
        auto it = puntos_por_cluster.find(clave);
        return (it != puntos_por_cluster.end()) ? it->second : vector<Punto>();
    }
    
    // Obtener puntos similares usando matrices precalculadas
    vector<Punto> obtenerSimilaresOptimizado(const Punto& punto_busqueda, int n_similares) {
        vector<Punto> resultado;
        
        // 1. Buscar puntos del mismo cluster y subcluster (máxima prioridad)
        string clave_mismo = generarClaveCluster(punto_busqueda.id_cluster_geografico, 
                                               punto_busqueda.id_subcluster_atributivo);
        auto it_mismo = puntos_por_cluster.find(clave_mismo);
        if (it_mismo != puntos_por_cluster.end()) {
            // Usar matriz precalculada si está disponible
            // Si no, calcular similitudes solo para estos puntos
            for (const auto& punto : it_mismo->second) {
                if (punto.id != punto_busqueda.id) {
                    double similitud = calcularSimilitudRapida(punto_busqueda, punto);
                    resultado.push_back(punto);
                    if (resultado.size() >= n_similares) return resultado;
                }
            }
        }
        
        // 2. Buscar puntos del mismo cluster pero diferente subcluster
        for (const auto& [clave, puntos] : puntos_por_cluster) {
            if (clave != clave_mismo && clave.substr(0, clave.find('_')) == 
                to_string(punto_busqueda.id_cluster_geografico)) {
                for (const auto& punto : puntos) {
                    double similitud = calcularSimilitudRapida(punto_busqueda, punto);
                    resultado.push_back(punto);
                    if (resultado.size() >= n_similares) return resultado;
                }
            }
        }
        
        // 3. Buscar puntos de otros clusters
        for (const auto& [clave, puntos] : puntos_por_cluster) {
            if (clave != clave_mismo && clave.substr(0, clave.find('_')) != 
                to_string(punto_busqueda.id_cluster_geografico)) {
                for (const auto& punto : puntos) {
                    double similitud = calcularSimilitudRapida(punto_busqueda, punto);
                    resultado.push_back(punto);
                    if (resultado.size() >= n_similares) return resultado;
                }
            }
        }
        
        return resultado;
    }
    
    // Calcular similitud rápida (sin PCA, solo atributos directos)
    double calcularSimilitudRapida(const Punto& p1, const Punto& p2) {
        // 1. Similitud por atributos directos (50%)
        double similitud_atributos = 0.0;
        double distancia = 0.0;
        
        int num_atributos = min((int)p1.atributos.size(), (int)p2.atributos.size());
        for (int i = 0; i < num_atributos; i++) {
            double diff = p1.atributos[i] - p2.atributos[i];
            distancia += diff * diff;
        }
        similitud_atributos = exp(-sqrt(distancia));
        
        // 2. Similitud por cluster geográfico (30%)
        double similitud_cluster = (p1.id_cluster_geografico == p2.id_cluster_geografico) ? 1.0 : 0.0;
        
        // 3. Similitud por subcluster (20%)
        double similitud_subcluster = (p1.id_subcluster_atributivo == p2.id_subcluster_atributivo) ? 1.0 : 0.0;
        
        return (0.5 * similitud_atributos) + (0.3 * similitud_cluster) + (0.2 * similitud_subcluster);
    }
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
    ResultadoBusqueda n_puntos_similiares_a_punto(const Punto& punto_de_busqueda, MBR& rango, int numero_de_puntos_similares);
    vector<vector<Punto>> grupos_similares_de_puntos(MBR& rango);
    
    // FUNCIONES DE CONSULTA OPTIMIZADAS PARA TIEMPO REAL
    vector<Punto> buscarPorSubcluster(int id_subcluster, MBR& rango);
    vector<Punto> buscarPorSimilitud(const Punto& referencia, double umbral_similitud, MBR& rango);
    
    // FUNCIONES PARA MICROCLUSTERS
    void crearMicroclustersEnHojas();
    void mostrarEstadisticasMicroclusters();
    
    // FUNCIONES DE CONSULTA OPTIMIZADAS CON MICROCLUSTERS
    ResultadoBusqueda consultaOptimizadaConMicroclusters(const Punto& punto_de_busqueda, MBR& rango, int numero_de_puntos_similares);
    vector<vector<Punto>> gruposSimilaresOptimizados(MBR& rango);
    
    // FUNCIONES DE ANÁLISIS DE RENDIMIENTO
    void analizarRendimientoConsultas();
    void benchmarkConsultas();
    
    // FUNCIONES PARA MATRICES DE SIMILITUD
    void calcularMatricesSimilitudEnHojas();
    
    vector<Punto> buscarPuntosDentroInterseccion(const MBR& rango, Nodo* nodo);
    MBR calcularMBR(const vector<Punto>& puntos);
    void imprimirArbol(Nodo* nodo = nullptr, int nivel = 0);
    int contarPuntosEnArbol(Nodo* nodo = nullptr);
    void verificarDuplicados();


private:
    Nodo* raiz = nullptr;
    unordered_map<int,bool> niveles_reinsert;
    MatrizSimilitudMBR matriz_mbr; // Nueva matriz temporal para MBR

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
    //void verificarDuplicadosRec(Nodo* nodo, vector<int>& ids);
    
    // FUNCIONES AUXILIARES PARA CONSULTAS
    double calcularSimilitudAtributos(const Punto& p1, const Punto& p2);
    vector<vector<Punto>> dividirGrupoPorProximidad(const vector<Punto>& grupo);
    
    // FUNCIONES AUXILIARES PARA MICROCLUSTERS
    void crearMicroclustersRec(Nodo* nodo);
    void crearMicroclustersEnNodoHoja(Nodo* nodo_hoja);
    double calcularDistanciaAtributos(const vector<double>& centro, const vector<double>& atributos);
    
    // FUNCIONES AUXILIARES PARA MATRICES DE SIMILITUD
    Nodo* encontrarNodoHoja(const Punto& punto);
    vector<Nodo*> obtenerNodosHermanosEnRango(Nodo* nodo_actual, const MBR& rango);
    void calcularMatricesSimilitudRec(Nodo* nodo);
    void buscarNodosHojasEnRango(Nodo* nodo, const MBR& rango, vector<Nodo*>& nodos_encontrados);
    
    // FUNCIONES DE VERIFICACIÓN Y DEBUG
    void verificarDuplicadosRec(Nodo* nodo, vector<int>& ids);

    // Después de insertar todos los puntos en el nodo, calcular la matriz de similitud
    // Llama a esto después de insertar todos los puntos
    inline void calcularMatrizSimilitudParaNodo(Nodo& nodo) {
        nodo.matriz_similitud.calcularSimilitudNodo(nodo.puntos);
    }

    // Función para obtener los N puntos más similares a un punto dado (por id)
    inline vector<Punto> obtenerNMasSimilaresEnNodo(Nodo& nodo, int punto_id, int n_similares) {
        return nodo.matriz_similitud.obtenerSimilaresNodo(punto_id, n_similares, nodo.puntos);
    }

    // Función auxiliar para calcular similitud optimizada
    double calcularSimilitudOptimizada(const Punto& p1, const Punto& p2);
};

#endif